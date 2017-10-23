#include "RefCore.h"

namespace RefCore {

PathResolverVar GetPathResolver() {
	static PathResolverVar var;
	if (var.Is())
		return var;
	var = new PathResolver();
	return var;
}




RefError::RefError() {
	#ifdef flagDEBUG
	Panic("RefError");
	#endif
}

RefError::RefError(const String& msg) : Exc(msg) {
	#ifdef flagDEBUG
	Panic("RefError: " + msg);
	#endif
}

String EncodePath(const PathArgs& path) {
	String out;
	for(int i = 0; i < path.GetCount(); i++)
		out << "/" << EncodePath(path, i);
	return out;
}

String EncodePath(const PathArgs& path, int pos) {
	String out = path[pos].a;
	const VectorMap<String, Value>& args = path[pos].b;
	for(int i = 0; i < args.GetCount(); i++) {
		out.Cat(i == 0 ? '?' : '&');
		out += args.GetKey(i) + "=";
		const Value& val = args[i];
		int type = val.GetType();
		if (type == INT_V)
			out += IntStr(val);
		else if (type == DOUBLE_V)
			out += DblStr(val);
		else
			out += "\"" + val.ToString() + "\"";
	}
	return out;
}













PathResolver::PathResolver() {
	id_counter = 0;
	
}

MetaTime& PathResolver::GetReverseTime() {
	if (rev_dt.IsReversed()) return rev_dt;
	
	// Init reverse MetaTIme
	rev_dt.SetBegin(dt.GetEnd());
	rev_dt.SetEnd(dt.GetBegin());
	rev_dt.SetBasePeriod(dt.GetBasePeriod());
	ASSERT(rev_dt.IsReversed());
	
	return rev_dt;
}

void PathResolver::SetPersistency(String path) {
	
}

void PathResolver::Serialize(Stream& s) {
	
	if (s.IsStoring()) {
		
		// Store all existing resolved and linked MetaNodes
		s % linked_paths;
		int count = resolved_cache.GetCount();
		s % count;
		for(int i = 0; i < count; i++) {
			String tmp = resolved_cache.GetKey(i);
			s % tmp;
			resolved_cache[i]->Serialize(s);
		}
		
	}
	else if (s.IsLoading()) {
		
		// Link new paths
		VectorMap<String, String> tmp_linked_paths;
		s % tmp_linked_paths;
		for(int i = 0; i < tmp_linked_paths.GetCount(); i++) {
			LinkPath(tmp_linked_paths.GetKey(i), tmp_linked_paths[i]);
		}
		
		// Load content of all resolved MetaNodes
		int count;
		s % count;
		for(int i = 0; i < count; i++) {
			String key;
			s % key;
			MetaVar dv = ResolvePath(key);
			dv->Serialize(s);
		}
	}
}

bool PathResolver::ParsePath(String path, PathArgs& parsed_path) {
	PathLexer lex(path);
	try {
		int depth = 0;
		while (lex.tk != LEX_EOF) {
			String id;
			
			if (depth == 0)
				lex.Match(LEX_SLASH);
			else
				lex.Match(',');
			
			if (lex.tk == LEX_ARRAYBEGIN && depth == 0) {
				lex.Match(LEX_ARRAYBEGIN);
				depth++;
				parsed_path.Add().a = "__array__";
			}
			
			id = lex.tk_str;
			lex.Match(LEX_ID);
			
			
			// Parse arguments
			PathArg& arg = parsed_path.Add();
			arg.a = id;
			VectorMap<String, Value>& args = arg.b;
			
			if (lex.tk == LEX_ARGBEGIN) {
				lex.Match(LEX_ARGBEGIN);
				int arg_count = 0;
				while (lex.tk != LEX_SLASH && lex.tk != LEX_EOF && lex.tk != ',' && lex.tk != LEX_ARRAYEND) {
					if (arg_count)
						lex.Match(LEX_ARGNEXT);
					String id = lex.tk_str;
					lex.Match(LEX_ID);
					lex.Match(LEX_ASSIGN);
					
					if (lex.tk == '-') {
						lex.Match(lex.tk);
						String s = lex.tk_str;
						if (lex.tk == LEX_INT)
							lex.Match(LEX_INT);
						else
							lex.Match(LEX_FLOAT);
						args.Add(id, Value(-StrDbl(s)));
					}
					else if (lex.tk == LEX_INT) {
						args.Add(id, Value(StrInt(lex.tk_str)));
						lex.Match(LEX_INT);
					}
					else if (lex.tk == LEX_FLOAT) {
						args.Add(id, Value(StrDbl(lex.tk_str)));
						lex.Match(LEX_FLOAT);
					}
					else if (lex.tk == LEX_STR) {
						args.Add(id, Value(lex.tk_str));
						lex.Match(LEX_STR);
					}
					arg_count++;
				}
			}
			
			SortByKey(args);
			
			if (lex.tk == LEX_ARRAYEND && depth) {
				lex.Match(LEX_ARRAYEND);
				lex.Match(LEX_EOF);
				break;
			}
		}
	}
	catch (Exc e) {
		LOG("PathResolver::ParsePath error: " + e);
		last_error = e;
		return false;
	}
	return true;
}

MetaVar PathResolver::ResolvePath(String path) {
	last_error.Clear();
	
	MetaVar out;
	
	PathArgs parsed_path;
	if (!ParsePath(path, parsed_path))
		return out;
	//DUMPM(parsed_path);
	
	return ResolvePath(path, parsed_path);
}

MetaVar PathResolver::ResolvePath(String path, const PathArgs& parsed_path) {
	MetaVar out;
	
	
	
	
	
	// Try to find full path first (required for resolving existing paths with array)
	String full_path = EncodePath(parsed_path);
	int i = resolved_cache.Find(full_path);
	if (i != -1)
		return resolved_cache[i];
	
	
	// Check properties of path
	int array_pos = -1;
	for(int i = 0; i < parsed_path.GetCount(); i++) {
		if (parsed_path[i].a == "__array__") {
			array_pos = i;
			break;
		}
	}
	
	
	// Find linked paths
	i = 0;
	String seeked_path;
	{
		PathLink* existing_link = 0;
		String tmp_path;
		int i2 = -1;
		for(int j = i; j < parsed_path.GetCount(); j++) {
			if (j == array_pos) break;
			const VectorMap<String, Value>& args = parsed_path[j].b;
			if (args.GetCount() != 0)
				break;
			const String& key = parsed_path[j].a;
			tmp_path = tmp_path + "/" + key;
			PathLink* link = FindLinkPath(tmp_path);
			if (!link) continue;
			if (link->path.GetCount()) {
				existing_link = link;
				i2 = j+1;
			}
		}
		if (existing_link) {
			i = i2;
			do {
				seeked_path = existing_link->path;
				out = existing_link->link;
				if (seeked_path.Find("?") != -1) break; // no arguments in links, so dont try to find
				existing_link = FindLinkPath(seeked_path);
			}
			while (existing_link);
		}
	}
	
	
	// Find already resolved paths
	for(; i < parsed_path.GetCount(); i++) {
		if (i == array_pos) break;
		String tmp_path = seeked_path + "/" + EncodePath(parsed_path, i);
		int j = resolved_cache.Find(tmp_path);
		if (j == -1) break;
		out = resolved_cache[j];
		seeked_path = tmp_path;
	}
	
	
	// Create remaining part of the path
	ArrayNode* array = 0;
	for(; i < parsed_path.GetCount(); i++) {
		MetaVar next;
		String path;
		
		if (i == array_pos) {
			array = new ArrayNode();
			array->SetSource(out);
			array->SetPath(seeked_path + "/__array__");
		}
		else {
			const String& id = parsed_path[i].a;
			const VectorMap<String, Value>& args = parsed_path[i].b;
			
			int j = GetFactories().Find(id);
			if (j == -1) {
				LOG("PathResolver::ResolvePath: '" + id + "' not found");
				return MetaVar(); // return empty reference when error
			}
			
			MetaFormatFactory fac = GetFactories()[j];
			next = fac();
			
			ASSERT(next.Is());
			
			int r = 0;
			
			if (i == 0 && !out.Is())
				next->SetSource(this);
			else
				next->SetSource(out);
			
			path = seeked_path + "/" + EncodePath(parsed_path, i);
			next->SetPath(path);
			
			try {
				next->SetArguments(args);
				
				resolved_cache.Add(path, next);
				next->MetaNode::NodeInit();
				next->Init();
			}
			catch (RefError e) {
				resolved_cache.RemoveKey(path);
				LOG("Error initializing " << id);
				throw RefError(); // return empty reference when error
			}
			
		}
		
		if (array_pos == -1 || i < array_pos) {
			seeked_path = path;
			out = next;
		}
		else if (i > array_pos) {
			array->sub_nodes.Add(next);
		}
	}
	
	if (array)
		out = array;
	
	return out;
}

void PathResolver::AddFormat(String key, MetaFormatFactory f) {
	GetFactories().Add(key, f);
}

void PathResolver::RegisterTest(String key, String links) {
	LinkList& ll = GetTestLinkList().Add();
	Vector<String> link_lines = Split(links, "\n");
	for(int i = 0; i < link_lines.GetCount(); i++) {
		const String& line = link_lines[i];
		int j = line.Find(",");
		if (j == -1) return;
		Tuple2<String, String>& item = ll.Add();
		item.a = line.Left(j);
		item.b = line.Mid(j+1);
	}
}

bool PathResolver::LinkPath(String dest, String src) {
	int i = linked_paths.Find(dest);
	if (i != -1) {
		String existing = linked_paths[i];
		ASSERT(existing == src);
		return true;
	}
	
	linked_paths.Add(dest, src);
	
	PathLexer lex_dest(dest);
	
	// Parse source
	PathArgs src_path;
	if (!ParsePath(src, src_path)) {
		LOG("PathResolver::LinkPath: failed to parse source: " + src);
		return false;
	}
	
	MetaVar src_var = ResolvePath(src, src_path);
	if (!src_var.Is()) {
		LOG("PathResolver::LinkPath: failed to resolve source: " + src);
		return false;
	}
	
	// Parse destination
	Vector<String> dest_path;
	try {
		while (lex_dest.tk != LEX_EOF) {
			lex_dest.Match(LEX_SLASH);
			String id = lex_dest.tk_str;
			if (lex_dest.tk == LEX_ID)	lex_dest.Match(LEX_ID);
			else						lex_dest.Match(LEX_INT);
			dest_path.Add(id);
		}
	}
	catch (Exc e) {
		last_error = e;
		LOG("PathResolver::LinkPath: failed to resolve destination: " + dest + " (" + last_error  + ")");
		return false;
	}
	
	PathLink* p = &link_root;
	String path;
	for(int i = 0; i < dest_path.GetCount(); i++) {
		const String& s = dest_path[i];
		path += "/" + s;
		p = &p->keys.GetAdd(s);
		if (!p->link.Is()) {
			p->link = new MetaNode();
			p->link->SetPath(path);
		}
	}
	
	p->path = EncodePath(src_path);
	p->link = src_var;
	
	return true;
}

PathLink* PathResolver::FindLinkPath(String path) {
	if (path == "/")
		return &link_root;
	
	PathLexer lex(path);
	Vector<String> dest_path;
	try {
		while (lex.tk != LEX_EOF) {
			lex.Match(LEX_SLASH);
			String id = lex.tk_str;
			if (lex.tk == LEX_ID)	lex.Match(LEX_ID);
			else					lex.Match(LEX_INT);
			dest_path.Add(id);
		}
	}
	catch (Exc e) {
		last_error = e;
		LOG("PathResolver::FindLinkPath: failed to resolve path: " + path + " (" + last_error  + ")");
		return 0;
	}
	
	PathLink* p = &link_root;
	for(int i = 0; i < dest_path.GetCount(); i++) {
		const String& s = dest_path[i];
		int j = p->keys.Find(s);
		if (j == -1)
			return 0;
		p = &p->keys[j];
	}
	return p;
}

















inline bool IsWhitespace ( char ch ) {
	return ( ch == ' ' ) || ( ch == '\t' ) || ( ch == '\n' ) || ( ch == '\r' );
}

inline bool IsNumeric ( char ch ) {
	return ( ch >= '0' ) && ( ch <= '9' );
}

inline bool IsPositiveNumber ( const String &str ) {
	for ( int i = 0;i < str.GetCount();i++ )
		if ( !IsNumeric ( str[i] ) )
			return false;
	return true;
}

inline bool IsHexadecimal ( char ch ) {
	return ( ( ch >= '0' ) && ( ch <= '9' ) ) ||
		   ( ( ch >= 'a' ) && ( ch <= 'f' ) ) ||
		   ( ( ch >= 'A' ) && ( ch <= 'F' ) );
}

inline bool IsAlpha ( char ch ) {
	return ( ( ch >= 'a' ) && ( ch <= 'z' ) ) || ( ( ch >= 'A' ) && ( ch <= 'Z' ) ) || ch == '_';
}

inline bool IsIDString ( const char *s ) {
	if ( !IsAlpha ( *s ) )
		return false;
	while ( *s )
	{
		if ( ! ( IsAlpha ( *s ) || IsNumeric ( *s ) ) )
			return false;

		s++;
	}
	return true;
}

PathLexer::PathLexer(String input) {
	data = input;
	data_owned = true;
	data_start = 0;
	data_end = data.GetCount();
	Reset();
}

void PathLexer::GetNextCh() {
	curr_ch = next_ch;
	
	if ( data_pos < data_end )
		next_ch = data[data_pos];
	else
		next_ch = 0;

	data_pos++;
	
}

void PathLexer::GetNextToken() {
	tk = LEX_EOF;
	tk_str.Clear();
	
	while ( curr_ch && IsWhitespace ( curr_ch ) )
		GetNextCh();
	
	token_start = data_pos - 2;

	if ( IsAlpha ( curr_ch ) )
	{

		while ( IsAlpha ( curr_ch ) || IsNumeric ( curr_ch ) )
		{
			tk_str += curr_ch;
			GetNextCh();
		}

		tk = LEX_ID;
		
	}

	else
	if ( IsNumeric ( curr_ch ) )
	{
		bool isHex = false;

		if ( curr_ch == '0' )
		{
			tk_str += curr_ch;
			GetNextCh();
		}

		if ( curr_ch == 'x' )
		{
			isHex = true;
			tk_str += curr_ch;
			GetNextCh();
		}

		tk = LEX_INT;

		while ( IsNumeric ( curr_ch ) || ( isHex && IsHexadecimal ( curr_ch ) ) )
		{
			tk_str += curr_ch;
			GetNextCh();
		}

		if ( !isHex && curr_ch == '.' )
		{
			tk = LEX_FLOAT;
			tk_str += '.';
			GetNextCh();

			while ( IsNumeric ( curr_ch ) )
			{
				tk_str += curr_ch;
				GetNextCh();
			}
		}

		if ( !isHex && ( curr_ch == 'e' || curr_ch == 'E' ) )
		{
			tk = LEX_FLOAT;
			tk_str += curr_ch;
			GetNextCh();

			if ( curr_ch == '-' )
			{
				tk_str += curr_ch;
				GetNextCh();
			}

			while ( IsNumeric ( curr_ch ) )
			{
				tk_str += curr_ch;
				GetNextCh();
			}
		}
	}

	else
	if ( curr_ch == '"' )
	{
		GetNextCh();

		while ( curr_ch && curr_ch != '"' )
		{
			if ( curr_ch == '\\' )
			{
				GetNextCh();

				switch ( curr_ch )
				{

				case 'n' :
					tk_str += '\n';
					break;

				case '"' :
					tk_str += '"';
					break;

				case '\\' :
					tk_str += '\\';
					break;

				default:
					tk_str += curr_ch;
				}
			}

			else
			{
				tk_str += curr_ch;
			}

			GetNextCh();
		}

		GetNextCh();

		tk = LEX_STR;
	}
	
	else
	{
		tk = curr_ch;
		
		if ( tk == '=' ) {
			tk = LEX_ASSIGN;
			GetNextCh();
		}
		else if ( tk == '/' ) {
			tk = LEX_SLASH;
			GetNextCh();
		}
		else if ( tk == '?' ) {
			tk = LEX_ARGBEGIN;
			GetNextCh();
		}
		else if ( tk == '&' ) {
			tk = LEX_ARGNEXT;
			GetNextCh();
		}
		else if ( tk == '[' ) {
			tk = LEX_ARRAYBEGIN;
			GetNextCh();
		}
		else if ( tk == ']' ) {
			tk = LEX_ARRAYEND;
			GetNextCh();
		}
		else if ( tk == '-' || tk == ',' ) {
			GetNextCh();
		}
	}

	token_last_end = token_end;

	token_end = data_pos - 3;
}

void PathLexer::Match(int expected_tk) {
	if ( tk != expected_tk )
	{
		String token_str = GetTokenStr(expected_tk);
		String msg;
		msg << "Got " << GetTokenStr ( tk ) << ", expected " << token_str
			<< " at " << data_pos;
		throw Exc ( msg );
	}

	GetNextToken();
}

String PathLexer::GetTokenStr(int token) {
	if ( token > 32 && token < 128 )
	{
		char buf[4] = "' '";
		buf[1] = ( char ) token;
		return buf;
	}

	switch ( token )
	{
		case LEX_EOF:
			return "EOF";
		case LEX_ID:
			return "ID";
		case LEX_INT:
			return "INT";
		case LEX_FLOAT:
			return "FLOAT";
		case LEX_STR:
			return "STRING";
		case LEX_ASSIGN:
			return "=";
		case LEX_SLASH:
			return "/";
		case LEX_ARGBEGIN:
			return "?";
		case LEX_ARGNEXT:
			return "&";
	}
	return "";
}

void PathLexer::Reset() {
	data_pos = data_start;
	token_start = 0;
	token_end = 0;
	token_last_end = 0;
	tk = 0;
	GetNextCh();
	GetNextCh();
	GetNextToken();
}

}
