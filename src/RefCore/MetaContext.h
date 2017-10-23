#ifndef _TheoremProver_MetaContext_h_
#define _TheoremProver_MetaContext_h_

#include "MetaNode.h"

namespace RefCore {

struct RefError : public Exc {
	RefError();
	RefError(const String& msg);
};

#define ASSERTREF(x) if (!(x)) throw RefError(#x);
#define ASSERTREF_(x, msg) if (!(x)) throw RefError(msg);

enum LEX_TYPES {
    LEX_EOF = 0,
    LEX_ID = 256,
    LEX_INT,
    LEX_FLOAT,
    LEX_STR,
    
    LEX_ASSIGN,
    LEX_SLASH,
    LEX_ARGBEGIN,
    LEX_ARGNEXT,
    LEX_ARRAYBEGIN,
    LEX_ARRAYEND
};

class PathLexer {
	
public:
	
	char curr_ch, next_ch;
    int tk;
    int token_start;
    int token_end;
    int token_last_end;
    String tk_str;
    String data;
    int data_start, data_end;
    bool data_owned;
    int data_pos;
    
public:
	PathLexer(String input);
	
    void GetNextCh();
    void GetNextToken();
    
    void Match(int expected_tk);
    static String GetTokenStr(int token);
    void Reset();
    
};

struct PathLink : Moveable<PathLink> {
	MetaVar link;
	String path;
	VectorMap<String, PathLink> keys;
};

typedef Tuple2<String, VectorMap<String, Value> > PathArg;
typedef Vector<PathArg> PathArgs;

String EncodePath(const PathArgs& path);
String EncodePath(const PathArgs& path, int pos);

class PathResolver : public MetaNode {
	
protected:
	MetaTime dt, rev_dt;
	int id_counter;
	
	String last_error;
	
	VectorMap<String, String> linked_paths;
	VectorMap<String, MetaVar> resolved_cache;
	
	PathLink link_root;
	
	typedef MetaVar (*MetaFormatFactory)();
	inline static VectorMap<String, MetaFormatFactory>& GetFactories() {return Single<VectorMap<String, MetaFormatFactory> >();}
	static void AddFormat(String key, MetaFormatFactory f);
	template <class T> static MetaVar FactoryFn() { return new T; }
	
public:
	PathResolver();
	
	void Serialize(Stream& s);
	
	MetaTime& GetTime() {return dt;}
	MetaTime& GetReverseTime();
	int GetNewId() {return id_counter++;}
	
	void SetPersistency(String path);
	inline void SetMetaBegin(Time t) {GetTime().SetBegin(t);}
	inline void SetMetaEnd(Time t) {GetTime().SetEnd(t);}
	
	bool LinkPath(String dest, String src);
	PathLink* FindLinkPath(String path);
	bool ParsePath(String path, PathArgs& out);
	
	MetaVar ResolvePath(String path);
	MetaVar ResolvePath(String path, const PathArgs& parsed_path);
	String GetLastError() {return last_error;}
	
	template <class T> static void Register() {
		MetaVar var = new T();
		String key = var->GetKey();
		ASSERTREF_(GetFactories().Find(key) == -1, "Key " + T().GetKey() + " redefined");
		AddFormat(key, &PathResolver::FactoryFn<T>);
	}
	template <class T> static void RegisterStatic() {
		ASSERTREF_(GetFactories().Find(T::GetKeyStatic()) == -1, "Key " + T::GetKeyStatic() + " redefined");
		AddFormat(T::GetKeyStatic(), &PathResolver::FactoryFn<T>);
	}
	static void RegisterTest(String key, String links);
	typedef Vector<Tuple2<String, String> > LinkList;
	inline static Vector<LinkList>& GetTestLinkList() {return Single<Vector<LinkList> >();}
	
};

typedef RefCore::Var<PathResolver> PathResolverVar;
PathResolverVar GetPathResolver();


void ClearPathResolverCache();

}

#endif
