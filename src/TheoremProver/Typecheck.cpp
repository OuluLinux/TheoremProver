#include "TheoremProver.h"

namespace TheoremProver {

void TypecheckTerm ( Node& term ) {
	if ( dynamic_cast<Variable*>(&term) )
		return;
	
	Function* fn = dynamic_cast<Function*>(&term);
	if (fn) {
		for(int i = 0; i < fn->terms.GetCount(); i++) {
			TypecheckTerm ( *fn->terms[i] );
		}
		return;
	}

	throw InvalidInputError ( Format("Invalid term: %s.", term.ToString()) );
}

void TypecheckFormula ( Node& formula ) {
	Predicate* pred = dynamic_cast<Predicate*>(&formula);
	if (pred) {
		for(int i = 0; i < pred->terms.GetCount(); i++)
			TypecheckTerm(*pred->terms[i]);
		return;
	}
	
	Not* not_ = dynamic_cast<Not*>(&formula);
	if (not_) {
		TypecheckFormula ( *not_->formula );
		return;
	}

	And* and_ = dynamic_cast<And*>(&formula);
	if (and_) {
		TypecheckFormula ( *and_->formula_a );
		TypecheckFormula ( *and_->formula_b );
		return;
	}
	
	Or* or_ = dynamic_cast<Or*>(&formula);
	if (or_) {
		TypecheckFormula ( *or_->formula_a );
		TypecheckFormula ( *or_->formula_b );
		return;
	}
	
	Implies* implies = dynamic_cast<Implies*>(&formula);
	if (implies) {
		TypecheckFormula ( *implies->formula_a );
		TypecheckFormula ( *implies->formula_b );
		return;
	}
	
	ForAll* forall = dynamic_cast<ForAll*>(&formula);
	if (forall) {
		if ( !dynamic_cast<Variable*>(&*forall->variable) )
			throw InvalidInputError (Format("Invalid bound variable in FORALL quantifier: %s.", forall->variable->ToString() ));

		TypecheckFormula ( *forall->formula );
		return;
	}
	
	ThereExists* there_exists = dynamic_cast<ThereExists*>(&formula);
	if (there_exists) {
		if (!dynamic_cast<Variable*>(&*there_exists->variable))
			throw InvalidInputError (Format("Invalid bound variable in exists quantifier: %s.", there_exists->variable->ToString()));

		TypecheckFormula ( *there_exists->formula );
		return;
	}

	throw InvalidInputError ( Format( "Invalid formula: %s.", formula.ToString() ) );
}

void CheckFormula ( Node& formula ) {
	try {
		TypecheckFormula ( formula );
	}
	catch ( InvalidInputError formula_error ) {
		try {
			TypecheckTerm ( formula );
		}
		catch ( InvalidInputError term_error ) {
			throw formula_error;
		}

		throw InvalidInputError ( "Enter a formula, !a term." );
	}
}

}
