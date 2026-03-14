// LFMMS_logical_volume_head_control.cpp
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

	bool handle::get_is_init() {
		return (block_version != 0 && index != 0 && logical_volume_ptr != nullptr);
	}

	void handle::try_lock_cache(mes::a_mes& mes_out) {
	
	};
	//零拷贝访问缓存
	void handle::try_get_cache_agent_ptr(
		cache_agent_ptr& cache_agent_ptr_out,
		mes::a_mes& mes_out
	) {
		
	};
	//解锁缓存
	void handle::try_unlock_cache() {
		
	};

};