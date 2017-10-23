#ifndef _TheoremProver_Language_h_
#define _TheoremProver_Language_h_

#include <RefCore/RefCore.h>


namespace TheoremProver {
using namespace RefCore;
using RefCore::Ref;

class NodeVar;
class UnificationTerm;

RefContext* GetContext();

class Node : public Ref<Node> {
	Node& operator=(const Node& n) {return *this;}
	Node(const Node& n) : Ref<Node>(TheoremProver::GetContext()) {}
	
protected:
	friend class NodeVar;
	
	String name;
	int time;
	
	
public:
	Node(const String& name) : name(name), time(0), Ref<Node>(TheoremProver::GetContext()) {}
	Node() : time(0), Ref<Node>(TheoremProver::GetContext()) {}
	virtual ~Node() {ASSERT(GetRefs() == 0);}
	
	
	String GetName() const {return name;}
	int GetTime() const {return time;}
	
	virtual Index<NodeVar> FreeVariables() {
		return Index<NodeVar>();
	}
	
	virtual Index<NodeVar> FreeUnificationTerms() {
		return Index<NodeVar>();
	}

	virtual bool operator==(Node& other) {
		return false;
	}

	virtual NodeVar Replace(Node& old, Node& new_);
	
	virtual void SetInstantiationTime(int time) {
		this->time = time;
	}
	
	virtual bool Occurs(UnificationTerm& unification_term) {
		return false;
	}
	
	virtual String ToString() const {return name;}
	virtual uint32 GetHashValue() const {return ToString().GetHashValue();}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("Node (%s:%d)", name, time);
		return out;
	}
	
	virtual bool Evaluate(const Index<String>& truth_assignment) const {return false;}
	
	virtual int GetCount() const {return 0;}
	virtual Node& operator[] (int i) {}
	
	virtual NodeVar GetDNF();
	
};

class NodeVar : public Var<Node>, Moveable<NodeVar> {
	
public:
	template <class T> NodeVar(const T& src) : Var<Node>(src) {}
	template <class T> NodeVar(T* src) : Var<Node>(src) {}
	NodeVar(const NodeVar& n) : Var<Node>((const Var<Node>&)n) {}
	NodeVar() {}
	~NodeVar() {}
	
	bool operator() (const NodeVar& a, const NodeVar& b) const;
	bool Evaluate(const Index<String>& truth_assignment) const {if (node) return node->Evaluate(truth_assignment); return false;}
	String AsString() {if (!node) return ""; return node->AsString();}
	String ToString() {if (!node) return ""; return node->ToString();}
	NodeVar GetDNF() {if (!node) return NodeVar(); return node->GetDNF();}
	
	NodeVar& operator=(const NodeVar& n) {Var<Node>::operator=(n); return *this;}
	
};

class UnificationTerm;

// Terms

class Variable : public Node {
	
public:
	Variable(const String& name) : Node(name) {}

	virtual Index<NodeVar> FreeVariables() {
		Index<NodeVar> out;
		out.Add(NodeVar(this));
		return out;
	}

	virtual bool operator==(Node& other) {
		if (!dynamic_cast<Variable*>(&other))
			return false;

		return GetName() == other.GetName();
	}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("Variable (%d:%d) %s", GetTime(), GetRefs(), GetName());
		return out;
	}
	
	
};

class UnificationTerm : public Node {
	
public:
	UnificationTerm(const String& name) : Node(name) {
		
	}

	virtual Index<NodeVar> FreeUnificationTerms() {
		Index<NodeVar> out;
		out.Add(NodeVar(this));
		return out;
	}

	virtual bool Occurs(UnificationTerm& unification_term) {
		return *this == unification_term;
	}

	virtual bool operator==(Node& other) {
		if (!dynamic_cast<UnificationTerm*>(&other))
			return false;

		return GetName() == other.GetName();
	}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("UnificationTerm (%d:%d) %s", GetTime(), GetRefs(), GetName());
		return out;
	}
	
};

class Function : public Node {
	
protected:
	friend ArrayMap<NodeVar, NodeVar> Unify(Node& term_a, Node& term_b);
	friend void TypecheckTerm ( Node& term );
	
	Index<NodeVar> terms;
	
public:
	Function(const String& name, const Index<NodeVar>& terms) : Node(name) {
		this->terms <<= terms;
	}
	
	virtual int GetCount() const {return terms.GetCount();}
	virtual Node& operator[] (int i) {return *terms[i];}
	
	virtual Index<NodeVar> FreeVariables() {
		if (terms.GetCount() == 0)
			return Index<NodeVar>();
		
		Index<NodeVar> out;
		for(int i = 0; i < terms.GetCount(); i++)
			Append(out, terms[i]->FreeVariables());
		return out;
		//return reduce((lambda x, y: x | y), [term.FreeVariables() for term in terms]);
	}

	virtual Index<NodeVar> FreeUnificationTerms() {
		if (terms.GetCount() == 0)
			return Index<NodeVar>();

		Index<NodeVar> out;
		for(int i = 0; i < terms.GetCount(); i++)
			Append(out, terms[i]->FreeUnificationTerms());
		return out;
		//return reduce((lambda x, y: x | y), [term.FreeUnificationTerms() for term in terms]);
	}

	virtual NodeVar Replace(Node& old, Node& new_) {
		if (*this == old)
			return &new_;
		
		Index<NodeVar> out;
		for(int i = 0; i < terms.GetCount(); i++) {
			out.Add(terms[i]->Replace(old, new_));
		}
		return NodeVar(new Function(GetName(), out));
		//return Function(GetName(), [term.Replace(old, new_) for term in terms]);
	}

	virtual bool Occurs(UnificationTerm& unification_term) {
		for(int i = 0; i < terms.GetCount(); i++) {
			if (terms[i]->Occurs(unification_term))
				return true;
		}
		return false;
		//return any([term.Occurs(unification_term) for term in terms]);
	}

	virtual void SetInstantiationTime(int time) {
		Node::SetInstantiationTime(time);
		
		for(int i = 0; i < terms.GetCount(); i++)
			terms[i]->SetInstantiationTime(time);
		//for (term in terms) term.SetInstantiationTime(time);
	}

	virtual bool operator==(Node& other) {
		Function* fn = dynamic_cast<Function*>(&other);
		if (!fn)
			return false;

		if (GetName() != other.GetName())
			return false;

		if (terms.GetCount() != fn->terms.GetCount())
			return false;
		
		if (!terms.GetCount()) return false;
		for(int i = 0; i < terms.GetCount(); i++) {
			if (!(*terms[i] == *fn->terms[i]))
				return false;
		}
		return true;
		//return all([terms[i] == other.terms[i] for i in range(terms.GetCount())]);
	}

	virtual String ToString() const {
		if (terms.GetCount() == 0)
			return GetName();

		String s = GetName() + "(";
		for(int i = 0; i < terms.GetCount(); i++) {
			if (i) s += ", ";
			s += terms[i]->ToString();
		}
		s += ")";
		return s;
	}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("Function (%d:%d) %s", GetTime(), GetRefs(), GetName());
		for(int i = 0; i < terms.GetCount(); i++) {
			if (i) out << "\n";
			out << terms[i]->AsString(ident+1);
		}
		return out;
	}
	
	virtual NodeVar GetDNF() {
		Index<NodeVar> dnf_terms;
		for(int i = 0; i < terms.GetCount(); i++)
			dnf_terms.Add(terms[i]->GetDNF());
		return new Function(GetName(), dnf_terms);
	}
	
};

// Formulae
class Predicate : public Node {
	
protected:
	friend ArrayMap<NodeVar, NodeVar> Unify(Node& term_a, Node& term_b);
	friend void TypecheckFormula ( Node& formula );
	friend bool ProveSequent(Node& sequent);
	
	
	Index<NodeVar> terms;
	
public:

	Predicate(const String& name, const Index<NodeVar>& terms) : Node(name) {
		this->terms <<= terms;
		for(int i = 0; i < terms.GetCount(); i++) {ASSERT(terms[i].GetNode());}
	}
	
	virtual int GetCount() const {return terms.GetCount();}
	virtual Node& operator[] (int i) {return *terms[i];}
	
	virtual Index<NodeVar> FreeVariables() {
		if (terms.GetCount() == 0)
			return Index<NodeVar>();

		Index<NodeVar> out;
		for(int i = 0; i < terms.GetCount(); i++)
			Append(out, terms[i]->FreeVariables());
		return out;
		//return reduce((lambda x, y: x | y), [term.FreeVariables() for term in terms]);
	}

	virtual Index<NodeVar> FreeUnificationTerms() {
		if (terms.GetCount() == 0)
			return Index<NodeVar>();

		Index<NodeVar> out;
		for(int i = 0; i < terms.GetCount(); i++)
			Append(out, terms[i]->FreeUnificationTerms());
		return out;
		//return reduce((lambda x, y: x | y), [term.FreeUnificationTerms() for term in terms]);
	}

	virtual NodeVar Replace(Node& old, Node& new_) {
		if (*this == old)
			return &new_;
		
		Index<NodeVar> out;
		for(int i = 0; i < terms.GetCount(); i++) {
			out.Add(terms[i]->Replace(old, new_));
		}
		return NodeVar(new Predicate(GetName(), out));
	}

	virtual bool Occurs(UnificationTerm& unification_term) {
		for(int i = 0; i < terms.GetCount(); i++) {
			if (terms[i]->Occurs(unification_term))
				return true;
		}
		return false;
		//return any([term.Occurs(unification_term) for term in terms]);
	}

	virtual bool operator==(Node& other) {
		Predicate* pr = dynamic_cast<Predicate*>(&other);
		if (!pr)
			return false;

		if (GetName() != other.GetName())
			return false;

		if (terms.GetCount() != pr->terms.GetCount())
			return false;
		
		if (!terms.GetCount()) return false;
		for(int i = 0; i < terms.GetCount(); i++) {
			if (!(*terms[i] == *pr->terms[i]))
				return false;
		}
		return true;
		//return all([terms[i] == other.terms[i] for i in range(terms.GetCount())]);
	}

	virtual void SetInstantiationTime(int time) {
		Node::SetInstantiationTime(time);
		for (int i = 0; i < terms.GetCount(); i++)
			terms[i]->SetInstantiationTime(time);
	}

	virtual String ToString() const {
		if (terms.GetCount() == 0)
			return GetName();
		String s = GetName() + "(";
		for(int i = 0; i < terms.GetCount(); i++) {
			if (i) s += ", ";
			s += terms[i]->ToString();
		}
		s += ")";
		return s;
		//return name + "(" + ", ".join([term.ToString() for term in terms]) + ")";
	}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("Predicate (%d:%d) %s", GetTime(), GetRefs(), GetName());
		for(int i = 0; i < terms.GetCount(); i++) {
			if (i) out << "\n";
			out << terms[i]->AsString(ident+1);
		}
		return out;
	}
	
	virtual bool Evaluate(const Index<String>& truth_assignment) const {
		return truth_assignment.Find(GetName()) != -1;
	}
	
	virtual NodeVar GetDNF() {
		Index<NodeVar> dnf_terms;
		for(int i = 0; i < terms.GetCount(); i++)
			dnf_terms.Add(terms[i]->GetDNF());
		return new Predicate(GetName(), dnf_terms);
	}
	
};


class Not : public Node {
	
protected:
	friend void TypecheckFormula ( Node& formula );
	friend bool ProveSequent(Node& sequent);
	
	NodeVar formula;

public:

	Not(Node& formula) : formula(&formula) {
		
	}
	
	virtual int GetCount() const {return 1;}
	virtual Node& operator[] (int i) {if (i == 0) return *formula;}
	
	virtual Index<NodeVar> FreeVariables() {
		return formula->FreeVariables();
	}

	virtual Index<NodeVar> FreeUnificationTerms() {
		return formula->FreeUnificationTerms();
	}

	virtual NodeVar Replace(Node& old, Node& new_) {
		if (*this == old)
			return &new_;

		return NodeVar(new Not(*formula->Replace(old, new_)));
	}

	virtual bool Occurs(UnificationTerm& unification_term) {
		return formula->Occurs(unification_term);
	}

	virtual void SetInstantiationTime(int time) {
		Node::SetInstantiationTime(time);
		formula->SetInstantiationTime(time);
	}

	virtual bool operator==(Node& other) {
		Not* no = dynamic_cast<Not*>(&other);
		
		if (!no)
			return false;

		return *formula == *no->formula;
	}

	virtual String ToString() const {
		return "¬" + formula->ToString();
	}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("Not (%d:%d) %s", GetTime(), GetRefs(), GetName());
		out << "\n" << formula->AsString(ident+1);
		return out;
	}
	
	virtual bool Evaluate(const Index<String>& truth_assignment) const {
		return !formula->Evaluate(truth_assignment);
	}
	
	
	virtual NodeVar GetDNF() {
		return new Not(*formula->GetDNF());
	}
	
};

class And : public Node {
	
protected:
	friend void TypecheckFormula ( Node& formula );
	friend bool ProveSequent(Node& sequent);
	
	NodeVar formula_a, formula_b;
	
public:

	And(Node& formula_a, Node& formula_b) : formula_a(&formula_a), formula_b(&formula_b) {
		
	}
	
	virtual int GetCount() const {return 2;}
	virtual Node& operator[] (int i) {if (i == 0) return *formula_a; if (i == 1) return *formula_b;}
	
	virtual Index<NodeVar> FreeVariables() {
		Index<NodeVar> out;
		Append(out, formula_a->FreeVariables());
		Append(out, formula_b->FreeVariables());
		return out;
	}

	virtual Index<NodeVar> FreeUnificationTerms() {
		Index<NodeVar> out;
		Append(out, formula_a->FreeUnificationTerms());
		Append(out, formula_b->FreeUnificationTerms());
		return out;
	}

	virtual NodeVar Replace(Node& old, Node& new_) {
		if (*this == old)
			return &new_;

		return NodeVar(new And(
			*formula_a->Replace(old, new_),
			*formula_b->Replace(old, new_)
		));
	}

	virtual bool Occurs(UnificationTerm& unification_term) {
		return formula_a->Occurs(unification_term) || formula_b->Occurs(unification_term);
	}

	virtual void SetInstantiationTime(int time) {
		Node::SetInstantiationTime(time);
		formula_a->SetInstantiationTime(time);
		formula_b->SetInstantiationTime(time);
	}

	virtual bool operator==(Node& other) {
		And* an = dynamic_cast<And*>(&other);
		
		if (!an)
			return false;

		return *formula_a == *an->formula_a && *formula_b == *an->formula_b;
	}

	virtual String ToString() const {
		return Format("(%s ∧ %s)", formula_a->ToString(), formula_b->ToString());
	}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("And (%d:%d) %s", GetTime(), GetRefs(), GetName());
		out << "\n" << formula_a->AsString(ident+1);
		out << "\n" << formula_b->AsString(ident+1);
		return out;
	}
	
	virtual bool Evaluate(const Index<String>& truth_assignment) const {
		return formula_a->Evaluate(truth_assignment) && formula_b->Evaluate(truth_assignment);
	}
	
	virtual NodeVar GetDNF() {
		return new And(*formula_a->GetDNF(), *formula_b->GetDNF());
	}
	
};

class Or : public Node {
	
protected:
	friend void TypecheckFormula ( Node& formula );
	friend bool ProveSequent(Node& sequent);
	
	NodeVar formula_a, formula_b;
	
public:

	Or(Node& formula_a, Node& formula_b) : formula_a(&formula_a), formula_b(&formula_b) {
		
	}
	
	virtual int GetCount() const {return 2;}
	virtual Node& operator[] (int i) {if (i == 0) return *formula_a; if (i == 1) return *formula_b;}
	
	virtual Index<NodeVar> FreeVariables() {
		Index<NodeVar> out;
		Append(out, formula_a->FreeVariables());
		Append(out, formula_b->FreeVariables());
		return out;
	}

	virtual Index<NodeVar> FreeUnificationTerms() {
		Index<NodeVar> out;
		Append(out, formula_a->FreeUnificationTerms());
		Append(out, formula_b->FreeUnificationTerms());
		return out;
	}

	virtual NodeVar Replace(Node& old, Node& new_) {
		if (*this == old)
			return &new_;

		return NodeVar(new Or(
			*formula_a->Replace(old, new_),
			*formula_b->Replace(old, new_)
		));
	}

	virtual bool Occurs(UnificationTerm& unification_term) {
		return formula_a->Occurs(unification_term) || formula_b->Occurs(unification_term);
	}

	virtual void SetInstantiationTime(int time) {
		Node::SetInstantiationTime(time);
		formula_a->SetInstantiationTime(time);
		formula_b->SetInstantiationTime(time);
	}

	virtual bool operator==(Node& other) {
		Or* or_ = dynamic_cast<Or*>(&other);
		
		if (!or_)
			return false;

		return *formula_a == *or_->formula_a && *formula_b == *or_->formula_b;
	}

	virtual String ToString() const {
		return Format("(%s ∨ %s)", formula_a->ToString(), formula_b->ToString());
	}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("Or (%d:%d) %s", GetTime(), GetRefs(), GetName());
		out << "\n" << formula_a->AsString(ident+1);
		out << "\n" << formula_b->AsString(ident+1);
		return out;
	}
	
	virtual bool Evaluate(const Index<String>& truth_assignment) const {
		return formula_a->Evaluate(truth_assignment) || formula_b->Evaluate(truth_assignment);
	}
	
	virtual NodeVar GetDNF() {
		return new Or(*formula_a->GetDNF(), *formula_b->GetDNF());
	}
	
};

class Implies : public Node {
	
protected:
	friend void TypecheckFormula ( Node& formula );
	friend bool ProveSequent(Node& sequent);
	
	NodeVar formula_a, formula_b;
	
public:

	Implies(Node& formula_a, Node& formula_b) : formula_a(&formula_a), formula_b(&formula_b) {
		
	}
	
	virtual int GetCount() const {return 2;}
	virtual Node& operator[] (int i) {if (i == 0) return *formula_a; if (i == 1) return *formula_b;}
	
	virtual Index<NodeVar> FreeVariables() {
		Index<NodeVar> out;
		Append(out, formula_a->FreeVariables());
		Append(out, formula_b->FreeVariables());
		return out;
	}

	virtual Index<NodeVar> FreeUnificationTerms() {
		Index<NodeVar> out;
		Append(out, formula_a->FreeUnificationTerms());
		Append(out, formula_b->FreeUnificationTerms());
		return out;
	}

	virtual NodeVar Replace(Node& old, Node& new_) {
		if (*this == old)
			return &new_;

		return NodeVar(new Implies(
			*formula_a->Replace(old, new_),
			*formula_b->Replace(old, new_)
		));
	}

	virtual bool Occurs(UnificationTerm& unification_term) {
		return formula_a->Occurs(unification_term) || formula_b->Occurs(unification_term);
	}

	virtual void SetInstantiationTime(int time) {
		Node::SetInstantiationTime(time);
		formula_a->SetInstantiationTime(time);
		formula_b->SetInstantiationTime(time);
	}

	virtual bool operator==(Node& other) {
		Implies* implies = dynamic_cast<Implies*>(&other);
		if (!implies)
			return false;

		return *formula_a == *implies->formula_a && *formula_b == *implies->formula_b;
	}

	virtual String ToString() const {
		return Format("(%s → %s)", formula_a->ToString(), formula_b->ToString());
	}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("Implies (%d:%d) %s", GetTime(), GetRefs(), GetName());
		out << "\n" << formula_a->AsString(ident+1);
		out << "\n" << formula_b->AsString(ident+1);
		return out;
	}
	
	virtual bool Evaluate(const Index<String>& truth_assignment) const {
		return !formula_a->Evaluate(truth_assignment) || formula_b->Evaluate(truth_assignment);
	}
	
	virtual NodeVar GetDNF() {
		NodeVar not_a = new Not(*formula_a->GetDNF());
		NodeVar or_ = new Or(*not_a, *formula_b->GetDNF());
		return or_;
	}
	
};

class ForAll : public Node {
	
protected:
	friend void TypecheckFormula ( Node& formula );
	friend bool ProveSequent(Node& sequent);
	
	NodeVar variable, formula;
	
public:

	ForAll(Node& variable, Node& formula) : variable(&variable), formula(&formula) {
		
	}
	
	virtual int GetCount() const {return 2;}
	virtual Node& operator[] (int i) {if (i == 0) return *variable; if (i == 1) return *formula;}
	
	virtual Index<NodeVar> FreeVariables() {
		Index<NodeVar> out = formula->FreeVariables();
		int i = out.Find(variable);
		if (i != -1) out.Remove(i);
		return out;
	}

	virtual Index<NodeVar> FreeUnificationTerms() {
		return formula->FreeUnificationTerms();
	}

	virtual NodeVar Replace(Node& old, Node& new_) {
		if (*this == old)
			return &new_;

		return NodeVar(new ForAll(
		   *variable->Replace(old, new_),
		   *formula->Replace(old, new_)
		));
	}

	virtual bool Occurs(UnificationTerm& unification_term) {
		return formula->Occurs(unification_term);
	}

	virtual void SetInstantiationTime(int time) {
		Node::SetInstantiationTime(time);
		variable->SetInstantiationTime(time);
		formula->SetInstantiationTime(time);
	}

	virtual bool operator==(Node& other) {
		ForAll* forall = dynamic_cast<ForAll*>(&other);
		if (!forall)
			return false;

		return *variable == *forall->variable && *formula == *forall->formula;
	}

	virtual String ToString() const {
		return Format("(∀%s. %s)", variable->ToString(), formula->ToString());
	}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("ForAll (%d:%d) %s", GetTime(), GetRefs(), GetName());
		out << "\n" << variable->AsString(ident+1);
		out << "\n" << formula->AsString(ident+1);
		return out;
	}
	
	virtual NodeVar GetDNF() {
		return new ForAll(*variable->GetDNF(), *formula->GetDNF());
	}
	
};

class ThereExists : public Node {
	
protected:
	friend void TypecheckFormula ( Node& formula );
	friend bool ProveSequent(Node& sequent);
	
	NodeVar variable, formula;
	
public:

	ThereExists(Node& variable, Node& formula) : variable(&variable), formula(&formula) {
		
	}
	
	virtual int GetCount() const {return 2;}
	virtual Node& operator[] (int i) {if (i == 0) return *variable; if (i == 1) return *formula;}
	
	virtual Index<NodeVar> FreeVariables() {
		Index<NodeVar> out = formula->FreeVariables();
		int i = out.Find(variable);
		if (i != -1) out.Remove(i);
		return out;
		//return formula->FreeVariables() - { variable };
	}

	virtual Index<NodeVar> FreeUnificationTerms() {
		return formula->FreeUnificationTerms();
	}

	virtual NodeVar Replace(Node& old, Node& new_) {
		if (*this == old)
			return &new_;

		return NodeVar(new ThereExists(
		   *variable->Replace(old, new_),
		   *formula->Replace(old, new_)
		));
	}

	virtual bool Occurs(UnificationTerm& unification_term) {
		return formula->Occurs(unification_term);
	}

	virtual void SetInstantiationTime(int time) {
		Node::SetInstantiationTime(time);
		variable->SetInstantiationTime(time);
		formula->SetInstantiationTime(time);
	}

	virtual bool operator==(Node& other) {
		ThereExists* there_exists = dynamic_cast<ThereExists*>(&other);
		if (!there_exists)
			return false;

		return *variable == *there_exists->variable && *formula == *there_exists->formula;
	}

	virtual String ToString() const {
		return Format("(∃%s. %s)", variable->ToString(), formula->ToString());
	}
	
	virtual String AsString(int ident=0) const {
		String out;
		for(int i = 0; i < ident; i++) out.Cat('\t');
		out << Format("ThereExists (%d:%d) %s", GetTime(), GetRefs(), GetName());
		out << "\n" << variable->AsString(ident+1);
		out << "\n" << formula->AsString(ident+1);
		return out;
	}
	
	virtual NodeVar GetDNF() {
		return new ThereExists(*variable->GetDNF(), *formula->GetDNF());
	}
	
};


}


#endif
