#ifndef _TheoremProver_Context_h_
#define _TheoremProver_Context_h_

#include "Ref.h"
#include "Var.h"

namespace RefCore {
	
/*
void MarkAllocated ( ScriptRef v );
void MarkAllocated ( ScriptLinkRef v );
void MarkDeallocated ( ScriptRef v );
void MarkDeallocated ( ScriptLinkRef v );
void ShowAllocated();

int GetMemoryVarCount();
int GetMemoryLinkCount();
ScriptRef GetMemoryVar(int i);
ScriptLinkRef GetMemoryLink(int i);
*/


class RefContext : public Ref<RefContext> {
	typedef Ref<RefContext> RefRefContext;
	
	VectorMap<int, RefBase*> ptrs;
	Vector<int> garbage;
	
public:
	RefContext();
	~RefContext();
	
	int GetMemoryLinkCount();
	RefBase* GetMemoryLink(int i);
	
	void AddRef(int id, RefBase* ref);
	void ChangeRef(int old_id, int new_id);
	void RemoveRef(int id, RefBase* ref);
	void FreeGarbage();
	void Clear();
	void FastClear();
	
};

RefContext& GetRefContext();

typedef Var<RefContext> RefContextVar;

}

#endif
