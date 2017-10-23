#ifndef _TheoremProver_MetaNode_h_
#define _TheoremProver_MetaNode_h_

#include "Context.h"

namespace RefCore {

class PathResolver;

class MetaTime {
	Time begin, end;
	int timediff;
	int base_period;
	int begin_ts, end_ts;
	bool reversed;
	
public:
	MetaTime() : timediff(0), base_period(60), reversed(false) {}
	
	void Serialize(Stream& s) {s % begin % end % timediff % base_period % begin_ts;}
	
	int GetCount(int period) const {return timediff / base_period / period * (reversed ? -1 : 1);}
	Time GetTime(int period, int pos) const {return begin + base_period * period * pos * (reversed ? -1 : 1);}
	Time GetBegin() const {return begin;}
	Time GetEnd() const {return end;}
	int GetBeginTS() {return begin_ts;}
	int GetEndTS() {return end_ts;}
	int GetBasePeriod() const {return base_period;}
	int GetShift(int src_period, int dst_period, int shift);
	int GetShiftFromTime(int timestamp, int period);
	int GetTfFromSeconds(int period_seconds);
	bool IsReversed() const {return reversed;}
	
	void SetBegin(Time t)	{begin = t; begin_ts = (int)(t.Get() - Time(1970,1,1).Get());}
	void SetEnd(Time t)	{end = t; end_ts = (int)(t.Get() - Time(1970,1,1).Get()); timediff = (int)(end.Get() - begin.Get()); reversed = begin_ts > end_ts;}
	void SetBasePeriod(int period)	{base_period = period;}
	
};

class MetaNode : public RefCore::Ref<MetaNode> {
	PathResolver* res;
	MetaTime* dt_ptr;
	int period, bardata_id;
	
protected:
	friend class PathResolver;
	typedef RefCore::Ref<MetaNode> MetaRef;
	typedef RefCore::Var<MetaNode> MetaVar;
	
	
	MetaVar src;
	String path;
	
	void NodeInit();
	void SetMetaTime(MetaTime& dt) {dt_ptr = &dt;}
public:
	typedef MetaNode CLASSNAME;
	MetaNode();
	
	
	virtual void Serialize(Stream& s) {s % period % path;}
	
	virtual void SetArguments(const VectorMap<String, Value>& args) {}
	virtual void Init() {}
	virtual void Start() {}
	virtual void RefreshSource() {}
	virtual String GetKey() const {return "";}
	virtual String GetCtrlKey() const {return "";}
	void StartThreaded() {Thread::Start(THISBACK(Start));}
	
	int GetId() const {return bardata_id;}
	int GetPeriod() {return period;}
	int GetSecondPeriod() {return GetPeriod() * GetTime().GetBasePeriod();}
	int GetMinutePeriod() {return GetPeriod() * GetTime().GetBasePeriod() / 60;}
	MetaVar GetSource() {return src;}
	String GetPath() {return path;}
	PathResolver& GetResolver();
	MetaTime& GetTime();
	bool IsLinkedResolver() {return res;}
	
	void SetPeriod(int i) {period = i;}
	void SetSource(MetaVar src) {this->src << src; if (src.Is()) SetPeriod(src->GetPeriod()); RefreshSource();}
	void SetPath(const String& path) {this->path = path;}
	void SetId(int i) {ASSERT(bardata_id == -1 || bardata_id == i); bardata_id = i;}
	
};

typedef RefCore::Var<MetaNode> MetaVar;

struct ArrayNode : public MetaNode {
	Vector<MetaVar> sub_nodes;
};

}

#endif
