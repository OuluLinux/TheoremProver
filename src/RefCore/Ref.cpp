#include "RefCore.h"
#include "Context.h"

namespace RefCore {

RefBase::RefBase(RefContext* ctx) : refs(0), ctx(ctx) {
	id = 0;
	Renew();
	REFLOG(Format("RefBase new id=%d", id));
}

RefBase::~RefBase() {
	#ifdef flagDEBUG
	if (id == __ref_break_id_dtor) {
		Panic(Format("Breakpoint: ref dtor id=%d", id));
	}
	#endif
	
	ASSERT(refs == 0); // Fail here means, that
	// some Ref<> object is not created with new-operator.
	// RefBase inheriting classes may NOT be created in stack memory, because
	// destructor code-position in function has higher priority than reference-count!
	// However, some circular reference-decrease by virtual destructor might also be the problem.
	
	if (ctx)
		ctx->RemoveRef(id, this);
}

RefBase& RefBase::Inc() {
	refs++;
	REFLOG(Format("RefBase++ %d id=%d", refs, id));
	return *this;
}

RefBase& RefBase::Dec() {
	if (id == -1) return *this;
	ASSERT(refs > 0);
	refs--;
	REFLOG(Format("RefBase-- %d id=%d", refs, id));
	#ifdef flagDEBUG
	if (id && id == __ref_break_id_dec) {
		Panic(Format("Breakpoint: ref dec id=%d", id));
	}
	#endif
	if (refs == 0) {
		REFLOG(Format("RefBase delete id=%d", id));
		delete this;
	}
	return *this;
}

void RefBase::Renew() {
	int prev_id = id;
	
	static SpinLock lock;
	lock.Enter();
	{
		__ref_id_counter++;
		id = __ref_id_counter;
		if (id == __ref_break_id_dec) {
			Panic(Format("Breakpoint: ref id=%d", id));
		}
	}
	lock.Leave();
	
	if (ctx) {
		if (prev_id == 0)
			ctx->AddRef(id, this);
		else
			ctx->ChangeRef(prev_id, id);
	}
}

}
