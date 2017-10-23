#include "TheoremProver.h"

namespace TheoremProver {

int GetIndexCommonCount(const ArrayMap<NodeVar, int>& a, const ArrayMap<NodeVar, int>& b) {
	int count = 0;
	for(int i = 0; i < a.GetCount(); i++) {
		if (b.Find(a.GetKey(i)) != -1)
			count++;
	}
	return count;
}

void RemoveRef(ArrayMap<NodeVar, int>& ind, const NodeVar& ref) {
	int i = ind.Find(ref);
	if (i != -1)
		ind.Remove(i);
}

int& GetInsert(ArrayMap<NodeVar, int>& data, const NodeVar& ref) {
	int i = data.Find(ref);
	if (i != -1)
		return data[i];
	return data.Insert(0, ref);
}

// Unification

// solve a single equation
ArrayMap<NodeVar, NodeVar> Unify(Node& term_a, Node& term_b) {
	UnificationTerm* uniterm_a = dynamic_cast<UnificationTerm*>(&term_a);
	UnificationTerm* uniterm_b = dynamic_cast<UnificationTerm*>(&term_b);
	ArrayMap<NodeVar, NodeVar> out;
	
	if (uniterm_a) {
		if (term_b.Occurs(*uniterm_a) || term_b.GetTime() > term_a.GetTime())
			return out;
		
		out.Add(&term_a, &term_b);
		return out;
	}

	if (uniterm_b) {
		if (term_a.Occurs(*uniterm_b) || term_a.GetTime() > term_b.GetTime())
			return out;

		out.Add(&term_b, &term_a);
		return out;
	}

	if (dynamic_cast<Variable*>(&term_a) && dynamic_cast<Variable*>(&term_b)) {
		if (term_a == term_b)
			return { };

		return out;
	}

	Function* fn_a = dynamic_cast<Function*>(&term_a);
	Function* fn_b = dynamic_cast<Function*>(&term_b);
	Predicate* pred_a = dynamic_cast<Predicate*>(&term_a);
	Predicate* pred_b = dynamic_cast<Predicate*>(&term_b);
	
	if ((fn_a && fn_b) || (pred_a && pred_b)) {
		if (term_a.GetName() != term_b.GetName())
			return out;
		
		bool is_fn = (fn_a && fn_b);
		Index<NodeVar>& a_terms = is_fn ? fn_a->terms : pred_a->terms;
		Index<NodeVar>& b_terms = is_fn ? fn_b->terms : pred_b->terms;
		
		if (a_terms.GetCount() != b_terms.GetCount())
			return out;

		ArrayMap<NodeVar, NodeVar> substitution;

		for (int i = 0; i < a_terms.GetCount(); i++) {
			NodeVar a = a_terms[i];
			NodeVar b = b_terms[i];
			
			for (int j = 0; j < substitution.GetCount(); j++) {
				const NodeVar& k = substitution.GetKey(j);
				NodeVar& v = substitution[j];
				a = a->Replace(*k, *v);
				b = b->Replace(*k, *v);
			}

			ArrayMap<NodeVar, NodeVar> sub = Unify(*a, *b);

			if (sub.GetCount() == 0)
				return out;
			
			for(int j = 0; j < sub.GetCount(); j++) {
				const NodeVar& k = sub.GetKey(j);
				NodeVar& v = sub[j];
				substitution.Add(k, v);
			}
		}

		return substitution;
	}

	return out;
}

// solve a list of equations
ArrayMap<NodeVar, NodeVar> UnifyList(const ArrayMap<NodeVar, NodeVar>& pairs) {
	ArrayMap<NodeVar, NodeVar> substitution;
	
	for (int i = 0; i < pairs.GetCount(); i++) {
		const NodeVar& term_a = pairs.GetKey(i);
		const NodeVar& term_b = pairs[i];
		NodeVar a = term_a;
		NodeVar b = term_b;
		
		for(int j = 0; j < substitution.GetCount(); j++) {
			const NodeVar& k = substitution.GetKey(j);
			NodeVar& v = substitution[j];
			a = a->Replace(*k, *v);
			b = b->Replace(*k, *v);
		}

		ArrayMap<NodeVar, NodeVar> sub = Unify(*a, *b);

		if (sub.GetCount() == 0)
			return ArrayMap<NodeVar, NodeVar>();
		
		for(int j = 0; j < sub.GetCount(); j++) {
			const NodeVar& k = sub.GetKey(j);
			NodeVar& v = sub[j];
			substitution.Add(k, v);
		}
	}

	return substitution;
}

// Sequents
class Sequent : public Node {
	
protected:
	friend bool ProveSequent(Node& sequent);
	
	ArrayMap<NodeVar, int> left, right;
	Index<NodeVar> siblings;
	int depth;
	
public:
	Sequent(const ArrayMap<NodeVar, int>& left, const ArrayMap<NodeVar, int>& right, const Index<NodeVar>& siblings, int depth) : Node("") {
		this->left <<= left;
		this->right <<= right;
		this->siblings <<= siblings;
		this->depth = depth;
	}

	virtual Index<NodeVar> FreeVariables() {
		Index<NodeVar> result;

		for(int i = 0; i < left.GetCount(); i++)
			Append(result, left.GetKey(i)->FreeVariables());
		
		for(int i = 0; i < right.GetCount(); i++)
			Append(result, right.GetKey(i)->FreeVariables());
		
		return result;
	}

	virtual Index<NodeVar> FreeUnificationTerms() {
		Index<NodeVar> result;

		for(int i = 0; i < left.GetCount(); i++)
			Append(result, left.GetKey(i)->FreeUnificationTerms());
		
		for(int i = 0; i < right.GetCount(); i++)
			Append(result, right.GetKey(i)->FreeUnificationTerms());
		
		return result;
	}

	String GetVariableName(const String& prefix) {
		Index<NodeVar> fv;
		Append(fv, FreeVariables());
		Append(fv, FreeUnificationTerms());
		int index = 1;
		String name = prefix + IntStr(index);

		NodeVar var_ref = new Variable(name);
		NodeVar unterm_ref = new UnificationTerm(name);
		
		while (fv.Find(var_ref) != -1 || fv.Find(unterm_ref) != -1) {
			index += 1;
			name = prefix + IntStr(index);
			var_ref = new Variable(name);
			unterm_ref = new UnificationTerm(name);
		}
		
		return name;
	}

	ArrayMap<NodeVar, NodeVar> GetUnifiablePairs() {
		ArrayMap<NodeVar, NodeVar> pairs;

		for (int i = 0; i < left.GetCount(); i++) {
			const NodeVar& formula_left = left.GetKey(i);
			for (int j = 0; j < right.GetCount(); j++) {
				const NodeVar& formula_right = right.GetKey(j);
				ArrayMap<NodeVar, NodeVar> tmp = Unify(*formula_left, *formula_right);
				if (tmp.GetCount())
					pairs.Add(formula_left, formula_right);
			}
		}

		return pairs;
	}

	virtual bool operator==(Node& other) {
		Sequent* seq = dynamic_cast<Sequent*>(&other);
		
		for (int i = 0; i < left.GetCount(); i++) {
			if (seq->left.Find(left.GetKey(i)) == -1)
				return false;
		}

		for (int i = 0; i < seq->left.GetCount(); i++) {
			if (left.Find(seq->left.GetKey(i)) == -1)
				return false;
		}

		for (int i = 0; i < right.GetCount(); i++) {
			if (seq->right.Find(right.GetKey(i)) == -1)
				return false;
		}

		for (int i = 0; i < seq->right.GetCount(); i++) {
			if (right.Find(seq->right.GetKey(i)) == -1)
				return false;
		}

		return true;
	}

	virtual String ToString() const {
		String left_part, right_part;
		
		for(int i = 0; i < left.GetCount(); i++) {
			if (i) left_part += ", ";
			left_part += left.GetKey(i)->ToString();
		}
		
		for(int i = 0; i < right.GetCount(); i++) {
			if (i) right_part += ", ";
			right_part += right.GetKey(i)->ToString();
		}

		if (left_part != "")
			left_part = left_part + " ";

		if (right_part != "")
			right_part = " " + right_part;

		return left_part + "⊢" + right_part;
	}

};


// Proof search

// returns true if the sequent == provable
// returns false || loops forever if the sequent != provable
bool ProveSequent(Node& sequent_) {
	Sequent& sequent = dynamic_cast<Sequent&>(sequent_);
	
	// reset the time for each formula in the sequent
	for (int i = 0; i < sequent.left.GetCount(); i++)
		sequent.left.GetKey(i)->SetInstantiationTime(0);

	for (int i = 0; i < sequent.right.GetCount(); i++)
		sequent.right.GetKey(i)->SetInstantiationTime(0);

	// sequents to be proven
	Vector<NodeVar> frontier;
	frontier.Add(&sequent);
	
	//# sequents which have been proven
	Index<NodeVar> proven;
	proven.Add(&sequent);
	
	String prev_str;
	
	while (true) {
		// get the next sequent
		NodeVar old_sequent_;

		while (frontier.GetCount() > 0 && (old_sequent_.Is() == false || proven.Find(old_sequent_) != -1)) {
			old_sequent_ = frontier[0];
			frontier.Remove(0);
		}

		if (old_sequent_.Is() == false)
			break;
		
		Sequent* old_sequent = old_sequent_.Get<Sequent>();
		ASSERT(old_sequent);
		String seq_str = old_sequent->ToString();
		if (seq_str == prev_str || old_sequent->depth > 10) {
			Print("Unable to continue");
			return false;
		}
		prev_str = seq_str;
		Print(Format("%d. %s", old_sequent->depth, seq_str));

		// check if this sequent == axiomatically true without unification
		if (GetIndexCommonCount(old_sequent->left, old_sequent->right) > 0) {
			proven.Insert(0, old_sequent);
			continue;
		}

		// check if this sequent has unification terms
		if (old_sequent->siblings.GetCount()) {
			
			// get the unifiable pairs for each sibling
			Vector<ArrayMap<NodeVar, NodeVar> > sibling_pair_lists;
			for(int i = 0; i < old_sequent->siblings.GetCount(); i++) {
				sibling_pair_lists.Add(old_sequent->siblings[i].Get<Sequent>()->GetUnifiablePairs());
			}

			// check if there == a unifiable pair for each sibling
			bool all_has_count = true;
			for(int i = 0; i < sibling_pair_lists.GetCount(); i++)
				if (sibling_pair_lists[i].GetCount() == 0)
					all_has_count = false;
			
			if (all_has_count) {
				
				// iterate through all simultaneous choices of pairs from each sibling
				ArrayMap<NodeVar, NodeVar> substitution;
				Vector<int> index;
				index.SetCount(sibling_pair_lists.GetCount(), 0);

				while (true) {
					// attempt to unify at the index
					ArrayMap<NodeVar, NodeVar> tmp;
					for(int i = 0; i < sibling_pair_lists.GetCount(); i++) {
						int j = index[i];
						tmp.Add(sibling_pair_lists[i].GetKey(j), sibling_pair_lists[i][j]);
					}
					substitution = UnifyList(tmp);

					if (substitution.GetCount())
						break;

					// increment the index
					int pos = sibling_pair_lists.GetCount() - 1;

					while (pos >= 0) {
						index[pos] += 1;

						if (index[pos] < sibling_pair_lists[pos].GetCount())
							break;

						index[pos] = 0;
						pos -= 1;
					}

					if (pos < 0)
						break;
				}

				if (substitution.GetCount()) {
					for(int i = 0; i < substitution.GetCount(); i++) {
						const NodeVar& k = substitution.GetKey(i);
						const NodeVar& v = substitution[i];
						Print(Format( "  %s = %s", k->ToString(), v->ToString()));
					}
					
					Append(proven, old_sequent->siblings);
					
					for(int i = 0; i < frontier.GetCount(); i++) {
						if (old_sequent->siblings.Find(frontier[i]) != -1) {
							frontier.Remove(i);
							i--;
						}
					}
					//frontier = [sequent for sequent in frontier if sequent !in old_sequent->siblings];
					continue;
				}
			}
			else {
				// unlink this sequent
				while (1) {
					int i = old_sequent->siblings.Find(old_sequent);
					if (i != -1)
						old_sequent->siblings.Remove(i);
					else
						break;
				}
				//old_sequent->siblings.remove(old_sequent);
			}
		}

		while (true) {
			// determine which formula to expand
			NodeVar left_formula;
			int left_depth = -1;

			//for (formula, depth in old_sequent->left.items()) {
			for (int i = 0; i < old_sequent->left.GetCount(); i++) {
				const NodeVar& formula = old_sequent->left.GetKey(i);
				int depth = old_sequent->left[i];
				
				if (left_depth == -1 || left_depth > depth) {
					if (!dynamic_cast<Predicate*>(&*formula)) {
						left_formula = formula;
						left_depth = depth;
					}
				}
			}

			NodeVar right_formula;
			int right_depth = -1;

			//for (formula, depth in old_sequent->right.items()) {
			for (int i = 0; i < old_sequent->right.GetCount(); i++) {
				const NodeVar& formula = old_sequent->right.GetKey(i);
				int depth = old_sequent->right[i];
				
				if (right_depth == -1 || right_depth > depth) {
					if (!dynamic_cast<Predicate*>(&*formula)) {
						right_formula = formula;
						right_depth = depth;
						LOG(formula->AsString());
					}
				}
			}
		
			bool apply_left = false;
			bool apply_right = false;

			if (left_formula.Is() && !right_formula.Is())
				apply_left = true;

			if (!left_formula.Is() && right_formula.Is())
				apply_right = true;

			if (left_formula.Is() && right_formula.Is()) {
				if (left_depth < right_depth)
					apply_left = true;
				else
					apply_right = true;
			}

			if (!left_formula.Is() && !right_formula.Is())
				return false;

			// apply a left rule
			if (apply_left) {
				Not* not_ = dynamic_cast<Not*>(&*left_formula);
				if (not_) {
					Sequent* new__sequent = new Sequent(
									  old_sequent->left,
									  old_sequent->right,
									  old_sequent->siblings,
									  old_sequent->depth + 1
								  );
					new__sequent->Inc();
					RemoveRef(new__sequent->left, left_formula);
					GetInsert(new__sequent->right, not_->formula) = (old_sequent->left.Get(left_formula) + 1);

					//if (new__sequent->siblings.GetCount())
						new__sequent->siblings.Insert(0, new__sequent);

					frontier.Add(new__sequent);
					new__sequent->Dec();
					break;
				}
				
				And* and_ = dynamic_cast<And*>(&*left_formula);
				if (and_) {
					Sequent* new__sequent = new Sequent(
									  old_sequent->left,
									  old_sequent->right,
									  old_sequent->siblings,
									  old_sequent->depth + 1
								  );
					new__sequent->Inc();
					RemoveRef(new__sequent->left, left_formula);
					GetInsert(new__sequent->left, and_->formula_a) = (old_sequent->left.Get(left_formula) + 1);
					GetInsert(new__sequent->left, and_->formula_b) = (old_sequent->left.Get(left_formula) + 1);

					//if (new__sequent->siblings.GetCount())
						new__sequent->siblings.Insert(0, new__sequent);

					frontier.Add(new__sequent);
					new__sequent->Dec();
					break;
				}
				
				Or* or_ = dynamic_cast<Or*>(&*left_formula);
				if (or_) {
					Sequent* new__sequent_a = new Sequent(
										old_sequent->left,
										old_sequent->right,
										old_sequent->siblings,
										old_sequent->depth + 1
									);
					Sequent* new__sequent_b = new Sequent(
										old_sequent->left,
										old_sequent->right,
										old_sequent->siblings,
										old_sequent->depth + 1
									);
					new__sequent_a->Inc();
					new__sequent_b->Inc();
					RemoveRef(new__sequent_a->left, left_formula);
					RemoveRef(new__sequent_b->left, left_formula);
					GetInsert(new__sequent_a->left, or_->formula_a) = (old_sequent->left.Get(left_formula) + 1);
					GetInsert(new__sequent_b->left, or_->formula_b) = (old_sequent->left.Get(left_formula) + 1);

					if (new__sequent_a->siblings.GetCount())
						new__sequent_a->siblings.Insert(0, new__sequent_a);

					frontier.Add(new__sequent_a);

					if (new__sequent_b->siblings.GetCount())
						new__sequent_b->siblings.Insert(0, new__sequent_b);

					frontier.Add(new__sequent_b);
					new__sequent_a->Dec();
					new__sequent_b->Dec();
					break;
				}
				
				Implies* implies = dynamic_cast<Implies*>(&*left_formula);
				if (implies) {
					Sequent* new__sequent_a = new Sequent(
										old_sequent->left,
										old_sequent->right,
										old_sequent->siblings,
										old_sequent->depth + 1
									);
					Sequent* new__sequent_b = new Sequent(
										old_sequent->left,
										old_sequent->right,
										old_sequent->siblings,
										old_sequent->depth + 1
									);
					new__sequent_a->Inc();
					new__sequent_b->Inc();
					RemoveRef(new__sequent_a->left, left_formula);
					RemoveRef(new__sequent_b->left, left_formula);
					GetInsert(new__sequent_a->right, implies->formula_a) = (old_sequent->left.Get(left_formula) + 1);
					GetInsert(new__sequent_b->left,  implies->formula_b) = ( old_sequent->left.Get(left_formula) + 1);

					if (new__sequent_a->siblings.GetCount())
						new__sequent_a->siblings.Insert(0, new__sequent_a);

					frontier.Add(new__sequent_a);

					if (new__sequent_b->siblings.GetCount())
						new__sequent_b->siblings.Insert(0, new__sequent_b);

					frontier.Add(new__sequent_b);
					new__sequent_a->Dec();
					new__sequent_b->Dec();
					break;
				}
				
				ForAll* forall = dynamic_cast<ForAll*>(&*left_formula);
				if (forall) {
					Sequent* new__sequent = new Sequent(
									  old_sequent->left,
									  old_sequent->right,
									  old_sequent->siblings,
									  old_sequent->depth + 1
								  );
					new__sequent->Inc();
					new__sequent->left.Get(left_formula) += 1;
					NodeVar unterm = new UnificationTerm(old_sequent->GetVariableName("t"));
					NodeVar formula = forall->formula->Replace(*forall->variable, *unterm);
					formula->SetInstantiationTime(old_sequent->depth + 1);
					SortByKey(new__sequent->left, NodeVar());
					
					if (new__sequent->left.Find(formula) == -1)
						new__sequent->left.Add(formula, new__sequent->left.Get(left_formula));

					new__sequent->siblings.Insert(0, new__sequent);

					frontier.Add(new__sequent);
					new__sequent->Dec();
					break;
				}
				
				ThereExists* there_exists = dynamic_cast<ThereExists*>(&*left_formula);
				if (there_exists) {
					Sequent* new__sequent = new Sequent(
									  old_sequent->left,
									  old_sequent->right,
									  old_sequent->siblings,
									  old_sequent->depth + 1
								  );
					new__sequent->Inc();
					RemoveRef(new__sequent->left, left_formula);
					NodeVar variable = new Variable(old_sequent->GetVariableName("v"));
					NodeVar formula = there_exists->formula->Replace(*there_exists->variable, *variable);
					formula->SetInstantiationTime(old_sequent->depth + 1);
					GetInsert(new__sequent->left, formula) = (old_sequent->left.Get(left_formula) + 1);
					SortByKey(new__sequent->left, NodeVar());
					
					new__sequent->siblings.Insert(0, new__sequent);

					frontier.Add(new__sequent);
					new__sequent->Dec();
					break;
				}
			}

			// apply a right rule
			if (apply_right) {
				Not* not_ = dynamic_cast<Not*>(&*right_formula);
				if (not_) {
					Sequent* new__sequent = new Sequent(
									  old_sequent->left,
									  old_sequent->right,
									  old_sequent->siblings,
									  old_sequent->depth + 1
								  );
					new__sequent->Inc();
					RemoveRef(new__sequent->right, right_formula);
					GetInsert(new__sequent->left, not_->formula) = (old_sequent->right.Get(right_formula) + 1);
					SortByKey(new__sequent->left, NodeVar());
					
					new__sequent->siblings.Insert(0, new__sequent);

					frontier.Add(new__sequent);
					new__sequent->Dec();
					break;
				}
				
				And* and_ = dynamic_cast<And*>(&*right_formula);
				if (and_) {
					Sequent* new__sequent_a = new Sequent(
										old_sequent->left,
										old_sequent->right,
										old_sequent->siblings,
										old_sequent->depth + 1
									);
					Sequent* new__sequent_b = new Sequent(
										old_sequent->left,
										old_sequent->right,
										old_sequent->siblings,
										old_sequent->depth + 1
									);
					RemoveRef(new__sequent_a->right, right_formula);
					RemoveRef(new__sequent_b->right, right_formula);
					GetInsert(new__sequent_a->right, and_->formula_a) = old_sequent->right.Get(right_formula) + 1;
					GetInsert(new__sequent_b->right, and_->formula_b) = old_sequent->right.Get(right_formula) + 1;
					SortByKey(new__sequent_a->right, NodeVar());
					SortByKey(new__sequent_b->right, NodeVar());
					
					if (new__sequent_a->siblings.GetCount())
						new__sequent_a->siblings.Insert(0, new__sequent_a);

					frontier.Add(new__sequent_a);

					if (new__sequent_b->siblings.GetCount())
						new__sequent_b->siblings.Insert(0, new__sequent_b);

					frontier.Add(new__sequent_b);
					break;
				}
				
				Or* or_ = dynamic_cast<Or*>(&*right_formula);
				if (or_) {
					Sequent* new__sequent = new Sequent(
									  old_sequent->left,
									  old_sequent->right,
									  old_sequent->siblings,
									  old_sequent->depth + 1
								  );
					new__sequent->Inc();
					RemoveRef(new__sequent->right, right_formula);
					GetInsert(new__sequent->right, or_->formula_a) = old_sequent->right.Get(right_formula) + 1;
					GetInsert(new__sequent->right, or_->formula_b) = old_sequent->right.Get(right_formula) + 1;
					SortByKey(new__sequent->right, NodeVar());
					
					new__sequent->siblings.Insert(0, new__sequent);

					frontier.Add(new__sequent);
					new__sequent->Dec();
					break;
				}
				
				Implies* implies = dynamic_cast<Implies*>(&*right_formula);
				if (implies) {
					Sequent* new__sequent = new Sequent(
									  old_sequent->left,
									  old_sequent->right,
									  old_sequent->siblings,
									  old_sequent->depth + 1
								  );
					new__sequent->Inc();
					RemoveRef(new__sequent->right, right_formula);
					GetInsert(new__sequent->left,  implies->formula_a) = old_sequent->right.Get(right_formula) + 1;
					GetInsert(new__sequent->right, implies->formula_b) = old_sequent->right.Get(right_formula) + 1;
					SortByKey(new__sequent->right, NodeVar());
					
					new__sequent->siblings.Insert(0, new__sequent);

					frontier.Add(new__sequent);
					new__sequent->Dec();
					break;
				}
				
				ForAll* forall = dynamic_cast<ForAll*>(&*right_formula);
				if (forall) {
					Sequent* new__sequent = new Sequent(
									  old_sequent->left,
									  old_sequent->right,
									  old_sequent->siblings,
									  old_sequent->depth + 1
								  );
					new__sequent->Inc();
					RemoveRef(new__sequent->right, right_formula);
					NodeVar variable(new Variable(old_sequent->GetVariableName("v")));
					NodeVar formula = forall->formula->Replace(*forall->variable, *variable);
					formula->SetInstantiationTime(old_sequent->depth + 1);
					GetInsert(new__sequent->right, formula) = old_sequent->right.Get(right_formula) + 1;
					SortByKey(new__sequent->right, NodeVar());
					
					new__sequent->siblings.Insert(0, new__sequent);

					frontier.Add(new__sequent);
					new__sequent->Dec();
					break;
				}
				
				ThereExists* there_exists = dynamic_cast<ThereExists*>(&*right_formula);
				if (there_exists) {
					Sequent* new__sequent = new Sequent(
									  old_sequent->left,
									  old_sequent->right,
									  old_sequent->siblings,
									  old_sequent->depth + 1
								  );
					new__sequent->Inc();
					new__sequent->right.Get(right_formula) += 1;
					NodeVar uniterm = new UnificationTerm(old_sequent->GetVariableName("t"));
					NodeVar formula = there_exists->formula->Replace(*there_exists->variable, *uniterm);
					formula->SetInstantiationTime(old_sequent->depth + 1);
					
					if (new__sequent->right.Find(formula) == -1)
						new__sequent->right.Insert(0, formula, new__sequent->right.Get(right_formula));
					SortByKey(new__sequent->right, NodeVar());
					new__sequent->siblings.Insert(0, new__sequent);

					frontier.Add(new__sequent);
					new__sequent->Dec();
					break;
				}
			}
		}
	}

	// no more sequents to prove
	return true;
}

// returns true if the formula == provable
// returns false || loops forever if the formula != provable
bool ProveFormula(const Index<NodeVar>& axioms, const NodeVar& formula) {
	ArrayMap<NodeVar, int> left, right;
	for(int i = 0; i < axioms.GetCount(); i++)
		left.Add(axioms[i], 0);
	right.Add(formula, 0);
	NodeVar seq(new Sequent(left, right, Index<NodeVar>(), 0));
	return ProveSequent(*seq);
}

extern String* catch_print;
extern Index<NodeVar> axioms;
extern ArrayMap<NodeVar, Index<NodeVar> > lemmas;

String ProveLogicNode(NodeVar formula) {
	String out;
	if (!formula.Is()) return out;
	
	catch_print = &out;
	
	CheckFormula ( *formula );
	Index<NodeVar> tmp;
	for(int i = 0; i < axioms.GetCount(); i++)
		tmp.Add(axioms[i]);
	for(int i = 0; i < lemmas.GetCount(); i++) {
		int j = tmp.Find(lemmas.GetKey(i));
		if (j == -1) tmp.Add(lemmas.GetKey(i));
	}
	
	bool result = ProveFormula ( tmp, formula );
	ASSERT(formula.GetNode());
	
	if ( result )
		Print ( Format( "Formula proven: %s.", formula->ToString() ));
	else
		Print ( Format( "Formula unprovable: %s.", formula->ToString() ));
	
	catch_print = 0;
	
	return out;
}

String ProveLogic(String str) {
	NodeVar n = Parse(str);
	if (n.Is()) return ProveLogicNode(n);
	return "";
}

}


