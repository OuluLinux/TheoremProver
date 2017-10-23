#include "TheoremProver.h"


namespace TheoremProver {

String* catch_print;

void Print(String s) {
	if (catch_print) {*catch_print << s << "\n";}
	Cout() << s << EOL;
	LOG(s);
}


Index<NodeVar> axioms;
ArrayMap<NodeVar, Index<NodeVar> > lemmas;


void LogicCLI() {
	Print ( "First-Order Logic Theorem Prover" );
	Print ( "" );
	Print ( "Common commands:" );
	Print ( "  examples       : runs example code" );
	Print ( "" );
	Print ( "Terms:" );
	Print ( "" );
	Print ( "  x               (variable)" );
	Print ( "  f(term, ...)    (function)" );
	Print ( "" );
	Print ( "Formulae:" );
	Print ( "" );
	Print ( "  P(term)        (predicate)" );
	Print ( "  !P             (complement)" );
	Print ( "  P || Q         (disjunction)" );
	Print ( "  P && Q         (conjunction)" );
	Print ( "  P implies Q    (implication)" );
	Print ( "  forall x. P    (universal quantification)" );
	Print ( "  exists x. P    (existential quantification)" );
	Print ( "" );
	Print ( "Enter formulae at the prompt. The following commands are also available for manipulating axioms:" );
	Print ( "" );
	Print ( "  axioms              (list axioms)" );
	Print ( "  lemmas              (list lemmas)" );
	Print ( "  axiom <formula>     (add an axiom)" );
	Print ( "  lemma <formula>     (prove && add a lemma)" );
	Print ( "  remove <formula>    (remove an axiom or lemma)" );
	Print ( "  reset               (remove all axioms and lemmas)" );
	
	Vector<String> autocmds;
	
	//autocmds.Add("examples");
	/*autocmds.Add("axiom forall x. Equals(x, x)");
	autocmds.Add("axioms");
	autocmds.Add("lemma Equals(a, a)");*/
	/*autocmds.Add("axiom not (A and B)");
	autocmds.Add("axiom B or C");
	autocmds.Add("axiom C implies D");
	autocmds.Add("axiom A");
	autocmds.Add("lemma D");*/

	
	while ( 1 ) {
		try {
			Cout() << "\n> ";
			String inp;
			
			if (autocmds.GetCount()) {
				inp = autocmds[0];
				autocmds.Remove(0);
				Cout() << inp << EOL;
			}
			else inp = TrimBoth(ReadStdIn());
			
			if (inp.GetCount() == 0) break;
			
			Index<String> commands;
			commands.Add("axiom");
			commands.Add("lemma");
			commands.Add("axioms");
			commands.Add("lemmas");
			commands.Add("remove");
			commands.Add("reset");
			commands.Add("q");
			commands.Add("quit");
			
			Vector<String> tokens = Lex(inp);
			for(int i = 0; i < tokens.GetCount(); i++) {
				String& t = tokens[i];
				String l = ToLower(t);
				if (commands.Find(l) != -1)
					tokens[i] = l;
			}
			
			for(int i = 1; i < tokens.GetCount(); i++) {
				String& token = tokens[i];
				if ( commands.Find(token) != -1 )
					throw InvalidInputError ( Format("Unexpected keyword: %s.", token) );
			}

			if (tokens.GetCount() && (tokens[0] == "q" || tokens[0] == "quit")) {
				break;
			}
			
			if (tokens.GetCount() && tokens[0] == "examples") {
				//autocmds.Add("((a and not b) implies c) and ((not a) equals (b and c))");
				autocmds.Add("P or not P");
				autocmds.Add("P and not P");
				autocmds.Add("forall x. P(x) implies (Q(x) implies P(x))");
				autocmds.Add("exists x. (P(x) implies forall y. P(y))");
				autocmds.Add("axiom forall x. Equals(x, x)");
				autocmds.Add("axioms");
				autocmds.Add("lemma Equals(a, a)");
				autocmds.Add("lemmas");
				autocmds.Add("remove forall x. Equals(x, x)");
				
			}
			else if ( tokens.GetCount() > 0 && tokens[0] == "axioms" ) {
				if ( tokens.GetCount() > 1 )
					throw InvalidInputError ( Format("Unexpected: %s.", tokens[1]) );
				
				for(int i = 0; i < axioms.GetCount(); i++) {
					const NodeVar& axiom = axioms[i];
					Print(axiom->ToString());
				}
			}
			else if ( tokens.GetCount() > 0 && tokens[0] == "lemmas" ) {
				if ( tokens.GetCount() > 1 )
					throw InvalidInputError ( Format("Unexpected: %s.", tokens[1]) );
				
				for(int i = 0; i < lemmas.GetCount(); i++) {
					const NodeVar& lemma = lemmas.GetKey(i);
					Print(lemma->ToString());
				}
			}
			else if ( tokens.GetCount() > 0 && tokens[0] == "axiom" ) {
				Vector<String> tmp;
				for(int i = 1; i < tokens.GetCount(); i++)
					tmp.Add(tokens[i]);
				NodeVar formula = Parse(tmp);
				CheckFormula ( *formula );
				axioms.Insert (0, formula );
				Print ( Format( "Axiom added: %s.", formula->ToString() ));
			}
			else if ( tokens.GetCount() > 0 && tokens[0] == "lemma" ) {
				Vector<String> tmp1;
				for(int i = 1; i < tokens.GetCount(); i++)
					tmp1.Add(tokens[i]);
				NodeVar formula = Parse(tmp1);
				CheckFormula ( *formula );
				
				Index<NodeVar> tmp2;
				for(int i = 0; i < axioms.GetCount(); i++)
					tmp2.Add(axioms[i]);
				for(int i = 0; i < lemmas.GetCount(); i++)
					if (tmp2.Find(lemmas.GetKey(i)) == -1)
						tmp2.Add(lemmas.GetKey(i));
				
				bool result = ProveFormula ( tmp2, formula );

				if ( result ) {
					lemmas.GetAdd(formula) <<= axioms;
					Print ( Format( "Lemma proven: %s.", formula->ToString() ));
				}
				else
					Print ( Format( "Lemma unprovable: %s.", formula->ToString() ));
			}
			else if ( tokens.GetCount() > 0 && tokens[0] == "remove" ) {
				Vector<String> tmp;
				for(int i = 1; i < tokens.GetCount(); i++)
					tmp.Add(tokens[i]);
				NodeVar formula = Parse(tmp);
				CheckFormula ( *formula );
				
				int pos = axioms.Find(formula);
				if ( pos != -1 ) {
					axioms.Remove ( pos );
					Index<NodeVar> bad_lemmas;

					//for ( lemma, dependent_axioms in lemmas.items() ) {
					for (int i = 0; i < lemmas.GetCount(); i++) {
						const NodeVar& lemma = lemmas.GetKey(i);
						Index<NodeVar>& dependent_axioms = lemmas[i];
						if (dependent_axioms.Find(formula) != -1)
							bad_lemmas.Add ( lemma );
					}
					
					for (int i = 0; i < bad_lemmas.GetCount(); i++) {
						const NodeVar& ref = bad_lemmas[i];
						int j = lemmas.Find(ref);
						if (j != -1) lemmas.Remove(j);
					}

					Print ( Format( "Axiom removed: %s.", formula->ToString() ));

					if (bad_lemmas.GetCount() == 1) {
						Print ( "This lemma was proven using that axiom and was also removed:" );

						for (int i = 0; i < bad_lemmas.GetCount(); i++)
							Print ( Format( "  %s", bad_lemmas[i]->ToString() ));
					}

					else if (bad_lemmas.GetCount() > 1 ) {
						Print ( "These lemmas were proven using that axiom and were also removed:" );

						for (int i = 0; i < bad_lemmas.GetCount(); i++)
							Print ( Format( "  %s", bad_lemmas[i]->ToString() ));
					}
				}
				else if ( lemmas.Find(formula) != -1 ) {
					lemmas.Remove(lemmas.Find(formula));
					Print ( Format( "Lemma removed: %s.", formula->ToString() ));
				}
				else
					Print ( Format( "Not an axiom: %s.", formula->ToString() ));
			}
			else if ( tokens.GetCount() > 0 && tokens[0] == "reset" ) {
				if ( tokens.GetCount() > 1 )
					throw InvalidInputError ( Format("Unexpected: %s.", tokens[1]) );

				axioms.Clear();
				lemmas.Clear();
			}
			else {
				NodeVar formula = Parse( tokens );
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
			}
		}
		catch ( InvalidInputError e ) {
			Print ( e );
		}
	}
}

}

#ifdef flagMAIN
using namespace TheoremProver;

CONSOLE_APP_MAIN {
	BreakNullVarDtor();
	
	LogicCLI();
}
#endif
