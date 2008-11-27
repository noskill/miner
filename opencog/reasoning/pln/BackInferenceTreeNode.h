#ifndef BACK_INFERENCE_TREE_NODE
#define BACK_INFERENCE_TREE_NODE

#define FORMULA_CAN_COMPUTE_WITH_EMPTY_ARGS 0

/**
 * Original prototype code by Ari A. Heljakka Sep/2006.
 * Port by Joel Pitt 2008.
 *
 */

#include <stack>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "rules/RuleApp.h"
#include "utils/NMPrinter.h"

namespace reasoning
{
class BITNode;
class BITNodeRoot;  // Root of a BIT, controls inference
class RuleProvider; // Provide rules when expanding the tree

enum spawn_mode { NO_SIBLING_SPAWNING = 0, ALLOW_SIBLING_SPAWNING };

struct BITNode_fitness_comp : public binary_function<BITNode*, BITNode*, bool>
{
    bool operator()(BITNode* lhs, BITNode* rhs) const;
};

/** 
	Store the knowledge of which argument slow I inhabit in my parent's Rule,
	and which variable-to-variable bindings the parent is applying to me,
	ie. "How does the parent view me"
*/
template<typename T>
struct parent_link
{
	T* link;
	uint parent_arg_i;
	Btr<bindingsT> bindings;

	parent_link() : link(NULL), parent_arg_i(0) {}
	parent_link(	T* _link,
			uint _parent_arg_i,
			Btr<bindingsT> _bindings = Btr<bindingsT>(new bindingsT))
		: link(_link), parent_arg_i(_parent_arg_i), bindings(_bindings) {}

	bool operator<(const parent_link& rhs) const {
		return link<rhs.link
			|| (link==rhs.link && parent_arg_i < rhs.parent_arg_i)
			|| (link==rhs.link && parent_arg_i == rhs.parent_arg_i && bindings < rhs.bindings);
	}
};

/// The generic BITNode in which some variables may be bound to other variables
/// A single ParametrizedBITNode can be used to represent various trees by using
/// different variable bindings. These bindings are for variables internal to
/// the PLN backward chainer as opposed to the variables within PLN theory.
class ParametrizedBITNode
{
public:
	Btr<bindingsT> bindings;
	BITNode* prover;

	ParametrizedBITNode(BITNode* _prover, Btr<bindingsT> _bindings);

	bool operator<(const ParametrizedBITNode& rhs) const
	{
		return (prover<rhs.prover  || (prover==rhs.prover && bindings<rhs.bindings));
	}
};

/** The basic node in the Backward Inference proof Tree
 *
 * Notes about the Backward Inference Tree and the Backward Chaining process.
 * - Circularity is not tolerated, though the algorithm could probably be
 *   expressed in a way that would allow for this. However, it turns out that
 *   circularity is never needed (I am not sure if this can be rigorously
 *   proven; possibly so).
 * - Subtrees are recycled by storing them in BITNode "templates". A subtree is
 *   reached via a channel that has inheritance-bindings associated to it, so
 *   that if we are already proving Inh(A,$1), we don't need a new tree for
 *   proving Inh(A,$2), but instead use the same tree with inheritance-binding
 *   ($1->$2).
 * - FW_VARIABLE_NODEs are used to denote placeholders for "future atoms that
 *   the bw chainer finds". Normal VariableNodes denote variables in
 *   variable-laden links that are real and reside in the AtomSpace.
 *   FW_VARIABLE_NODEs should normally not be stored in the AtomSpace, but
 *   currenty there are.
 *
 * The basic BW chainer dispatch cycle for expanding the tree with 1 inference
 * step is (omitting procedures such as the argument place of each BITNode,
 * result validation, expansion pool filtering etc.):
 *
 * -# BITNodeRoot.executionPool.insert(root)
 * -# currentBITNode = getFittest(root.executionPool)
 * -# root.executionPool = currentBITNode.expand()
 * -# if (not sufficient(root.getResults())): goto #2
 *
 * where...
 *
 * getFittest(pool):
 * \code
 * 	 returns the fittest entry in pool. This depends on a fitness function
 * 	 related to tree pruning algorithm discussed in DAPPIE. 
 * \endcode
 * 
 * currentBITNode.expand():
 * \code
 *    For currentBITNode.target, put into RS all Rule objects that could create it
 *    For each Composer C in RS:
 *  	   childCreate(C,RS)
 *  	 For each Generator G in RS:
 *  	   put into GS all possible atoms produced by G that match currentBITNode.target
 *    For each Handle H in GS:
 *      If GS was produced without binding any FW variables:
 *        put H into currentBITNode.results
 *      Else For each variable bind (var_i => H_i) made to get H:
 *        For the owner O of var_i:
 *          O.tryClone(var_i => H_i)
 *      C.evaluate(H)
 * \endcode
 *
 * O.tryClone(var_i => H_i):
 * \code
 *    For each parent P of O:
 *      If P has inheritance-bindings (var2_i => var_i):
 *        P.tryClone(var2_i => H_i)
 *      Else:
 *        P.childCreate(a copy of O such that all instances of var_i have been
 *        replaced with H_i)
 * \endcode
 * 
 * P.childCreate(Rule R, args AS):
 * \code
 *    If root.recyclerPool does not contain a BITNode that can be produced as B'
 *    from (R,AS) by some variable bindings {(Var_i=>Var2_i)} of variables in AS
 *    to other variables, then:
 *      Add B' to root.recyclerPool
 *      B = B'
 *    Else:
 *      B = ParametrizedBITNode(B') with inheritance-bindings (Var_i=>Var2_i)
 *      such that B owns each new variable from R.o2i
 *      Mark P and all users of P as users of B.
 *    add B to P.children
 *    make P the parent of B
 *    return B
 * \endcode
 * 
 * BITNode.evaluate(latest_result) = try to compute all the results of the Rule
 * of this BITNode by using latest_result in conjunction with old results.
 * \code
 *    For each new_result in results:
 *      For each parent:
 *        call parent.Evaluate(new_result). 
 * \endcode
 * 
 *
 * @todo investigate keeping FW_VARIABLE_NODEs out of the AtomSpace.
 *
 * @remarks BITNode supports pre-bindings extensively, but it's unclear whether they are
 * obsolete or not.
 *
 */
class BITNode
{
	friend class BITNode_fitness_comp;
	friend class bdrum_updater;
	friend class BITNodeRoot;
	friend class ExplicitlyEvaluatedBITNode;
protected:
	/// Contains the target of this inference state node
	meta bound_target, raw_target;

	/// For heuristics; store the nr so you don't have to re-count.
	uint counted_number_of_free_variables_in_target;

    /// parent is the set of parent_links that refer upwards in the BIT.
    /// A set is used to store the parent links because a node can have
    /// multiple parents when the parent node has different
    /// bindings
	mutable set<parent_link<BITNode> > parents;

	/// The results produced by combining all the possible results
	/// for my arguments (contained as sets), with an entry in the vector for
    /// each argument.
	vector<set<VtreeProvider*> > eval_results;

	unsigned int depth;
	BITNodeRoot* root;

	bool Expanded;

	/// The Rule that the child_results of this state object's child_results will be
	/// associated with. This BITNode can be thought of as an implementation of
    /// this Rule
	Rule *rule;
	
    /// bdrum = the Best (confidence of) Direct Result Under Me
	float my_bdrum;
	
	typedef set<vtree, less_tree_vertex> vtreeset;

	/// Targets of all atoms above me... obsolete but partially still functions
	vtreeset target_chain;

	/// The direct results stored in this node. This approach is somewhat
	/// clumsy but gets the job done. The direct results that do not need
    /// evaluation. Could be referred to as "generators". One example is
    /// the LookUp rule which just checks if an atom exists.
	Btr<set<BoundVertex> > direct_results;

	/// The vector of targets of my children, which will then become Rule arguments.
    /// Refers downwards in the BIT.
	Rule::MPs args;

	/* METHODS */

	void SetTarget(meta _target, Btr<bindingsT> binds);
	meta GetTarget() const { return bound_target; }

	/// Apply an operator to all nodes that have this one as a node
	
	template<typename _opT, typename _stateT> bool ApplyUp(_opT _op, _stateT my_state) {
		_op(this);
		foreach(parent_link<BITNode> p, parents)
			p.link->ApplyUp(_op, my_state);

		return true;
	}

	/// Apply an operator to all nodes below this one (and to this one)
	template<typename opT> void ApplyDown(opT op)
	{
		using namespace boost;
		
		for (vector<set<ParametrizedBITNode> >::iterator i =  children.begin(); i!=children.end(); i++)
			foreach(const ParametrizedBITNode& pbit, *i)
				pbit.prover->ApplyDown<opT>(op);
		op(this);
	}
	
	/// Apply an operator to all nodes below this one (and to this one) in reverse
	template<typename opT> bool ApplyDown2(opT op)
	{
		using namespace boost;
		
		if (!op(this))
			return false;

		for (vector<set<ParametrizedBITNode> >::iterator i =  children.begin(); i!=children.end(); i++)
			foreach(ParametrizedBITNode& pbit, *i)
				if (!pbit.prover->ApplyDown2<opT>(op))
					return false;

		return true;
	}

	/// Inform the parents of a new result
	bool NotifyParentOfResult(VtreeProvider* new_result) const;
	
	/// Evaluate the Rule with the given new result as arg #arg_i
    /// Allows one to add arguments one by one and evaluating when all slots
    /// filled. Alternatively there has been discussion to evaluate empty slots
    /// as arguments with 0 confidence.
	void EvaluateWith(unsigned int arg_i, VtreeProvider* new_result);
	
	/// Creation of child nodes
	/// Normally arg = args[rule_arg_i], but children with differently-bound targets
	/// may also be created.
    /// == expansion of the BIT.
	BITNode* CreateChild(unsigned int my_rule_arg_i, Rule* new_rule, const Rule::MPs& rule_args, 
						BBvtree arg, const bindingsT& bindings,spawn_mode spawning);
	bool CreateChildren(int rule_arg_i, BBvtree arg, Btr<bindingsT> bindings, spawn_mode);
	void CreateChildrenForAllArgs();

	/// Check for whether the target atom can simply be Generated. Here, Generated = "direct"
	/// Note: Rules come in 2 flavours: Composers and Generators. See PLN implementation docs.
	bool CheckForDirectResults();
	BITNode* HasChild(int arg_i, Rule* r,  const Rule::MPs& rule_args, meta target, const bindingsT& _pre_bindings) const;
	BITNode* HasChild(BITNode* new_child, int arg_i) const;

	/// Add a new parent BITNode. Note: whenever a new BITNode starts to use me, via some parametrization,
	/// it becomes my parent.
	void addNewParent(parent_link<BITNode> new_parent);

	bool inferenceLoop(Rule::MPs reqs);
	bool inferenceLoopWith(meta req);

	/// helper
	void ValidateRuleArgs(const vector<BoundVertex>& rule_args) const;

	/// helper
	template<typename IterT>
	void ValidateRuleArgs(IterT rule_args_begin, IterT rule_args_end) const
	{

		/// Check for wild FreeVariableNodes 

		/*				foreach(BoundVertex v, rule_args)
		{
		if (v.bindings)
		foreach(hpair t, *v.bindings)
		{
		if (nm->getType(t.second) == FW_VARIABLE_NODE)
		{
		LOG(0, "FW_VARIABLE_NODE found on binding's RHS!");
		getc(stdin);
		}
		}
		}	*/
		//AtomSpace *nm = CogServer::getAtomSpace();
		for(IterT bv = rule_args_begin; bv != rule_args_end; bv++)
			if (GET_ATW->getType(v2h(*(*bv)->getVtree().begin())) == FW_VARIABLE_NODE)
			{
/*				LOG(0, "FW_VARIABLE_NODE found on Rule args: "+string(nm->getName(v2h(bv->value))));

				tlog(0,"Args were:\n");
				foreach(BoundVertex v, rule_args)
					printTree(v2h(v.value),0,0);*/
			}
	}


	/// helper
	//bool ValidRuleResult(BoundVertex& new_result, const vector<BoundVertex>& rule_args, Btr<bindingsT> bindings_of_all_args) const;

	template<typename ArgIterT>
	bool ValidRuleResult(	BoundVertex& new_result,
									ArgIterT rule_args_begin,
									ArgIterT rule_args_end,
									Btr<bindingsT> bindings_of_all_args) const
	{
		Handle* ph;

		if (!(!new_result.bindings ||new_result.bindings->empty()))
			printSubsts(new_result, -1);
		assert(!new_result.bindings ||new_result.bindings->empty());

        if ((ph = v2h(&new_result.value)) && *ph != Handle::UNDEFINED)
		{
			puts("Rule returned NULL! Args were:\n");

			for_each(rule_args_begin, rule_args_end,
				boost::bind<void>(
					NMPrinter(	NMP_DEFAULT,NM_PRINTER_DEFAULT_TRUTH_VALUE_PRECISION,
						NM_PRINTER_DEFAULT_INDENTATION_TAB_SIZE, -2),
					bind(&VtreeProvider::getVtree, _1)
					)
			);

			tlog(1,"Binds were (%d):\n", bindings_of_all_args->size());			

			foreach(hpair phh, *bindings_of_all_args)
			{
				printTree(phh.first,0,3);
				cprintf(3,"=>");
				printTree(phh.second,0,3);
			}			

			getc(stdin);getc(stdin);

			return false;
		}
		else
		{
			tlog(2,"::compute() resulted in: \n");
			printTree(v2h(new_result.value),0,2);
			printSubsts(new_result,2);

			return true;
		}
	}

    /// Completely and exhaustively expand tree to the bottom.
	template<typename VectorT, typename SetT, typename VectorOfSetsIterT>
	void WithLog_expandVectorSet(const std::vector<Btr<SetT> >& child_results,
								 set<VectorT>& argVectorSet) const
	{
		tlog(3, 	"expandVectorSet(child_results, argVectorSet)...\n");

		expandVectorSet<VectorT, 
						VectorOfSetsIterT,
						insert_iterator<set<VectorT> > >(
							child_results.begin(),
							child_results.end(),
							inserter(argVectorSet, argVectorSet.begin()));

		tlog(2, 	"RESULT FROM expandVectorSet(child_results, argVectorSet) ((%d,%d), %d,%d)...\n", (child_results.size()>0?child_results[0]->size():0), 
			(child_results.size()>1?child_results[1]->size():0), argVectorSet.size(),	(argVectorSet.empty() ? 0 : argVectorSet.begin()->size()) );	

		if (!child_results.empty())
		{ string bc = string("Arg Expansion results:\n");
		foreach(const VectorT& vbv, argVectorSet)
			if (!vbv.empty())
			{
				bc += "(";
				foreach(VtreeProvider* bv, vbv)
					bc += i2str((int)v2h(*bv->getVtree().begin()).value()) + " ";
				bc += ")\n";
			}
			tlog(1, bc.c_str());
		}
	}
	
	/// Add the new result from a Generator, and spawn new subtrees when necessary
	/// according to the variable bindings that accompany the new result
	void addDirectResult(Btr<set<BoundVertex> > directResult, spawn_mode spawning);

	/// Create children and do some (probably obsolete) pre-binding checks.
	bool expandRule(Rule *rule, int target_i, BBvtree arg,
		Btr<bindingsT> bindings, spawn_mode spawning);
	
	void clearResults();
	
	/// Heuristics helper value. See implementation for details.
	float my_solution_space() const;
	int number_of_free_variables_in_target() const;
	
	/// If target is a Handle, push it to the result set and replace
	/// the target with its virtual counter-part.
	void ForceTargetVirtual(spawn_mode spawning);
	
	/// Fitness-for-being-selected-for-expansion-next.
	float fitness() const;
	
	/// If inserting the rule invocation node in the subtree obeys our policy
	static bool obeysSubtreePolicy(Rule *new_rule, meta arg);

	/// If inserting the rule invocation node in the expansion pool obeys our policy
	static bool obeysPoolPolicy(Rule *new_rule, meta arg);

	/// Find if we already have a BITNode like this such that we can re-use it by some
	/// template_bindings which are returned to the caller.
	void findTemplateBIT(BITNode* new_node, BITNode*& template_node, bindingsT& template_binds) const;

	/// Try to clone this BITNode (under its existing parents) with a new binding applied.
	/// The cloning may fail if the result causes the arguments of the associated Rule to
	/// go invalid, but I think the check is not really made rigorously here.
	void tryClone(hpair binding) const;

	/// Find the fittest BITNode for expansion. \todo This method should be moved to the root class.
	void findFittest(BITNode*& bisse, float& best_fitness);

public:
	/// Set of possible inputs for each index of the input vectors of the Rule
	/// associated with this node. Contains the BITNodes, as opposed to the
    /// results as in direct_results
	vector<set<ParametrizedBITNode> > children;

	BITNode();
	BITNode(	BITNodeRoot* _root,
						BITNode* _parent,
						unsigned int _depth,
						unsigned int _parent_arg_i,
						meta _target,
						Rule *_rule,
						const Rule::MPs& _args,
						const vtreeset& _target_chain,
						Btr<bindingsT> _pre_bindings = Btr<bindingsT>(new bindingsT),
						spawn_mode spawning = NO_SIBLING_SPAWNING,
						bool _create = true);
	virtual ~BITNode();

	/// After construction, the object is comparable to others by
	/// using eq() methods. After Create(), it becomes fully usable.
	void create();	

	/// Use for debugging
	void loopCheck() const;
	
	/// Use for debugging
	int totalChildren() const;
	
	/// Use for debugging
	bool hasAncestor(const BITNode* const _p) const;

	bool isComputable() const { return rule->isComputable(); }

	const vector<set<VtreeProvider*> >& getEvalResults() const { return eval_results; }
	const set<parent_link<BITNode> >& getParents() const { return parents; }
	
	/// Look for a node equal to an existing dummy node
	BITNode* findNode(BITNode* new_child) const;

	/// Look for a node based on certain defining characteristics
	BITNode* findNode(Rule* new_rule, meta _target, const Rule::MPs& rule_args, const bindingsT& new_bindings) const;

	/// helpers
	bool eq(BITNode* rhs) const;
	bool eq(Rule* r,  const Rule::MPs& _args, meta _target, const bindingsT& _pre_bindings) const;

	/// Expand whole tree level. Typically not called externally
	void expandNextLevel();
	
	/// Typically not called externally
	void expandFittest();

	// Printing utilities
	int tlog(int debugLevel, const char *format, ...) const;
	
	void printChildrenSizes() const;
	void print(int loglevel=0, bool compact=false, Btr<set<BITNode*> > UsedBITNodes = Btr<set<BITNode*> >()) const;
	void printFitnessPool();
	void printTarget() const;
	void printArgs() const;
	void printResults() const; 
};

/**
The root of a Backward Inference proof Tree
The BITNodeRoot class works in the following way:
1. if the target of the query / inference is either a VariableScopeLink or ForAllLink (they have slightly different math), then BITNodeRoot::infer will carry out the unification steps and find all fitting results, but return the Handle to the single atom such as ForAllLink [$x] (ImplicationLink (isBoy $x) (isDumb $x)). It's TV can be retrieved from the atom table.
2. If the target is an atom of different type, then BITNodeRoot::infer will return the set of atoms that satisfy the target description. Eg. if the target is
ImplicationLink (isBoy $x) (isDumb $x), then it will return all ImpLinks that can be bound to this formula.
*/

class BITNodeRoot : public BITNode
{
public:

	~BITNodeRoot();

	set<BITNode*> used_nodes;

	// The class assumes ownership of the RuleProvider

	BITNodeRoot(meta _target, RuleProvider* _rp);
	
/**
	Basic usage: 
	Btr<BITNodeRoot> state = reasoning::PLNEvaluator::BIT_evaluate(reasoning::InferenceTaskParameters(NULL,target));
or you can create the BITNodeRoot directly.

	state->infer(ProofResources, minConfidenceForAlternativeAction, minConfidenceForAction);

	\param Resources = the max nr of inference steps. The method returns by ref the nr of actual steps taken.
	\param minConfidenceForStorage = the conf. threshold for storing an inference result
	\param minConfidenceForAbort = the conf. threshold for finishing the inference once "good enough" result found
*/
	const set<VtreeProvider*>& infer(int& resources,
            float minConfidenceForStorage = 0.000001f,
            float minConfidenceForAbort = 1.00f);

	/// Manual evaluation. Should not be needed anymore.

	Btr<set<BoundVertex> > evaluate(set<const BITNode*>* chain = NULL) const;

	/// Apply either ForAllRule or VariableScopeRule, depending on the _resultT type.
	BoundVertex Generalize(Btr<set<BoundVertex> >, Type _resultT) const;
//	BoundVertex Generalize(const set<BoundVertex>&, Type _resultT) const;

	/// Extraction of an actionable plan from the proof tree of the atom with Handle h.
	void extract_plan(Handle h) const;
	void extract_plan(Handle h, unsigned int level, vtree& do_template, vector<Handle>& plan) const;

	// Statistics
	
//	map<Handle,vector<Handle> > inferred_from;
//	map<Handle,Rule*> inferred_with;
	map<Handle,BITNode*> hsource;
	long InferenceNodes;

	void printTrail(Handle h) const;

	void print_users(BITNode* b);
	void print_parents(BITNode* b);

protected:
	friend struct not_owned_var;
	typedef list<BITNode*> exec_poolT;
	exec_poolT exec_pool;

	map<Vertex, set<BITNode*> > varOwner;

	std::map<Rule*, float> priority;

	Rule::MPs dummy_args;
	
    /// "Users[b]" list is maintained in order to know which BITNodes directly
    /// or indirectly use (ie. Have as a subtree, with possibly some
    /// inheritance-bindings) BITNode b. This information is used (only) to
    /// prevent circularity: a BITNode cannot indirectly or directly use itself.
	map<BITNode*, set<BITNode*> > users;

	/// It's too slow to sort the pool after every insertion. Otherwise we'd do this:
	/// typedef set<BITNode*, BITNode_fitness_comp> exec_poolT;

	bool exec_pool_sorted;
	set<BITNode*> BITNodeTemplates;

	RuleProvider* rp;

	/// 0 = no, other = the result link type
	Type post_generalize_type;
	
	// Methods

	bool spawns(const bindingsT& bindings) const;
	void spawn(Btr<bindingsT> bindings);

	BITNode* CreateChild(int my_rule_arg_i, Rule* new_rule, const Rule::MPs& rule_args, 
						BBvtree arg, const bindingsT& bindings,spawn_mode spawning);
						
	void printTrail(Handle h, unsigned int level) const;

	friend class BITNode;
};

/// A BITNode results of which are produced by (slow) manual evaluation
/// Obsolete but works. Contains interesting stats class, although the RuleApp
/// can generate richer statistics now.
class ExplicitlyEvaluatedBITNode : public BITNode
{
public:
	/// TODO: Complete implementation
	BoundVertex evaluate1(int index);
	
	Btr<set<BoundVertex> > evaluate(set<const BITNode*>* chain = NULL) const;

	/// Use for brute force checking of whether the tree can be evaluated (but do not really evaluate).
	/// TODO: This may no longer be up-2-date with the real evaluate()
	Btr<set<BoundVertex> > pseudo_evaluate() const;
};

	template<typename T>
	struct nofilter
	{
		T filteredBegin, filteredEnd;
		nofilter() {}
		nofilter(T begin, T end) : filteredBegin(begin), filteredEnd(end) {}
		void create(T begin, T end) {
			filteredBegin = begin;
			filteredEnd = end;
		}
	};
	template<typename T, typename T2>
	struct triviality_filter
	{
		T filteredBegin, filteredEnd;
		T2 filtered;
		triviality_filter() {}
		triviality_filter(T begin, T end) { Create(begin, end); }
		void create(T begin, T end) {
			for(T i = begin; i != end; ++i)
				if (i->first->isComputable())
					filtered.insert(*i);

//			remove_copy_if(begin, end, inserter(filtered, filtered.begin()), mem_fun(&T2::IsComputable));
			filteredBegin	= filtered.begin();
			filteredEnd		= filtered.end();
//			for (T i = begin; i != end; ++i)
//				(*begin)->rule->IsComputable
		}
	};

	/*
		The stats class is for recording statistics pertaining to BITNodes.
		This approach may be obsoleted by an approach which uses RuleApp.
		Such an approach doesn't exist yet tho (at May 27, 2007).
	*/

	class stats : public Singleton<stats>
	{
		friend class Singleton<stats>;
		stats() {}
		
	public:
		//Store the result set of each BITNode for later analysis.
		typedef map<BITNode*, set<Vertex> > ITN2atomT;
		typedef map<BITNode*, set<Vertex> >::iterator ITN2atomIteratorT;
		typedef map<BITNode*, set<Vertex> >::const_iterator ITN2atomIteratorConstT;
		typedef nofilter<ITN2atomIteratorConstT> nofilterT;
		typedef triviality_filter<ITN2atomIteratorConstT, ITN2atomT> triviality_filterT;

		ITN2atomT ITN2atom;
		template<typename filterT> // = nofilter<ITN2atomIteratorT> >
		void print(filterT filter = nofilterT()) const
		{

            puts("stats::print()\n");
            filter.create(ITN2atom.begin(), ITN2atom.end());

            for (map<BITNode*, set<Vertex> >::const_iterator	i = filter.filteredBegin;
                                                                i!= filter.filteredEnd; i++)
            {
    //			printf("[%d]: ", (int)i->first);
                i->first->print(-10, true);
                foreach(Vertex v, i->second)
                {
                    printf("%lu\n", v2h(v).value());
                    NMPrinter(NMP_BRACKETED|NMP_TYPE_NAME |NMP_NODE_NAME|NMP_NODE_TYPE_NAME|NMP_TRUTH_VALUE|NMP_PRINT_TO_FILE, -10).print(v2h(v));
                }
                printf("\n");
            }
            puts("---\n");
		}
	};

}

#endif
