#ifndef _TheoremProver_Ref_h_
#define _TheoremProver_Ref_h_

#include <Core/Core.h>
using namespace Upp;

namespace RefCore {

#if defined(VERBOSE_REF) && defined(flagDEBUG)
	#define REFLOG(x) LOG(x)
#else
	#define REFLOG(x)
#endif

//#ifdef flagDEBUG
extern int __ref_id_counter;
extern int __ref_break_id;
extern int __ref_break_id_dec;
extern int __ref_break_id_dtor;
inline void BreakRefId(int id) {__ref_break_id = id;}
inline void BreakRefIdDec(int id) {__ref_break_id_dec = id;}
inline void BreakRefIdDtor(int id) {__ref_break_id_dtor = id;}
//#endif

class RefContext;

class RefBase {
	RefContext* ctx;
	int refs;
	int id;
	
protected:
	friend class RefContext;
	
	//#ifdef flagDEBUG
	void Renew();
	//#endif
	
	void PrepareForcedDelete() {ctx = NULL; refs = 0; id = -1;}
	
public:

	RefBase(RefContext* ctx);
	virtual ~RefBase();
	RefContext& GetContext() {return *ctx;}
	RefBase& Inc();
	RefBase& Dec();
	int GetRefs() const {return refs;}
	
};

template <class T>
class Ref : public Pte<T>, public RefBase {
	
	
protected:
	typedef Ref<T> RefT;
	
	
	
	//int in_heap_magic;
	#define IN_HEAP_MAGIC 0x16849245
	
public:
	
	/*void* operator new(size_t size) {
		void* ptr = ::operator new(size);
		Ref* ref_ptr = reinterpret_cast<Ref*>(ptr);
		ref_ptr->in_heap_magic = IN_HEAP_MAGIC;
		return ptr;
	}

	void* operator new(size_t size, const T* r) {
		void* ptr = ::operator new(size);
		Ref* ref_ptr = reinterpret_cast<Ref*>(ptr);
		ref_ptr->in_heap_magic = IN_HEAP_MAGIC;
		return ptr;
	}*/

	//bool IsInHeap() {return in_heap_magic == IN_HEAP_MAGIC;}
	
	Ref(RefContext* ctx) : RefBase(ctx) {
		//if (!IsInHeap()) refs++;
	}
	
	Ref(const Ref& ref) : RefBase(ref.ctx) {
		//if (!IsInHeap()) refs++;
	}
	
	Ref& operator=(const Ref& ref) {RefBase::Renew(); return *this;}
	
	
	
	
	
};

}

#endif
