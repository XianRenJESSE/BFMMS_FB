module LFMMS;

#include "LFMMS_dp.h"

namespace lfmms_f {
	void cache_agent_ptr::load(
        size_t load_begin_in,
        size_t load_size_in,
        cache_agent_ptr& save_agent_ptr_in,
        size_t store_begin_in,
        bool safe
    ) {
        // 转换 cache_agent_ptr 到 agent_ptr 的引用
        bmms_f::agent_ptr::load(
            load_begin_in,
            load_size_in,
            static_cast<bmms_f::agent_ptr&>(save_agent_ptr_in),
            store_begin_in, safe
        );
    }

    void cache_agent_ptr::store(
        cache_agent_ptr& load_agent_ptr_in,
        size_t load_begin_in,
        size_t load_size_in,
        size_t store_begin_in,
        bool safe
    ) {
        bmms_f::agent_ptr::store(
            static_cast<bmms_f::agent_ptr&>(load_agent_ptr_in),
            load_begin_in,
            load_size_in,
            store_begin_in,
            safe
        );
    }

    void cache_agent_ptr::release() {
        if (raw_ptr != nullptr) {
            raw_ptr = nullptr;
            if (made_void_ptr_ptr != nullptr) {
                *static_cast<uintptr_t*>(made_void_ptr_ptr) = 0;
            }
            size = 0;
        }
    }

};