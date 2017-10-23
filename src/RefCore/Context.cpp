#include "RefCore.h"

namespace RefCore {

RefContext& GetRefContext() {
	return Single<RefContext>();
}



RefContext::RefContext() : RefRefContext(0) {
	
}

RefContext::~RefContext() {
	FastClear();
}

void RefContext::Clear() {
	while (ptrs.GetCount()) {
		int pos = ptrs.GetCount()-1;
		RefBase* ref = ptrs[pos];
		ptrs.Remove(pos);
		ASSERT(ref != 0);
		ref->PrepareForcedDelete();
		delete ref;
	}
}

void RefContext::FastClear() {
	for(int i = ptrs.GetCount()-1; i >= 0; i--) {
		ptrs[i]->PrepareForcedDelete();
	}
	for(int i = ptrs.GetCount()-1; i >= 0; i--) {
		delete ptrs[i];
	}
	ptrs.Clear();
}

int RefContext::GetMemoryLinkCount() {
	return ptrs.GetCount();
}

RefBase* RefContext::GetMemoryLink(int i) {
	return ptrs[i];
}

void RefContext::AddRef(int id, RefBase* ref) {
	ASSERT(ref != (RefBase*)this);
	ptrs.Add(id, ref);
	ref->Inc();
}

void RefContext::ChangeRef(int old_id, int new_id) {
	int i = ptrs.Find(old_id);
	ASSERT(i >= 0);
	ptrs.SetKey(i, new_id);
}

void RefContext::RemoveRef(int id, RefBase* ref) {
	ASSERT(id >= 0);
	garbage.Add(id);
	if (garbage.GetCount() > 10000)
		FreeGarbage();
}

void RefContext::FreeGarbage() {
	Sort(garbage, StdLess<int>());
	Vector<int> garbage_pos;
	garbage_pos.SetCount(garbage.GetCount());
	int count = 0;
	for(int i = 0; i < garbage.GetCount(); i++) {
		int j = ptrs.Find(garbage[i]);
		if (j == -1) continue;
		garbage_pos[count] = j;
		count++;
	}
	garbage.Clear();
	garbage_pos.SetCount(count);
	Sort(garbage_pos, StdLess<int>());
	ptrs.Remove(garbage_pos);
}

}
