#include "TheoremProver.h"



namespace TheoremProver {

void GetPredicates(Node& n, Index<NodeVar>& preds) {
	Predicate* pred = dynamic_cast<Predicate*>(&n);
	if (pred) {
		NodeVar ref(pred);
		if (preds.Find(ref) == -1) preds.Add(ref);
	}
	int count = n.GetCount();
	for(int i = 0; i < count; i++) {
		GetPredicates(n[i], preds);
	}
}

String EvaluateLogicNode(NodeVar ref) {
	String out;
	
	Index<String> vars;
	Index<NodeVar> refs;
	GetPredicates(*ref, refs);
	for(int i = 0; i < refs.GetCount(); i++)
		vars.Add(refs[i]->GetName());
	
	String line;
	for(int j = 0; j < vars.GetCount(); j++) {
		out << vars[j] << "\t";
		line << "----";
	}
	out << "result\n";
	out << line << "------";
	
	int combs = 1 << vars.GetCount();
	for(int i = 0; i < combs; i++) {
		out << "\n";
		Index<String> tmp_vars;
		String result;
		for(int j = 0; j < vars.GetCount(); j++) {
			int mask = 1 << j;
			bool is_true = i & mask;
			if (is_true)
				tmp_vars.Add(vars[j]);
			result << IntStr(is_true * 1) << "\t";
		}
		bool result_bool = ref->Evaluate(tmp_vars);
		result << IntStr(result_bool * 1);
		out << result;
	}
	
	return out;
}

String EvaluateLogic(String str) {
	NodeVar n = Parse(str);
	if (n.Is()) return EvaluateLogicNode(n);
	return "";
}

NodeVar OrList(Index<NodeVar>& list) {
	if (!list.GetCount()) return NodeVar();
	NodeVar out = list[0];
	for(int i = 1; i < list.GetCount(); i++) {
		out = new Or(*out, *list[i]);
	}
	return out;
}

NodeVar AndList(Index<NodeVar>& list) {
	if (!list.GetCount()) return NodeVar();
	NodeVar out = list[0];
	for(int i = 1; i < list.GetCount(); i++) {
		out = new And(*out, *list[i]);
	}
	return out;
}

NodeVar GetTruthTableDNF(NodeVar ref) {
	Index<NodeVar> refs, not_refs;
	GetPredicates(*ref, refs);
	
	for(int i = 0; i < refs.GetCount(); i++)
		not_refs.Add(new Not(*refs[i]));
	
	Index<String> vars;
	for(int i = 0; i < refs.GetCount(); i++)
		vars.Add(refs[i]->GetName());
	
	Vector<Index<NodeVar> > or_list;
	
	int combs = 1 << vars.GetCount();
	for(int i = 0; i < combs; i++) {
		Index<String> tmp_vars;
		Index<NodeVar> tmp_refs;
		for(int j = 0; j < vars.GetCount(); j++) {
			int mask = 1 << j;
			bool is_true = i & mask;
			if (is_true) {
				tmp_vars.Add(vars[j]);
				tmp_refs.Add(refs[j]);
			} else {
				tmp_refs.Add(not_refs[j]);
			}
		}
		bool result_bool = ref->Evaluate(tmp_vars);
		if (result_bool)
			or_list.Add(tmp_refs);
	}
	if (!or_list.GetCount())
		return NodeVar();
	
	NodeVar out = AndList(or_list[0]);
	for(int i = 1; i < or_list.GetCount(); i++) {
		out = new Or(*out, *AndList(or_list[i]));
	}
	
	return out;
}

NodeVar GetTruthTableCNF(NodeVar ref) {
	Index<NodeVar> refs, not_refs;
	GetPredicates(*ref, refs);
	
	for(int i = 0; i < refs.GetCount(); i++)
		not_refs.Add(new Not(*refs[i]));
	
	Index<String> vars;
	for(int i = 0; i < refs.GetCount(); i++)
		vars.Add(refs[i]->GetName());
	
	Vector<Index<NodeVar> > and_not_list;
	
	int combs = 1 << vars.GetCount();
	for(int i = 0; i < combs; i++) {
		Index<String> tmp_vars;
		Index<NodeVar> tmp_refs;
		for(int j = 0; j < vars.GetCount(); j++) {
			int mask = 1 << j;
			bool is_true = i & mask;
			if (is_true) {
				tmp_vars.Add(vars[j]);
				tmp_refs.Add(not_refs[j]);
			}
			else
				tmp_refs.Add(refs[j]);
		}
		bool result_bool = ref->Evaluate(tmp_vars);
		if (!result_bool)
			and_not_list.Add(tmp_refs);
	}
	if (!and_not_list.GetCount())
		return NodeVar();
	
	NodeVar out = OrList(and_not_list[0]);
	for(int i = 1; i < and_not_list.GetCount(); i++) {
		out = new And(*out, *OrList(and_not_list[i]));
	}
	
	return out;
}


}
