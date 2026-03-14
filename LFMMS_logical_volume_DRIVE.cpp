// LFMMS_logical_volume_DRIVE.cpp
module LFMMS;

#include "LFMMS_dp.h"

namespace lfmms_f {
	#define EXIT_FAILURE 1

	#if defined(LFMMS_INCLUDE_TEST)
	#define __CODING_ERROR__(mes)\
		mes().out(); \
		throw mes();
	#else
	#define __CODING_ERROR__(mes)\
		mes().out();std::exit(EXIT_FAILURE);
	#endif

	#define __SYS_ERROR__(mes)\
		mes().out(); \
		throw mes();

	#define __RSM__ mes_out = mes::a_mes()        //reset mes_out
	#define __CAR__ if(mes_out.code != 0)return   //check and return

	logical_volume::DRIVE::DRIVE(logical_volume* this_ptr_in) {
		if (this_ptr_in == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		this_ptr      = this_ptr_in;
		check_        = check(this_ptr);
		head_control_ = head_control(this_ptr);
	}



};