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

    cache_RAII_lock::cache_RAII_lock(
        shared_ptr<handle>& handle_ptr_in,
        cache_agent_ptr&    cache_agent_ptr_out,
        mes::a_mes&         mes_out
    ):  handle_ptr(handle_ptr_in),
        cache_agent_ptr_ptr(&cache_agent_ptr_out)
    {
        //1 重置消息
        mes_out = mes::a_mes();

        //1 校验空指针
        if (handle_ptr == nullptr) {
            __CODING_ERROR__(lfmms_m::err::coding_error_cache_RAII_lock_build_from_null_ptr)
        }

        //2 校验是否初始化
        if (!handle_ptr->get_is_init()) {
            __CODING_ERROR__(lfmms_m::err::coding_error_cache_RAII_lock_build_from_not_init_handle)
        }

        //3 尝试加锁
        handle_ptr->try_lock_cache(mes_out);
        if (mes_out.code != 0) {
            handle_ptr = nullptr;
            cache_agent_ptr_ptr = nullptr;
            return;
        }

        //4 尝试建立指针
        handle_ptr->try_get_cache_agent_ptr(
            cache_agent_ptr_out,
            mes_out
        );
        if (mes_out.code != 0) {
            handle_ptr->try_unlock_cache();
            handle_ptr = nullptr;
            cache_agent_ptr_ptr = nullptr;
            return;
        }
    };

    cache_RAII_lock::~cache_RAII_lock() { 
        //1 校验空指针
        if (handle_ptr          == nullptr) {
            return;
        }
        if (cache_agent_ptr_ptr == nullptr) {
            return;
        }
        //2 释放指针
        cache_agent_ptr_ptr->release();
        //3 释放缓存锁
        handle_ptr->try_unlock_cache();
    };
};