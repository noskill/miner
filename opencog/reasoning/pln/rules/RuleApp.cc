#include "../PLN.h"
#include "../AtomSpaceWrapper.h"
#include "RuleApp.h"

namespace reasoning
{
// Was:
const vtree& RuleApp::getVtree() const
{
    vt_result = vtree(compute().value);
	return vt_result;
}
/*const vtree& RuleApp::getVtree() const
{
    Vertex v = compute(NULL).value;
    Handle h = boost::get<Handle>(v);
	vt_result.set_head(h);
    //vt_result = make_vtree(h);
	return vt_result;
}*/

BoundVertex RuleApp::compute(const vector<Vertex>& h, Handle CX) const
{
	vector<VtreeProvider*>::iterator next_unused_arg;

	vector<VtreeProvider*> vs;
	Vertices_TO_VtreeProviders(h, back_inserter(vs));

	BoundVertex ret = compute(vs.begin(), vs.end(), next_unused_arg, CX);
	assert(next_unused_arg == vs.end()); // We don't allow args to remain unused ultimately.

	return ret;
}

BoundVertex RuleApp::compute(Handle CX) const
{
    vector<VtreeProvider*> dummy_vp;
    return compute(dummy_vp.end(), dummy_vp.end(), CX);
}

RuleApp::~RuleApp() {
    foreach(VtreeProvider* vtp, args)
        delete vtp;
}
RuleApp::RuleApp(//iAtomSpaceWrapper *_destTable,
        Rule *_root_rule)
        :	Rule(_root_rule->destTable, false, true, "Inference Pathway"),
         result((Handle)NULL), arg_changes_since_last_compute(true), root_rule(_root_rule)
{ 
    if (!_root_rule->hasFreeInputArity())
    { int i = 1; }
    //args.resize(_root_rule->getInputFilter().size());
    args.insert(args.begin(), 
        (!_root_rule->hasFreeInputArity()
        ? _root_rule->getInputFilter().size()
        : 50),
        (VtreeProviderWrapper*)NULL);
}
//const* VtreeProvider clone() const { assert(0); }

/// Takes ownership of the "arg"
/// false if arg was already bound. (And assert failure.)
bool RuleApp::Bind(int arg_i, VtreeProvider* arg) const
{
    if (args[arg_i])
    {
        VtreeProviderWrapper* vtw = dynamic_cast<VtreeProviderWrapper*>(args[arg_i]);

        Handle nh = Handle::UNDEFINED;
        if (!vtw || vtw->val != vtree(nh))
        {
            assert(0);
            return false;
        }
    }

    args[arg_i] = arg;
    arg_changes_since_last_compute = true;

    bool isvtree = (dynamic_cast<VtreeProviderWrapper*>(args[arg_i]) != /*(VtreeProviderWrapper*)*/NULL);
    bool isruleapp = (dynamic_cast<RuleApp*>(args[arg_i]) != /*(VtreeProviderWrapper*)*/NULL);
    assert(isvtree || isruleapp);
//		assert(dynamic_cast<VtreeProviderWrapper*>(ra) != /*(VtreeProviderWrapper*)*/NULL)

    return true;
}


/// false if arg was already bound.
bool RuleApp::Bind(vector<VtreeProvider*>::iterator ai, VtreeProvider* arg) const
{
    int i=0;
    
    for (vector<VtreeProvider*>::iterator	ai2 = args.begin(); ai2!= ai; ++i,++ai2)
        ;

    return Bind(i, arg);
}

// From Rule
set<Rule::MPs> RuleApp::o2iMetaExtra(meta outh, bool& overrideInputFilter) const
{
    const set<Rule::MPs> my_arg_targets_s = root_rule->o2iMeta(outh);
    assert(my_arg_targets_s.size()<2); //We can't deal with the more complex case in this kind of approach!
    if ( my_arg_targets_s.empty())
        return my_arg_targets_s;
    const Rule::MPs my_arg_targets(*my_arg_targets_s.begin());
    Rule::MPs::const_iterator ret_i = my_arg_targets.begin();
    Rule::MPs final_input_args;

    for (vector<VtreeProvider*>::const_iterator ai = args.begin();
            ai != args.end();
            ++ai, ++ret_i)
    {
        const RuleApp* ra;
        if ((ra = dynamic_cast<const RuleApp*>(*ai)) != NULL) //If the arg is a RuleApp, take its input vector
        {
            set<Rule::MPs> free_args_of_the_branch = ra->o2iMeta(*ret_i);
            assert(free_args_of_the_branch.size()<=1);
            if (!free_args_of_the_branch.empty())
                final_input_args.insert(final_input_args.end(),
                                        free_args_of_the_branch.begin()->begin(),
                                        free_args_of_the_branch.begin()->end());
        }
        else //If the arg is a non-fulfilled (ie. ==NULL) vtree, assume it from my own input vector (ret_i)
        {
            const VtreeProviderWrapper* vtw = dynamic_cast<const VtreeProviderWrapper*>(*ai);
            Handle nh=Handle::UNDEFINED;
            if (vtw->val == vtree(nh))
                final_input_args.push_back(*ret_i);
        }
    }

    return makeSingletonSet(final_input_args);
}	



}
