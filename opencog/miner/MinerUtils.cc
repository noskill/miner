/*
 * MinerUtils.cc
 *
 * Copyright (C) 2018 SingularityNET Foundation
 *
 * Author: Nil Geisweiller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "MinerUtils.h"

#include <opencog/util/dorepeat.h>
#include <opencog/util/random.h>
#include <opencog/util/algorithm.h>

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/atoms/base/Link.h>
#include <opencog/atoms/base/Node.h>
#include <opencog/atoms/core/LambdaLink.h>
#include <opencog/atoms/core/RewriteLink.h>
#include <opencog/atoms/core/PresentLink.h>
#include <opencog/atoms/core/VariableList.h>
#include <opencog/atoms/core/NumberNode.h>
#include <opencog/atoms/core/FindUtils.h>
#include <opencog/atoms/core/TypeUtils.h>
#include <opencog/atoms/core/UnorderedLink.h>
#include <opencog/atoms/pattern/PatternLink.h>
#include <opencog/atoms/pattern/GetLink.h>
#include <opencog/query/Satisfier.h>

#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/numeric/conversion/cast.hpp>

namespace opencog
{

HandleSetSeq MinerUtils::shallow_abstract(const Valuations& valuations,
                                          unsigned ms)
{
	// Base case
	if (valuations.no_focus())
		return HandleSetSeq();

	// Recursive case
	HandleSetSeq shabs_per_var{focus_shallow_abstract(valuations, ms)};
	valuations.inc_focus_variable();
	HandleSetSeq remaining = shallow_abstract(valuations, ms);
	valuations.dec_focus_variable();
	append(shabs_per_var, remaining);
	return shabs_per_var;
}

HandleSet MinerUtils::focus_shallow_abstract(const Valuations& valuations,
                                             unsigned ms)
{
	HandleSet shabs;

	// No more variable to specialize from
	if (valuations.no_focus())
		return HandleSet();

	// Strongly connected valuations associated to the variable under
	// focus
	const SCValuations& var_scv(valuations.focus_scvaluations());

	////////////////////////////
	// Shallow abtractions    //
	////////////////////////////

	// For each valuation create an abstraction (shallow pattern) of
	// the value associated to variable, and associate the remaining
	// valuations to it.
	HandleUCounter shapats;
	// Calculate how many valuations will be encompassed by these
	// shallow abstractions
	unsigned val_count = valuations.size() / var_scv.size();
	for (const HandleSeq& valuation : var_scv.valuations) {
		const Handle& value = var_scv.focus_value(valuation);

		// If var_scv contains only one variable, then ignore shallow
		// abstractions of nodes and nullary links as they create
		// constant abstractions and
		//
		// 1. In case there is only one strongly connected component,
		//    constant patterns cannot have support > 1.
		//
		// 2. In case there are more than one strongly connected
		//    component, constant patterns are essentially useless
		//    (don't affect the support), and they will no longer
		//    reconnect, so they will remain useless.
		//
		// For these 2 reasons they can be safely ignored.
		if (valuation.size() == 1 and is_nullary(value))
			continue;

		// Otherwise generate its shallow abstraction
		if (Handle shabs = shallow_abstract_of_val(value))
			shapats[shabs] += val_count;
	}

	// Only consider shallow abstractions that reach the minimum
	// support
	for (const auto& shapat : shapats) {
		if (ms <= shapat.second) {
			set_support(shapat.first, shapat.second);
			shabs.insert(shapat.first);
		}
	}

	////////////////////////////////
	// Variable factorizations    //
	////////////////////////////////

	// Add all subsequent factorizable variables
	HandleSeq remvars = valuations.remaining_variables();
	HandleUCounter facvars;
	for (const Handle& rv : remvars) {
		// Strongly connected valuations associated to that variable
		const SCValuations& rv_scv(valuations.get_scvaluations(rv));

		// Index of rv in rv_scv
		unsigned rv_idx = rv_scv.index(rv);

		// Whether var and rv are in the same strongly connected
		// valuations (using pointer equality to speed it up)
		bool same_scv = &rv_scv == &var_scv;

		// Ref to keep track of the number of data tree instances where
		// the value of var is equal to the value to rv
		unsigned& rv_count = facvars[rv];

		// If they are in different stronly connected valuations, then
		// put all values of rv in a set, to quickly check if any
		// value is in.
		HandleUCounter rv_vals = same_scv ?
			HandleUCounter() : rv_scv.values(rv_idx);

		// Calculate how many valuations will be encompassed by this
		// variable factorization
		unsigned val_fac_count = val_count;
		if (not same_scv)
			val_fac_count /= rv_scv.size();

		for (const HandleSeq& valuation : var_scv.valuations) {
			// Value associated to var
			const Handle& val = valuation[var_scv.focus_index()];

			// If the value of var is equal to that of rv, then
			// increase rv factorization count
			if (same_scv) {
				if (content_eq(val, valuation[rv_idx])) {
					rv_count += val_fac_count;
				}
			}
			else {
				auto it = rv_vals.find(val);
				if (it != rv_vals.end()) {
					rv_count += val_fac_count * it->second;
				}
			}

			// If the minimum support has been reached, no need to
			// keep counting
			if (ms <= rv_count)
				break;
		}
	}

	// Only consider variable factorizations reaching the minimum
	// support
	for (const auto& fvar : facvars)
		if (ms <= fvar.second) {
			set_support(fvar.first, fvar.second);
			shabs.insert(fvar.first);
		}

	return shabs;
}

bool MinerUtils::is_nullary(const Handle& h)
{
	return h->is_node() or h->get_arity() == 0;
}

Handle MinerUtils::shallow_abstract_of_val(const Handle& value)
{
	// Node or empty link, nothing to abstract
	if (is_nullary(value))
		return value;

	Type tt = value->get_type();
	HandleSeq rnd_vars = gen_rand_variables(value->get_arity());
	Handle vardecl = variable_list(rnd_vars);

	// TODO: this can probably be simplified using PresentLink, would
	// entail to have RewriteLink::beta_reduce support PresentLink.

	// Links wrapped with LocalQuoteLink
	if (tt == AND_LINK or
	    tt == OR_LINK or
	    tt == NOT_LINK) {
		return lambda(vardecl, local_quote(createLink(rnd_vars, tt)));
	}

	if (tt == BIND_LINK or       // TODO: should probabably be replaced
	                             // by scope link and its subtypes
	    (tt == EVALUATION_LINK and
	     value->getOutgoingAtom(0)->get_type() == GROUNDED_PREDICATE_NODE) or
	    nameserver().isA(tt, FUNCTION_LINK) or
	    nameserver().isA(tt, VIRTUAL_LINK)) {
		// TODO: comment out the following lines when issue #1843 on the
		// atomspace repository has been fixed (see more about that
		// below).

		// // Wrap variables in UnquoteLink
		// HandleSeq uq_vars;
		// for (Handle& var : rnd_vars)
		// 	uq_vars.push_back(unquote(var));

		// return lambda(vardecl, quote(createLink(uq_vars, tt)));
		// TODO: ignore these links for now!!! In order to support them
		// we first need to address issue #1843 on the atomspace
		// repository. That is because otherwise the quotations inside
		// these patterns get wrongly consumed down the line (especially
		// while being used in the specialization rule defined in
		// rules/specialization.scm).
		return Handle::UNDEFINED;
	}

	// Links to ignore no matter what (till supported)
	if (tt == DEFINE_LINK)
		return Handle::UNDEFINED;

	// Generic non empty link, abstract away all the arguments
	return lambda(vardecl, createLink(rnd_vars, tt));
}

Handle MinerUtils::variable_list(const HandleSeq& vars)
{
	return vars.size() == 1 ? vars[0] : Handle(createVariableList(vars));
}

Handle MinerUtils::lambda(const Handle& vardecl, const Handle& body)
{
	return createLink(LAMBDA_LINK, vardecl, body);
}

Handle MinerUtils::quote(const Handle& h)
{
	return createLink(QUOTE_LINK, h);
}

Handle MinerUtils::unquote(const Handle& h)
{
	return createLink(UNQUOTE_LINK, h);
}

Handle MinerUtils::local_quote(const Handle& h)
{
	return createLink(LOCAL_QUOTE_LINK, h);
}

Handle MinerUtils::compose(const Handle& pattern, const HandleMap& var2pat)
{
	if (RewriteLinkPtr sc = RewriteLinkCast(pattern))
		return remove_useless_clauses(sc->beta_reduce(var2pat));
	return pattern;
}

Handle MinerUtils::vardecl_compose(const Handle& vardecl, const HandleMap& var2subdecl)
{
	OC_ASSERT((bool)vardecl, "Not implemented");

	Type t = vardecl->get_type();

	// Base cases

	if (t == VARIABLE_NODE) {
		auto it = var2subdecl.find(vardecl);
		// Compose if the variable maps to another variable
		// declaration
		if (it != var2subdecl.end())
			return it->second;
		return vardecl;
	}

	// Recursive cases

	if (t == VARIABLE_LIST) {
		HandleSeq oset;
		for (const Handle& h : vardecl->getOutgoingSet()) {
			Handle nh = vardecl_compose(h, var2subdecl);
			if (nh) {
				if (nh->get_type() == VARIABLE_LIST)
					for (const Handle nhc : nh->getOutgoingSet())
						oset.push_back(nhc);
				else
					oset.push_back(nh);
			}
		}

		if (oset.empty())
			return Handle::UNDEFINED;
		if (oset.size() == 1)
			return oset[0];
		return createLink(oset, t);
	}
	else if (t == TYPED_VARIABLE_LINK) {
		return vardecl_compose(vardecl->getOutgoingAtom(0), var2subdecl);
	}
	else {
		OC_ASSERT(false, "Not implemented");
		return Handle::UNDEFINED;
	}
}

HandleSeq MinerUtils::get_db(const Handle& db_cpt)
{
	// Retrieve all members of db_cpt
	HandleSeq db;
	IncomingSet member_links = db_cpt->getIncomingSetByType(MEMBER_LINK);
	for (const LinkPtr l : member_links) {
		Handle member = l->getOutgoingAtom(0);
		if (member != db_cpt)
			db.push_back(member);
	}
	return db;
}

unsigned MinerUtils::get_uint(const Handle& h)
{
	NumberNodePtr nn = NumberNodeCast(h);
	return (unsigned)std::round(nn->get_value());
}

unsigned MinerUtils::support(const Handle& pattern,
                             const HandleSeq& db,
                             unsigned ms)
{
	// Partition the pattern into strongly connected components
	HandleSeq cps(get_component_patterns(pattern));

	// Likely a constant pattern
	if (cps.empty())
	    return 1;

	// Otherwise calculate the frequency of each component
	std::vector<unsigned> freqs;
	boost::transform(cps, std::back_inserter(freqs),
	                 [&](const Handle& cp)
	                 { return component_support(cp, db, ms); });

	// Return the product of all frequencies
	return boost::accumulate(freqs, 1, std::multiplies<unsigned>());
}

unsigned MinerUtils::component_support(const Handle& component,
                                       const HandleSeq& db,
                                       unsigned ms)
{
	if (totally_abstract(component))
		return db.size();
	return restricted_satisfying_set(component, db, ms)->get_arity();
}

bool MinerUtils::enough_support(const Handle& pattern,
                                const HandleSeq& db,
                                unsigned ms)
{
	return ms <= support_mem(pattern, db, ms);
}

HandleSetSeq MinerUtils::shallow_abstract(const Handle& pattern,
                                          const HandleSeq& db,
                                          unsigned ms)
{
	Valuations valuations(pattern, db);
	return shallow_abstract(valuations, ms);
}

HandleSet MinerUtils::shallow_specialize(const Handle& pattern,
                                         const HandleSeq& db,
                                         unsigned ms,
                                         unsigned mv)
{
	// Calculate all shallow abstractions of pattern
	HandleSetSeq shabs_per_var = shallow_abstract(pattern, db, ms);

	// For each variable of pattern, generate the corresponding shallow
	// specializations
	const Variables& vars = MinerUtils::get_variables(pattern);
	size_t vari = 0;
	HandleSet results;
	for (const HandleSet& shabs : shabs_per_var) {
		for (const Handle& sa : shabs) {
			Handle npat = compose(pattern, {{vars.varseq[vari], sa}});

			if (mv < get_variables(npat).size())
				continue;

			// Set the count of npat, stored in its shallow abstraction
			set_support(npat, get_support(sa));
			// Shallow_abstract should already have eliminated shallow
			// abstraction that do not have enough support.
			results.insert(npat);
		}
		vari++;
	}
	return results;
}

Handle MinerUtils::mk_body(const HandleSeq& clauses)
{
	if (clauses.size() == 0)
		return Handle::UNDEFINED;
	if (use_present_link)
		return Handle(createPresentLink(clauses));
	if (clauses.size() == 1)
		return clauses.front();
	return Handle(createLink(clauses, AND_LINK));
}

Handle MinerUtils::mk_pattern_no_vardecl(const HandleSeq& clauses)
{
	return Handle(createLambdaLink(HandleSeq{mk_body(clauses)}));
}

Handle MinerUtils::mk_pattern_filtering_vardecl(const Handle& vardecl,
                                                const HandleSeq& clauses)
{
	Handle fvd = filter_vardecl(vardecl, clauses);
	Handle body = mk_body(clauses);
	if (fvd != nullptr and body != nullptr)
		return Handle(createLambdaLink(fvd, body));
	return Handle::UNDEFINED;
}

Handle MinerUtils::mk_pattern(const Handle& vardecl,
                              const HandleSeq& clauses)
{
	return Handle(createLambdaLink(vardecl, mk_body(clauses)));
}

HandleSeq MinerUtils::get_component_patterns(const Handle& pattern)
{
	PatternLink pl(MinerUtils::get_vardecl(pattern),
	               MinerUtils::get_body(pattern));
	HandleSeq compats;
	const HandleSeqSeq comps(pl.get_components());
	for (unsigned i = 0; i < comps.size(); ++i)
	{
		Handle comp = mk_pattern_filtering_vardecl(get_vardecl(pattern),
		                                           comps[i]);
		if (comp)
			compats.push_back(comp);
	}
	return compats;
}

HandleSeqSeq MinerUtils::get_components(const HandleSeq& clauses)
{
	return PatternLink(mk_body(clauses)).get_components();
}

HandleSeq MinerUtils::get_conjuncts(const Handle& pattern)
{
	if (pattern->get_type() == LAMBDA_LINK) {
		Handle body = get_body(pattern);
		Type bt = body->get_type();
		if (bt == AND_LINK or bt == PRESENT_LINK) {
			Handle vardecl = get_vardecl(pattern);
			HandleSeq conjs;
			for (const Handle& clause : body->getOutgoingSet()) {
				Handle conj = mk_pattern(vardecl, {clause});
				if (conj)
					conjs.push_back(conj);
			}
			return conjs;
		}
		return {pattern};
	}
	return {};
}

Handle MinerUtils::restricted_satisfying_set(const Handle& pattern,
                                             const HandleSeq& db,
                                             unsigned ms)
{
	static AtomSpace tmp_db_as; // TODO: fix to be thread safe
	tmp_db_as.clear();
	HandleSeq tmp_db;
	for (const auto& dt : db)
		tmp_db.push_back(tmp_db_as.add_atom(dt));

	// Avoid pattern matcher warning
	if (totally_abstract(pattern) and n_conjuncts(pattern) == 1)
		return tmp_db_as.add_link(SET_LINK, tmp_db);

	// Define pattern to run
	AtomSpace tmp_query_as(&tmp_db_as);
	Handle tmp_pattern = tmp_query_as.add_atom(pattern),
		vardecl = get_vardecl(tmp_pattern),
		body = get_body(tmp_pattern),
		gl = tmp_query_as.add_link(GET_LINK, vardecl, body);

	// Run pattern matcher
	SatisfyingSet sater(&tmp_db_as);
	sater.max_results = ms;
	GetLinkCast(gl)->satisfy(sater);

	return Handle(createUnorderedLink(sater._satisfying_set, SET_LINK));
}

bool MinerUtils::totally_abstract(const Handle& pattern)
{
	// Check whether it is an abstraction to begin with
	if (pattern->get_type() != LAMBDA_LINK)
		return false;

	// If some variables are typed then the abstraction isn't total
	const Variables& vars = get_variables(pattern);
	if (not vars._simple_typemap.empty() or not vars._deep_typemap.empty())
		return false;

	// Make sure the body is either a variable, or a conjunction of
	// variables
	Handle body = get_body(pattern);
	Type bt = body->get_type();
	if (bt == VARIABLE_NODE)
		return true;
	if (bt != AND_LINK and bt != PRESENT_LINK)
		return false;
	for (const Handle& ch : body->getOutgoingSet())
		if (ch->get_type() != VARIABLE_NODE)
			return false;
	return true;
}

HandleSeq MinerUtils::gen_rand_variables(size_t n)
{
	HandleSeq variables;
	dorepeat (n)
		variables.push_back(gen_rand_variable());
	return variables;
}

Handle MinerUtils::gen_rand_variable()
{
	return createNode(VARIABLE_NODE, randstr("$PM-"));
}

const Variables& MinerUtils::get_variables(const Handle& pattern)
{
	if (RewriteLinkPtr sc = RewriteLinkCast(pattern))
		return RewriteLinkCast(pattern)->get_variables();
	static Variables empty_variables;
	return empty_variables;
}

Handle MinerUtils::get_vardecl(const Handle& pattern)
{
	if (RewriteLinkPtr sc = RewriteLinkCast(pattern)) {
		Handle vardecl = sc->get_vardecl();
		if (not vardecl)
			vardecl = sc->get_variables().get_vardecl();
		return vardecl;
	}
	return Handle::UNDEFINED;
}

const Handle& MinerUtils::get_body(const Handle& pattern)
{
	if (RewriteLinkPtr sc = RewriteLinkCast(pattern))
		return sc->get_body();
	return pattern;
}

HandleSeq MinerUtils::get_clauses(const Handle& pattern)
{
	return get_clauses_of_body(get_body(pattern));
}

HandleSeq MinerUtils::get_clauses_of_body(const Handle& body)
{
	Type bt = body->get_type();
	if (bt == AND_LINK or bt == PRESENT_LINK)
		return body->getOutgoingSet();
	return {body};
}

unsigned MinerUtils::n_conjuncts(const Handle& pattern)
{
	if (pattern->get_type() == LAMBDA_LINK) {
		Type bt = get_body(pattern)->get_type();
		if (bt == AND_LINK or bt == PRESENT_LINK)
			return get_body(pattern)->get_arity();
		return 1;
	}
	return 0;
}

Handle MinerUtils::remove_useless_clauses(const Handle& pattern)
{
	Handle vardecl = get_vardecl(pattern),
		body = get_body(pattern);
	body = remove_useless_clauses(vardecl, body);
	return Handle(createLambdaLink(vardecl, body));
}

Handle MinerUtils::remove_useless_clauses(const Handle& vardecl,
                                          const Handle& body)
{
	// Remove useless clauses
	HandleSeq clauses = get_clauses(body);
	remove_useless_clauses(vardecl, clauses);
	// Reconstruct body
	return mk_body(clauses);
}

void MinerUtils::remove_useless_clauses(const Handle& vardecl, HandleSeq& clauses)
{
	remove_constant_clauses(vardecl, clauses);
	remove_redundant_subclauses(clauses);
}

void MinerUtils::remove_constant_clauses(const Handle& vardecl, HandleSeq& clauses)
{
	// Get Variables
	VariableListPtr vl = createVariableList(vardecl);
	const HandleSet& vars = vl->get_variables().varset;

	// Remove constant clauses
	auto is_constant = [&](const Handle& clause) {
		return not any_unquoted_unscoped_in_tree(clause, vars); };
	boost::remove_erase_if(clauses, is_constant);
}

void MinerUtils::remove_redundant_subclauses(HandleSeq& clauses)
{
	// Check that each clause is not a subtree of another clause,
	// remove it otherwise.
	for (auto it = clauses.begin(); it != clauses.end();) {
		// Take all clauses except *it
		HandleSeq others(clauses.begin(), it);
		others.insert(others.end(), std::next(it), clauses.end());

		// Make sure *it is not a subtree of any other
		if (is_unquoted_unscoped_in_any_tree(others, *it))
			it = clauses.erase(it);
		else
			++it;
	}
}

void MinerUtils::remove_redundant_clauses(HandleSeq& clauses)
{
	boost::sort(clauses);
	typedef std::equal_to<opencog::Handle> HandleEqual;
	boost::erase(clauses,
	             boost::unique<boost::return_found_end, HandleSeq, HandleEqual>
	             (clauses, HandleEqual()));
}

Handle MinerUtils::alpha_convert(const Handle& pattern,
                                 const Variables& other_vars)
{
	const Variables& pattern_vars = get_variables(pattern);

	// Detect collision between pattern_vars and other_vars
	HandleMap aconv;
	for (const Handle& var : pattern_vars.varset) {
		if (other_vars.is_in_varset(var)) {
			Handle nvar;
			bool used;
			do {
				nvar = createNode(VARIABLE_NODE, randstr(var->get_name() + "-"));
				// Make sure it is not in other_vars or pattern_vars
				used = other_vars.is_in_varset(nvar) or pattern_vars.is_in_varset(nvar);
			} while (used);
			aconv[var] = nvar;
		}
	}

	// No collision
	if (aconv.empty())
		return pattern;

	// Collisions, need to alpha convert vardecl and body
	Handle nvardecl = pattern_vars.substitute_nocheck(get_vardecl(pattern), aconv);
	Handle nbody = pattern_vars.substitute_nocheck(get_body(pattern), aconv);

	// Reconstruct alpha converted pattern
	return Handle(createLambdaLink(nvardecl, nbody));
}

Handle MinerUtils::expand_conjunction_disconnect(const Handle& cnjtion,
                                                 const Handle& pattern)
{
	// Copy variables from cnjtion as it will be extended
	Variables cnjtion_vars = get_variables(cnjtion);

	// Alpha convert pattern, if necessary, to avoid collisions between
	// cnjtion_vars and pattern variables
	Handle acpat = alpha_convert(pattern, cnjtion_vars);

	// Extend cnjtion_vars with pattern variables
	cnjtion_vars.extend(get_variables(acpat));

	// Expand cnjtion body with pattern, flattening the body if necessary
	HandleSeq nclauses = get_clauses(cnjtion);
	append(nclauses, get_clauses(acpat));

	// Remove redundant subclauses. This can happen if there's only one
	// variable to connect, then some subclause turn out to be
	// redundant.
	remove_redundant_subclauses(nclauses);

	// Recreate expanded conjunction
	Handle nvardecl = cnjtion_vars.get_vardecl(),
		npattern = mk_pattern(nvardecl, nclauses);

	return npattern;
}

Handle MinerUtils::expand_conjunction_connect(const Handle& cnjtion,
                                              const Handle& pattern,
                                              const Handle& cnjtion_var,
                                              const Handle& pattern_var)
{
	HandleMap p2c{{pattern_var, cnjtion_var}};
	return expand_conjunction_connect(cnjtion, pattern, p2c);
}

Handle MinerUtils::expand_conjunction_connect(const Handle& cnjtion,
                                              const Handle& pattern,
                                              const HandleMap& pv2cv)
{
	// Substitute pattern variables by cnjtion variables in pattern
	Variables pattern_vars = get_variables(pattern);
	Handle npat_body = pattern_vars.substitute_nocheck(get_body(pattern), pv2cv);
	for (const auto& el : pv2cv)
		pattern_vars.erase(el.first);

	// Extend cnjtion variables with the pattern variables, except
	// mapped variables
	Variables cnjtion_vars = get_variables(cnjtion);
	cnjtion_vars.extend(pattern_vars);

	// Expand cnjtion body with npat_body, flattening cnjtion_body if necessary
	const Handle& cnjtion_body = get_body(cnjtion);
	HandleSeq nclauses = get_clauses(cnjtion_body);
	append(nclauses, get_clauses_of_body(npat_body));

	// Remove redundant subclauses. This can happen if there's only one
	// variable to connect, then some subclause turn out to be
	// redundant.
	remove_redundant_subclauses(nclauses);

	// Recreate expanded conjunction
	Handle nvardecl = cnjtion_vars.get_vardecl(),
		npattern = mk_pattern(nvardecl, nclauses);

	return npattern;
}

HandleSet MinerUtils::expand_conjunction_rec(const Handle& cnjtion,
                                             const Handle& pattern,
                                             const HandleSeq& db,
                                             unsigned ms,
                                             unsigned mv,
                                             const HandleMap& pv2cv,
                                             unsigned pvi)
{
	HandleSet patterns;
	const Variables& cvars = get_variables(cnjtion);
	const Variables& pvars = get_variables(pattern);
	for (; pvi < pvars.size(); pvi++) {
		for (const Handle& cv : cvars.varseq) {
			HandleMap pv2cv_ext(pv2cv);
			pv2cv_ext[pvars.varseq[pvi]] = cv;
			Handle npat = expand_conjunction_connect(cnjtion, pattern, pv2cv_ext);

			// If the number of variables is too high or the number of
			// conjuncts has dropped then it shouldn't be considered.
			if (get_variables(npat).size() <= mv and
			    n_conjuncts(cnjtion) < n_conjuncts(npat)) {

				// Insert npat in the atomspace where cnjtion and pattern
				// are, before memoizing its support.
				if (cnjtion->getAtomSpace())
					npat = cnjtion->getAtomSpace()->add_atom(npat);

				// If npat does not have enough support, any recursive
				// call will produce specializations that do not have
				// enough support, thus can be ignored.
				if (not enough_support(npat, db, ms))
					continue;

				patterns.insert(npat);
			}

			HandleSet rrs = expand_conjunction_rec(cnjtion, pattern, db, ms, mv,
			                                       pv2cv_ext, pvi + 1);
			patterns.insert(rrs.begin(), rrs.end());
		}
	}
	return patterns;
}

HandleSet MinerUtils::expand_conjunction_es_rec(const Handle& cnjtion,
                                                const Handle& pattern,
                                                const HandleSeq& db,
                                                unsigned ms,
                                                unsigned mv,
                                                const HandleMap& pv2cv,
                                                unsigned pvi)
{
	const Variables& pvars = get_variables(pattern);

	/////////////////
	// Base case   //
	/////////////////

	// If pv2cv is total (thus specialization is guarantied) then we
	// can build the conjunction.
	if (pv2cv.size() == pvars.size()) {
		Handle npat = expand_conjunction_connect(cnjtion, pattern, pv2cv);

		// If the number of variables is too high or the number of
		// conjuncts has dropped then it shouldn't be considered.
		if (mv < get_variables(npat).size() or
		    n_conjuncts(npat) <= n_conjuncts(cnjtion))
			return {};

		// Insert npat in the atomspace where cnjtion and pattern
		// are, before memoizing its support.
		if (cnjtion->getAtomSpace())
			npat = cnjtion->getAtomSpace()->add_atom(npat);

		// If npat does not have enough support, it shouldn't be
		// considered.
		if (not enough_support(npat, db, ms))
			return {};

		return {npat};
	}

	//////////////////////
	// Recursive case   //
	//////////////////////

	HandleSet patterns;
	const Variables& cvars = get_variables(cnjtion);
	for (const Handle& cv : cvars.varseq) {
		HandleMap pv2cv_ext(pv2cv);
		pv2cv_ext[pvars.varseq[pvi]] = cv;
		HandleSet rrs = expand_conjunction_es_rec(cnjtion, pattern, db, ms,
		                                          mv, pv2cv_ext, pvi + 1);
		patterns.insert(rrs.begin(), rrs.end());
	}
	return patterns;
}

HandleSet MinerUtils::expand_conjunction(const Handle& cnjtion,
                                         const Handle& pattern,
                                         const HandleSeq& db,
                                         unsigned ms,
                                         unsigned mv,
                                         bool es)
{
	// Alpha convert pattern, if necessary, to avoid collisions between
	// cnjtion variables and pattern variables
	Handle apat = alpha_convert(pattern, get_variables(cnjtion));

	// Consider all variable mappings from apat to cnjtion
	return es ?
		expand_conjunction_es_rec(cnjtion, apat, db, ms, mv)
		: expand_conjunction_rec(cnjtion, apat, db, ms, mv);
}

const Handle& MinerUtils::support_key()
{
	static Handle ck(createNode(NODE, "*-SupportValueKey-*"));
	return ck;
}

void MinerUtils::set_support(const Handle& pattern, double support)
{
	FloatValuePtr support_fv = createFloatValue(boost::numeric_cast<double>(support));
	pattern->setValue(support_key(), ValueCast(support_fv));
}

double MinerUtils::get_support(const Handle& pattern)
{
	FloatValuePtr support_fv = FloatValueCast(pattern->getValue(support_key()));
	if (support_fv)
		return support_fv->value().front();
	return -1.0;
}

double MinerUtils::support_mem(const Handle& pattern,
                               const HandleSeq& db,
                               unsigned ms)
{
	double sup = get_support(pattern);
	if (sup < 0) {
		sup = support(pattern, db, ms);
		set_support(pattern, sup);
	}
	return sup;
}

} // namespace opencog
