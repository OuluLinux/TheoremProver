#include "TheoremProver.h"

namespace TheoremProver {

RefContext* GetContext() {
	static RefContext ctx;
	return &ctx;
}

NodeVar Node::GetDNF() {
	return this;
}

NodeVar Node::Replace(Node& old, Node& new_) {
	if (*this == old)
		return &new_;

	return this;
}

bool NodeVar::operator() (const NodeVar& a, const NodeVar& b) const {
	// NOTE: this sort allows the same result than in python, even when it is a little bit silly
	// TODO: right answer without silly sort. 
	/*int64 av = StrInt(Format("%d", (int64)a->GetHashValue()));
	int64 bv = StrInt(Format("%d", (int64)b->GetHashValue()));
	return av < bv;*/
	
	return a->GetTime() > b->GetTime();
}















}
