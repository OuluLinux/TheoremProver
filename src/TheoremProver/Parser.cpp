#include "TheoremProver.h"

namespace TheoremProver {


bool IsAlphaNumber(int chr) {return IsAlpha ( chr ) || IsDigit ( chr );}
bool IsAlphaNumber(const String& s) {
	for(int i = 0; i < s.GetCount(); i++) {
		if (!IsAlphaNumber(s[i]))
			return false;
	}
	return s.GetCount();
}
bool IsLower(const String& s) {
	for(int i = 0; i < s.GetCount(); i++) {
		if (!Upp::IsLower(s[i]))
			return false;
	}
	return s.GetCount();
}
bool HasUpper(const String& s) {
	for(int i = 0; i < s.GetCount(); i++) {
		if (Upp::IsUpper(s[i]))
			return true;
	}
	return false;
}

Vector<String> Lex( const String& inp ) {
	// perform a lexical analysis
	Vector<String> tokens;
	int pos = 0;

	while ( pos < inp.GetCount() ) {
		int chr = inp[pos];
		
		// skip whitespace
		if ( chr == ' ' || chr == '\t' ) {
			pos += 1;
			continue;
		}

		// identifiers
		String identifier;

		while ( pos < inp.GetCount() ) {
			chr = inp[pos];

			if ( !IsAlphaNumber(chr) )
				break;

			identifier.Cat ( chr );
			pos += 1;
		}

		if ( identifier.GetCount() ) {
			tokens.Add ( identifier );
			continue;
		}

		// symbols
		identifier.Clear();
		identifier.Cat ( inp[pos] );
		tokens.Add ( identifier );
		pos += 1;
	}

	// return the tokens
	return tokens;
}

NodeVar Parse( Vector<String>& tokens ) {
	
	// keywords
	Index<String> keywords;
	keywords.Add ( "not" );
	keywords.Add ( "implies" );
	keywords.Add ( "and" );
	keywords.Add ( "or" );
	keywords.Add ( "forall" );
	keywords.Add ( "exists" );

	for ( int i = 0; i < tokens.GetCount(); i++ ) {
		String s = ToLower ( tokens[i] );

		if ( keywords.Find ( s ) != -1 )
			tokens[i] = s;
	}

	// empty formula
	if ( tokens.GetCount() == 0 )
		throw InvalidInputError ( "Empty formula." );

	// ForAll
	if ( tokens[0] == "forall" ) {
		int dot_pos = -1;

		for ( int i = 0; i < tokens.GetCount(); i++ ) {
			if ( tokens[i] == "." ) {
				dot_pos = i;
				break;
			}
		}

		if ( dot_pos == -1 ) {
			throw InvalidInputError ( "Missing \".\" in FORALL quantifier." );
		}

		Vector<NodeVar> args;
		int i = 1;

		while ( i <= dot_pos ) {
			int end = dot_pos;
			int depth = 0;

			for ( int j = i; j < dot_pos; j++ ) {
				if ( tokens[j] == "(" ) {
					depth += 1;
					continue;
				}

				if ( tokens[j] == ")" ) {
					depth -= 1;
					continue;
				}

				if ( depth == 0 && tokens[j] == "," ) {
					end = j;
					break;
				}
			}

			if ( i == end )
				throw InvalidInputError ( "Missing variable in FORALL quantifier." );
			
			Vector<String> tmp;
			for(int j = i; j < end; j++)
				tmp.Add(tokens[j]);
			
			args.Add ( Parse( tmp ) );
			i = end + 1;
		}

		if ( tokens.GetCount() == dot_pos + 1 ) {
			throw InvalidInputError ( "Missing formula in FORALL quantifier." );
		}
		
		Vector<String> tmp;
		for(int j = dot_pos + 1; j < tokens.GetCount(); j++)
			tmp.Add(tokens[j]);
		
		NodeVar formula = Parse( tmp );

		for (int j = args.GetCount() - 1; j >= 0; j--)
			formula = NodeVar(new ForAll ( *args[j], *formula ));
		
		return formula;
	}

	// ThereExists
	if ( tokens[0] == "exists" ) {
		int dot_pos = -1;

		for ( int i = 1; i < tokens.GetCount(); i++) {
			if ( tokens[i] == "." ) {
				dot_pos = i;
				break;
			}
		}

		if ( dot_pos == -1 ) {
			throw InvalidInputError ( "Missing \".\" in exists quantifier." );
		}

		if ( dot_pos == 1 ) {
			throw InvalidInputError ( "Missing variable in exists quantifier." );
		}

		Vector<NodeVar> args;
		int i = 1;

		while ( i <= dot_pos ) {
			int end = dot_pos;
			int depth = 0;

			for (int j = i; j < dot_pos; j++) {
				if ( tokens[j] == "(" ) {
					depth += 1;
					continue;
				}

				if ( tokens[j] == ")" ) {
					depth -= 1;
					continue;
				}

				if ( depth == 0 && tokens[j] == "," ) {
					end = j;
					break;
				}
			}

			if ( i == end )
				throw InvalidInputError ( "Missing variable in exists quantifier." );
			
			Vector<String> tmp;
			for(int j = i; j < end; j++)
				tmp.Add(tokens[j]);
			args.Add(Parse(tmp));
			
			i = end + 1;
		}

		if ( tokens.GetCount() == dot_pos + 1 )
			throw InvalidInputError ( "Missing formula in exists quantifier." );
		
		Vector<String> tmp;
		for(int j = dot_pos + 1; j < tokens.GetCount(); j++)
			tmp.Add(tokens[j]);
		NodeVar formula = Parse(tmp);
		
		for(int j = args.GetCount()-1; j >= 0; j--)
			formula = new ThereExists ( *args[j], *formula );

		return formula;
	}

	// Implies
	int implies_pos = -1;
	int depth = 0;

	for (int i = 0; i < tokens.GetCount(); i++) {
		if ( tokens[i] == "(" ) {
			depth += 1;
			continue;
		}

		if ( tokens[i] == ")" ) {
			depth -= 1;
			if (depth < 0) break;
			continue;
		}

		if ( depth == 0 && tokens[i] == "implies" ) {
			implies_pos = i;
			break;
		}
	}

	if ( implies_pos != -1 ) {
		bool quantifier_in_left = false;
		int depth = 0;

		for (int i = 0; i < implies_pos; i++) {
			if ( tokens[i] == "(" ) {
				depth += 1;
				continue;
			}

			if ( tokens[i] == ")" ) {
				depth -= 1;
				if (depth < 0) break;
				continue;
			}

			if ( depth == 0 && ( tokens[i] == "forall" || tokens[i] == "exists" ) ) {
				quantifier_in_left = true;
				break;
			}
		}

		if ( !quantifier_in_left ) {
			if ( implies_pos == 0 || implies_pos == tokens.GetCount() - 1 )
				throw InvalidInputError ( "Missing formula in IMPLIES connective." );
			
			Vector<String> tmp1, tmp2;
			for(int i = 0; i < implies_pos; i++)
				tmp1.Add(tokens[i]);
			for(int i = implies_pos+1; i < tokens.GetCount(); i++)
				tmp2.Add(tokens[i]);
			return new Implies ( *Parse(tmp1), *Parse(tmp2) );
		}
	}
	
	// Equals
	int eq_pos = -1;
	depth = 0;

	for (int i = 0; i < tokens.GetCount(); i++) {
		if ( tokens[i] == "(" ) {
			depth += 1;
			continue;
		}

		if ( tokens[i] == ")" ) {
			depth -= 1;
			if (depth < 0) break;
			continue;
		}

		if ( depth == 0 && tokens[i] == "equals" ) {
			eq_pos = i;
			break;
		}
	}

	if ( eq_pos != -1 ) {
		bool quantifier_in_left = false;
		int depth = 0;
		
		for (int i = 0; i < eq_pos; i++) {
			if ( tokens[i] == "(" ) {
				depth += 1;
				continue;
			}

			if ( tokens[i] == ")" ) {
				depth -= 1;
				if (depth < 0) break;
				continue;
			}

			if ( depth == 0 && ( tokens[i] == "forall" || tokens[i] == "exists" ) ) {
				quantifier_in_left = true;
				break;
			}
		}

		if ( !quantifier_in_left ) {
			if ( eq_pos == 0 || eq_pos == tokens.GetCount() - 1 )
				throw InvalidInputError ( "Missing formula in OR connective." );
			
			Vector<String> tmp1, tmp2;
			for(int i = 0; i < eq_pos; i++)
				tmp1.Add(tokens[i]);
			for(int i = eq_pos+1; i < tokens.GetCount(); i++)
				tmp2.Add(tokens[i]);
			
			NodeVar a = Parse(tmp1);
			NodeVar b = Parse(tmp2);
			NodeVar impl_lr = new Implies(*a, *b);
			NodeVar impl_rl = new Implies(*b, *a);
			return new And( *impl_lr, *impl_rl );
		}
	}

	// Or
	int or_pos = -1;
	depth = 0;

	for (int i = 0; i < tokens.GetCount(); i++) {
		if ( tokens[i] == "(" ) {
			depth += 1;
			continue;
		}

		if ( tokens[i] == ")" ) {
			depth -= 1;
			if (depth < 0) break;
			continue;
		}

		if ( depth == 0 && tokens[i] == "or" ) {
			or_pos = i;
			break;
		}
	}

	if ( or_pos != -1 ) {
		bool quantifier_in_left = false;
		int depth = 0;
		
		for (int i = 0; i < or_pos; i++) {
			if ( tokens[i] == "(" ) {
				depth += 1;
				continue;
			}

			if ( tokens[i] == ")" ) {
				depth -= 1;
				if (depth < 0) break;
				continue;
			}

			if ( depth == 0 && ( tokens[i] == "forall" || tokens[i] == "exists" ) ) {
				quantifier_in_left = true;
				break;
			}
		}

		if ( !quantifier_in_left ) {
			if ( or_pos == 0 || or_pos == tokens.GetCount() - 1 )
				throw InvalidInputError ( "Missing formula in OR connective." );
			
			Vector<String> tmp1, tmp2;
			for(int i = 0; i < or_pos; i++)
				tmp1.Add(tokens[i]);
			for(int i = or_pos+1; i < tokens.GetCount(); i++)
				tmp2.Add(tokens[i]);
			return new Or ( *Parse(tmp1), *Parse(tmp2) );
		}
	}

	// And
	int and_pos = -1;
	depth = 0;
	
	for (int i = 0; i < tokens.GetCount(); i++) {
		if ( tokens[i] == "(" ) {
			depth += 1;
			continue;
		}

		if ( tokens[i] == ")" ) {
			depth -= 1;
			if (depth < 0) break;
			continue;
		}

		if ( depth == 0 && tokens[i] == "and" ) {
			and_pos = i;
			break;
		}
	}

	if ( and_pos != -1 ) {
		bool quantifier_in_left = false;
		int depth = 0;
		
		for (int i = 0; i < and_pos; i++) {
			if ( tokens[i] == "(" ) {
				depth += 1;
				continue;
			}

			if ( tokens[i] == ")" ) {
				depth -= 1;
				if (depth < 0) break;
				continue;
			}

			if ( depth == 0 && ( tokens[i] == "forall" || tokens[i] == "exists" ) ) {
				quantifier_in_left = true;
				break;
			}
		}

		if ( !quantifier_in_left ) {
			if ( and_pos == 0 || and_pos == tokens.GetCount() - 1 )
				throw InvalidInputError ( "Missing formula in AND connective." );
			
			Vector<String> tmp1, tmp2;
			for(int i = 0; i < and_pos; i++)
				tmp1.Add(tokens[i]);
			for(int i = and_pos+1; i < tokens.GetCount(); i++)
				tmp2.Add(tokens[i]);
			return new And( *Parse(tmp1), *Parse(tmp2) );
		}
	}

	// Not
	if ( tokens[0] == "not" ) {
		if ( tokens.GetCount() < 2 )
			throw InvalidInputError ( "Missing formula in NOT connective." );
		
		Vector<String> tmp;
		for(int i = 1; i < tokens.GetCount(); i++)
			tmp.Add(tokens[i]);
		return new Not ( *Parse(tmp) );
	}

	// Function
	if ( IsAlphaNumber(tokens[0]) && keywords.Find(ToLower(tokens[0])) == -1 &&
		 tokens.GetCount() > 1 && IsLower(tokens[0]) && tokens[1] == "(" ) {
		if ( tokens[tokens.GetCount() - 1] != ")" )
			throw InvalidInputError ( "Missing \")\" after function argument list." );
		
		String name = tokens[0];

		Index<NodeVar> args;
		int i = 2;

		if ( i < tokens.GetCount() - 1 ) {
			while ( i <= tokens.GetCount() - 1 ) {
				int end = tokens.GetCount() - 1;
				int depth = 0;

				for (int j = 0; j < tokens.GetCount() - 1; j++) {
					if ( tokens[j] == "(" ) {
						depth += 1;
						continue;
					}

					if ( tokens[j] == ")" ) {
						depth -= 1;
						if (depth < 0) break;
						continue;
					}

					if ( depth == 0 && tokens[j] == "," ) {
						end = j;
						break;
					}
				}

				if ( i == end )
					throw InvalidInputError ( "Missing function argument." );
				
				Vector<String> tmp;
				for(int j = i; j < end; j++)
					tmp.Add(tokens[j]);
				args.Add(Parse(tmp));

				i = end + 1;
			}
		}

		return new Function ( name, args );
	}

	// Predicate
	if ( IsAlphaNumber(tokens[0]) && keywords.Find(ToLower(tokens[0])) == -1 &&
		 tokens.GetCount() == 1 && HasUpper(tokens[0]))
		return new Predicate ( tokens[0], Index<NodeVar>() );

	if ( IsAlphaNumber(tokens[0]) && keywords.Find(ToLower(tokens[0])) == -1 &&
		 tokens.GetCount() > 1 && HasUpper(tokens[0]) && tokens[1] == "(" ) {
		if ( tokens[tokens.GetCount() - 1] != ")" )
			throw InvalidInputError ( "Missing \")\" after predicate argument list." );
		
		String name = tokens[0];

		Index<NodeVar> args;
		int i = 2;

		if ( i < tokens.GetCount() - 1 ) {
			while ( i <= tokens.GetCount() - 1 ) {
				int end = tokens.GetCount() - 1;
				int depth = 0;

				for (int j = i; j < tokens.GetCount() - 1; j++) {
					if ( tokens[j] == "(" ) {
						depth += 1;
						continue;
					}

					if ( tokens[j] == ")" ) {
						depth -= 1;
						if (depth < 0) break;
						continue;
					}

					if ( depth == 0 && tokens[j] == "," ) {
						end = j;
						break;
					}
				}

				if ( i == end )
					throw InvalidInputError ( "Missing predicate argument." );
				
				Vector<String> tmp;
				for(int j = i; j < end; j++)
					tmp.Add(tokens[j]);
				args.Add(Parse(tmp));

				i = end + 1;
			}
		}

		return new Predicate ( name, args );
	}

	// Variable
	if ( IsAlphaNumber(tokens[0]) && keywords.Find(ToLower(tokens[0])) == -1 &&
		 tokens.GetCount() == 1 && IsLower(tokens[0]) ) {
		return new Variable ( tokens[0] );
	}

	// Group
	if ( tokens[0] == "(" ) {
		if ( tokens[tokens.GetCount() - 1] != ")" )
			throw InvalidInputError ( "Missing \")\"." );
		if ( tokens.GetCount() == 2 )
			throw InvalidInputError ( "Missing formula in parenthetical group." );
		Vector<String> tmp;
		int depth = 0;
		for(int i = 1; i < tokens.GetCount()-1; i++) {
			String& t = tokens[i];
			if (t == "(") depth++;
			else if (t == ")") {if (!depth) break; depth--;}
			tmp.Add(t);
		}
		return Parse(tmp);
	}
	
	throw InvalidInputError (Format("Unable to parse: %s...", tokens[0]));
}

NodeVar UnsafeParse(String str) {
	Vector<String> tokens = Lex(str);
	return Parse(tokens);
}

NodeVar Parse(String str) {
	try {
		Vector<String> tokens = Lex(str);
		return Parse(tokens);
	}
	catch (InvalidInputError e) {
		Print(e);
	}
	return NodeVar();
}

extern String* catch_print;
extern Index<NodeVar> axioms;
extern ArrayMap<NodeVar, Index<NodeVar> > lemmas;

void ClearLogic() {
	axioms.Clear();
	lemmas.Clear();
}

String AddAxiom(String str) {
	String out;
	catch_print = &out;
	
	NodeVar n = Parse(str);
	if (n.Is()) {
		try {
			CheckFormula ( *n );
			axioms.Insert (0, n );
			return 0;
		}
		catch (...) {}
	}
	catch_print = 0;
	return out;
}

String GetAxioms() {
	String out;
	catch_print = &out;
	
	for(int i = 0; i < axioms.GetCount(); i++) {
		const NodeVar& axiom = axioms[i];
		Print(axiom->ToString());
	}
	
	catch_print = 0;
	return out;
}

extern String* catch_print;
extern Index<NodeVar> axioms;
extern ArrayMap<NodeVar, Index<NodeVar> > lemmas;

String ProveLemmaNode(NodeVar formula) {
	String out;
	catch_print = &out;
	
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
	
	catch_print = 0;
	return out;
}

String ProveLemma(String str) {
	NodeVar n = Parse(str);
	if (n.Is()) {
		try {
			CheckFormula ( *n );
			return ProveLemmaNode(n);
		}
		catch (...) {}
	}
	return "";
}

String GetLemmas() {
	String out;
	catch_print = &out;
	
	for(int i = 0; i < lemmas.GetCount(); i++) {
		const NodeVar& lemma = lemmas.GetKey(i);
		Print(lemma->ToString());
	}
	
	catch_print = 0;
	return out;
}


}
