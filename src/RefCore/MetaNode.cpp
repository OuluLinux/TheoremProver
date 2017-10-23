#include "RefCore.h"

namespace RefCore {


int MetaTime::GetShift(int src_period, int dst_period, int shift) {
	int64 timediff = shift * src_period; //		* base_period
	return (int)(timediff / dst_period); //			/ base_period
}

int MetaTime::GetShiftFromTime(int timestamp, int period) {
	int64 timediff = timestamp - begin_ts;
	return (int)(timediff / period / base_period * (reversed ? -1 : 1));
}

int MetaTime::GetTfFromSeconds(int period_seconds) {
	return period_seconds / base_period;
}










MetaNode::MetaNode() : MetaRef(&GetRefContext()) {
	period = 1;
	res = NULL;
	bardata_id = -1;
	dt_ptr = NULL;
}

void MetaNode::NodeInit() {
	// Already inited (not implemented virtual function) TODO: clean, but don't use different Init
	if (res) return;
	
	bool try_to_set_id = bardata_id == -1;
	MetaVar src = this->src;
	
	while (src.Is()) {
		// Resolve PathResolver
		if (!src->src.Is()) {
			res = src.Get<PathResolver>();
			break;
		} else {
			src = src->src;
		}
	}
}

PathResolver& MetaNode::GetResolver() {
	ASSERT(res); // Error here means that MetaNode::Init is not called early enough in the inheriting virtual Init
	return *res;
}

MetaTime& MetaNode::GetTime() {
	if (dt_ptr) return *dt_ptr;
	return GetResolver().GetTime();
}



}
