#include "RefCore.h"

namespace RefCore {
	
//#ifdef flagDEBUG
int __var_id_counter;
int __var_break_id;
int __var_break_id_chk;
int __var_break_id_dtor;
bool __var_break_null_dtor;
int __ref_id_counter;
int __ref_break_id;
int __ref_break_id_dec;
int __ref_break_id_dtor;
//#endif

}

#ifdef flagREFCORE_TESTS
CONSOLE_APP_MAIN {
	
}
#endif
