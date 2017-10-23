#ifndef _TheoremProver_Var_h_
#define _TheoremProver_Var_h_

#include <Core/Core.h>
using namespace Upp;

namespace RefCore {
	
#ifdef VERBOSE_VAR
	#define VARLOG(x) LOG(x)
#else
	#define VARLOG(x)
#endif


struct NullRef : public Exc {
	NullRef(const String& s) : Exc(s) {}
};

//#ifdef flagDEBUG
extern int __var_id_counter;
extern int __var_break_id;
extern int __var_break_id_chk;
extern int __var_break_id_dtor;
extern bool __var_break_null_dtor;

inline void BreakNullVarDtor(bool b=true) {__var_break_null_dtor = true;}
inline void BreakVarId(int id) {__var_break_id = id;}
inline void BreakVarIdChk(int id) {__var_break_id_chk = id;}
inline void BreakVarIdDtor(int id) {__var_break_id_dtor = id;}
//#endif

template <class T>
class Var : Moveable<Var<T> > {
	
protected:
	typedef Var<T> VarT;
	
	#ifdef VAR_FORCEUNSAFE
	T* node;
	#else
	Ptr<T> node;
	#endif
	
	//#ifdef flagDEBUG
	void RenewId() {
		static SpinLock lock;
		lock.Enter();
		{
			__var_id_counter++;
			id = __var_id_counter;
			if (__var_break_id == id) {
				Panic("Breakpoint at id " + IntStr(id));
			}
			VARLOG(Format("Var renew id=%d", id));
		}
		lock.Leave();
	}
	inline void DebugCtor() {
		RenewId();
	}
	inline void Chk() const {
		//ASSERT(&*node != (T*)0x800000000000000);
		if (!node && __var_break_null_dtor) {
			Panic("Pointer is null. Id: " + IntStr(id));
		}
		if (__var_break_id_chk == id) {
			Panic("Breakpoint at id " + IntStr(id));
		}
	}
	inline void ChkDtor() {
		if (!node && __var_break_id_dtor == id) {
			Panic("Breakpoint at id " + IntStr(id));
		}
		VARLOG(Format("Var reset id=%d", id));
		id = 0;
	}
	int id;
	/*#else
	inline void DebugCtor() const {}
	inline void Chk() const {}
	inline void ChkDtor() const {}
	inline void RenewId() const {}
	#endif*/
	
	
	
public:

	template <class Type> Var(Type* src) {
		ASSERT(src != ((Type*)-1));
		DebugCtor();
		node = src;
		if (src) {
			ASSERT(node);
			node->Inc();
		}
	}
	
	template <class Type> Var(const Type& src) {
		DebugCtor();
		node = new T(src);
		node->Inc();
	}
	
	Var(const VarT& n) {
		DebugCtor();
		node = &*n.node;
		if (node) node->Inc();
	}
	
	Var() : node(0) {
		#ifdef flagDEBUG
		id = 0;
		#endif
		
	}
	
	~Var() {
		if (id) Chk();
		if (node) {
			T* n = &*node;
			node = 0;
			n->Dec();
			ChkDtor();
		}
	}
	
	bool IsEqual(const VarT& var) {return &*var == &*node;}
	bool IsEqualVar(const VarT& var) {return &*var == &*node;}
	
	uint32 GetHashValue() const {
		if (node) return node->GetHashValue();
		return 0;
	}
	
	VarT& operator=(const VarT& n) {
		Clear();
		ASSERT(&*n.node);
		node = &*n.node;
		node->Inc();
		RenewId();
		return *this;
	}
	
	VarT& operator<<(const VarT& n) {
		Clear();
		if (&*n.node) {
			node = &*n.node;
			node->Inc();
			RenewId();
		}
		return *this;
	}
	
	T* operator->() const {return node;}
	T& operator*() const {return *node;}
	
	template <class Type> Type* Get() const {return dynamic_cast<Type*>(&*node);}
	T* GetNode() const {return &*node;}
	
	void Clear() {
		if (node) {
			T* n = &*node;
			node = 0;
			n->Dec();
			ChkDtor();
		}
	}
	
	// NEVER operator bool() {return node;}
	bool Is() const {
		if (id) Chk();
		return &*node != 0;
	}
	
	bool operator==(const VarT& src) const {
		return src.GetHashValue() == GetHashValue();
	}
	
	bool operator!=(const VarT& src) const {
		return src.GetHashValue() != GetHashValue();
	}
	
	
};

template <class T>
class VarRef : Moveable<VarRef<T> >, public Var<T>, public Ref<Var<T> > {
	typedef Ref<Var<T> > RefVarT;
	typedef VarRef<T> VarRefT;
	typedef Var<T> VarT;
	
	bool protect;
	
public:
	VarRef(RefContext* ctx) : VarT((T*)0), protect(0), RefVarT(ctx) {}
	VarRef(const VarRefT& r) : VarT(r), protect(0), RefVarT(&r.GetContext()) {}
	VarRef(const VarT& r) : VarT(r), protect(0), RefVarT(&r->GetContext()) {}
	VarRef(T* ptr, RefContext* ctx) : VarT(ptr), protect(0), RefVarT(ctx) {}
	virtual ~VarRef() {ASSERT(!protect); }
	
	VarRef& operator=(const VarRef& r) {ASSERT(!protect); Var<T>::operator=(r); return *this;}
	
	void Protect() {protect = true;}
	
};

template <class T>
class RefVar : Moveable<RefVar<T> >/*, public Ref<RefVar<T> >*/ {
	typedef Ref<RefVar<T> > RefRefVarT;
	typedef RefVar<T> RefVarT;
	typedef VarRef<T> VarRefT;
	
	Var<VarRef<T> > node;
	bool protect;
	
public:
	RefVar(/*RefContext* ctx*/) : node((VarRefT*)0), protect(0) /*, RefRefVarT(ctx)*/ {}
	RefVar(const RefVarT& r) : node(r.node), protect(0) /*, RefRefVarT(&r.GetContext())*/ {}
	RefVar(T* ptr/*, RefContext* ctx*/) : node((VarRefT*)0), protect(0) /*, RefRefVarT(ctx)*/  {if (ptr) {node = new VarRef<T>(ptr);}}
	~RefVar() {ASSERT(!protect);}
	
	uint32 GetHashValue() const {if (Is()) return (**node).GetHashValue(); return 0;}
	void Clear() {ASSERT(!protect); node.Clear();}
	void ReplaceWith(const RefVarT& r) {(*node) = (*r.node);}
	void Protect() {if (node.Is()) node->Protect();}
	void ProtectThisContainer() {protect = true;}
	
	bool Is() const {return node.Is() && node->Is();}
	bool IsEqual(const RefVarT& var) {
		if (!Is() || !var.Is()) return false;
		return &**var.node == &**node;
	}
	T* operator->() const {if (!node.Is()) return 0; return &**node;}
	T& operator*() const {if (!node.Is()) return *(T*)0; return **node;}
	
	RefVarT& operator=(const RefVarT& n) {
		Clear();
		ASSERT(n.Is());
		node = n.node;
		return *this;
	}
	
	RefVarT& operator<<(const RefVarT& n) {
		Clear();
		if (n.Is()) {
			node = n.node;
		}
		return *this;
	}
	
	
	/*
	void* operator new(size_t size, const RefVarT& r) {
		void* ptr = ::operator new(size);
		RefVarT* ref_ptr = reinterpret_cast<RefVarT*>(ptr);
		ref_ptr->in_heap_magic = IN_HEAP_MAGIC;
		return ptr;
	}*/
};

}

#endif
