// free_part.cpp
module BMMS;

#include "BMMS_dp.h"

namespace bmms_f {
    #define EXIT_FAILURE 1

    #if defined(BMMS_INCLUDE_TEST)
    #define __CODING_ERROR__(mes)\
					    mes().out(); \
					    throw mes();
    #else
    #define __CODING_ERROR__(mes)\
					     mes().out();std::exit(EXIT_FAILURE);
    #endif
    //======================================== 构造与析构 ========================================

    free_part::free_part(
        size_t      block_size_in,
        size_t      block_count_max_in,
        init_type   init_type_in,
        mes::a_mes& mes_out
    ) : block_size(block_size_in),
        block_count_max(block_count_max_in),
        init_type_(init_type_in)
    {
        // 1 重置消息
        mes_out = mes::a_mes();

        // 2 参数校验
        check_build_value_valid(mes_out);
        if (mes_out.code != 0) return;

        // 3 开辟blocks_agent_ptr_ram_space内存
        try {
            // 注意：这里创建的是unique_ptr的数组
            block_ptr_ram_space = make_unique<unique_ptr<byte_1[]>[]>(block_count_max);
        }
        catch (...) {
            mes_out = bmms_m::err::service_err_free_part_memory_alloc_failed();
            return;
        }

        // 4 移交管理权 - 创建代理指针管理指针数组
        // block_ptr_ram_space.get()返回的是unique_ptr<byte_1[]>*
        // 但我们需要的是存储指针的数组的起始地址
        void* raw_ptr_array = reinterpret_cast<void*>(block_ptr_ram_space.get());
        size_t array_size = nf_mul(block_count_max, sizeof(void*));

        block_ptr_ram_space_agent_ptr = bmms_f::agent_ptr(raw_ptr_array, array_size);

        // 5 初始化blocks_agent_ptr_ram_space内存（全部置nullptr）
        block_ptr_ram_space_agent_ptr.clear();

        // 6 根据策略初始化块
        init_blocks(mes_out);
        if (mes_out.code != 0) {
            // 初始化失败，清理资源
            block_ptr_ram_space.reset();
            return;
        }

        // 7 成功
        is_init = true;
    }

    free_part::~free_part() {
        if (!is_init) return;

        // unique_ptr数组会自动释放所有元素
        // 每个unique_ptr<byte_1[]>会自动释放其内存
        block_ptr_ram_space.reset();
    }

    //======================================== 访问控制 ========================================

    free_part::free_part(free_part&& other) noexcept
        : is_init(std::exchange(other.is_init, false))
        , init_type_(std::exchange(other.init_type_, init_type::no_init_all_size))
        , block_size(std::exchange(other.block_size, 0))
        , block_count_max(std::exchange(other.block_count_max, 0))
        , using_block_count(std::exchange(other.using_block_count, 0))
        , block_ptr_ram_space(move(other.block_ptr_ram_space))
        , block_ptr_ram_space_agent_ptr(move(other.block_ptr_ram_space_agent_ptr))
    {
        // fill数组会被默认初始化
    }

    free_part& free_part::operator=(free_part&& other) noexcept {
        if (this != &other) {
            // 释放当前资源
            if (is_init) {
                block_ptr_ram_space.reset();
            }

            // 转移所有资源
            is_init = std::exchange(other.is_init, false);
            init_type_ = std::exchange(other.init_type_, init_type::no_init_all_size);
            block_size = std::exchange(other.block_size, 0);
            block_count_max = std::exchange(other.block_count_max, 0);
            using_block_count = std::exchange(other.using_block_count, 0);
            block_ptr_ram_space = move(other.block_ptr_ram_space);
            block_ptr_ram_space_agent_ptr = move(other.block_ptr_ram_space_agent_ptr);
        }
        return *this;
    }

    //======================================== 数据控制 ========================================

    void free_part::clear() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }

        for (size_t i = 0; i < block_count_max; i++) {
            void* block_ptr_raw = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                i * sizeof(void*),
                sizeof(void*),
                &block_ptr_raw,
                true
            );

            if (block_ptr_raw != nullptr) {
                // 清空块内容（填充0）
                bmms_f::agent_ptr block_agent_ptr(block_ptr_raw, block_size);
                block_agent_ptr.clear();
            }
        }
    }

    void free_part::clear_range(
        size_t begin_in,
        size_t size_in
    ) {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }

        // 区间值校验
        check_range(begin_in, size_in);

        // 借用static_volume_part的范围处理逻辑
        size_t index_begin_out = 0;
        size_t index_count_out = 0;
        size_t index_end = 0;
        size_t op_begin_of_first_block = 0;
        size_t op_size_of_end_block = 0;
        bool is_over_more_than_a_block = false;

        get_range_info(
            begin_in,
            size_in,
            index_begin_out,
            index_count_out,
            index_end,
            op_begin_of_first_block,
            op_size_of_end_block,
            is_over_more_than_a_block
        );

        // 处理每个涉及的块
        if (!is_over_more_than_a_block) {
            bmms_f::agent_ptr block_agent_ptr;
            bool is_nullptr = false;
            get_block_ptr(index_begin_out, block_agent_ptr, is_nullptr);
            if (!is_nullptr) {
                block_agent_ptr.clear_range(op_begin_of_first_block, size_in);
            }
        }
        else {
            // 处理第一个块
            bmms_f::agent_ptr block_agent_ptr;
            bool is_nullptr = false;

            get_block_ptr(index_begin_out, block_agent_ptr, is_nullptr);
            if (!is_nullptr) {
                size_t first_block_size = block_size - op_begin_of_first_block;
                block_agent_ptr.clear_range(op_begin_of_first_block, first_block_size);
            }

            // 处理中间块
            if (index_count_out > 2) {
                for (size_t i = index_begin_out + 1; i < index_end; i++) {
                    get_block_ptr(i, block_agent_ptr, is_nullptr);
                    if (!is_nullptr) {
                        block_agent_ptr.clear();
                    }
                }
            }

            // 处理最后一个块
            get_block_ptr(index_end, block_agent_ptr, is_nullptr);
            if (!is_nullptr) {
                block_agent_ptr.clear_range(0, op_size_of_end_block);
            }
        }
    }

    void free_part::resize(
        size_t block_count_limit_in,
        mes::a_mes& mes_out
    ) {
        // ====================== 1. 初始化校验 ======================
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }

        // ====================== 2. 参数校验 ======================
        if (block_count_limit_in == 0) {
            mes_out = bmms_m::err::service_err_free_part_resize_block_count_zero();
            return;
        }

        if (block_count_limit_in > bmms_f::index_max) {
            mes_out = bmms_m::err::coding_err_free_part_build_block_count_over_index_max();
            return;
        }

        // 如果大小不变，直接返回成功
        if (block_count_limit_in == block_count_max) {
            mes_out = mes::a_mes();
            return;
        }

        // ====================== 3. 准备变量 ======================
        mes_out = mes::a_mes(); // 重置消息

        unique_ptr<unique_ptr<byte_1[]>[]> new_block_ptr_ram_space = nullptr;
        bmms_f::agent_ptr new_agent_ptr;
        size_t new_block_count_max = block_count_limit_in;

        // ====================== 4. 分配新控制数组 ======================
        try {
            // 注意：这里创建的是 unique_ptr 的数组的 unique_ptr
            new_block_ptr_ram_space = make_unique<unique_ptr<byte_1[]>[]>(new_block_count_max);
        }
        catch (...) {
            mes_out = bmms_m::err::service_err_free_part_resize_memory_alloc_failed();
            return;
        }

        // ====================== 5. 创建新代理指针 ======================
        {
            void* raw_ptr_array = reinterpret_cast<void*>(new_block_ptr_ram_space.get());
            size_t array_size = nf_mul(new_block_count_max, sizeof(void*));

            // 通过 agent_ptr 构造函数（不是 move）
            new_agent_ptr = bmms_f::agent_ptr(raw_ptr_array, array_size);

            // 初始化新数组为全 null
            new_agent_ptr.clear();
        }

        // ====================== 6. 复制旧控制数据（通过 agent_ptr） ======================
        size_t copy_count = std::min(block_count_max, new_block_count_max);
        if (copy_count > 0) {
            // 一次复制整个区间，而不是逐指针复制
            block_ptr_ram_space_agent_ptr.load(
                0,                                   // 从旧数组开始
                nf_mul(copy_count, sizeof(void*)),   // 要复制的字节数
                new_agent_ptr,                        // 目标代理指针
                0,                                    // 从新数组开始
                true                                  // 安全检查
            );
        }

        // ====================== 7. 处理缩小情况 ======================
        if (new_block_count_max < block_count_max) {
            // 释放超出新限制的块
            for (size_t i = new_block_count_max; i < block_count_max; i++) {
                // 检查旧数组中是否有分配的块
                void* old_block_ptr = nullptr;
                block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                    nf_mul(i, sizeof(void*)),
                    sizeof(void*),
                    &old_block_ptr,
                    true
                );

                if (old_block_ptr != nullptr) {
                    // 块已分配，需要释放
                    // unique_ptr 会自动释放内存（当数组被销毁时）
                    // 但我们需要减少 using_block_count
                    using_block_count = nf_sub(using_block_count, 1);
                }
            }
        }
        // ====================== 8. 处理扩大情况 ======================
        else if (new_block_count_max > block_count_max) {
            if (init_type_ == init_type::init_all_size) {
                // 需要初始化新增的块
                for (size_t i = block_count_max; i < new_block_count_max; i++) {
                    try {
                        // 分配新块
                        new_block_ptr_ram_space[i] = make_unique<byte_1[]>(block_size);

                        // 初始化新块为0（通过代理指针）
                        void* block_raw_ptr = new_block_ptr_ram_space[i].get();
                        bmms_f::agent_ptr block_agent_ptr(block_raw_ptr, block_size);
                        block_agent_ptr.clear();

                        // 更新指针数组
                        new_agent_ptr.store_from_void_ptr(
                            &block_raw_ptr,
                            sizeof(void*),
                            nf_mul(i, sizeof(void*)),
                            true
                        );

                        using_block_count = nf_add(using_block_count, 1);
                    }
                    catch (...) {
                        // 分配失败，需要回滚已分配的新块
                        for (size_t j = block_count_max; j < i; j++) {
                            if (new_block_ptr_ram_space[j]) {
                                // 清除指针数组中的条目
                                void* null_ptr = nullptr;
                                new_agent_ptr.store_from_void_ptr(
                                    &null_ptr,
                                    sizeof(void*),
                                    nf_mul(j, sizeof(void*)),
                                    true
                                );

                                // unique_ptr 会自动释放
                                using_block_count = nf_sub(using_block_count, 1);
                            }
                        }

                        mes_out = bmms_m::err::service_err_free_part_resize_memory_alloc_failed();
                        return;
                    }
                }
            }
            // 如果是 no_init_all_size，新增块保持为 null，不需要特殊处理
        }

        // ====================== 9. 更新 using_block_count（精确计算） ======================
        // 重新计算实际的 using_block_count
        if (new_block_count_max != block_count_max) {
            size_t actual_using_count = 0;
            for (size_t i = 0; i < new_block_count_max; i++) {
                void* block_ptr = nullptr;
                new_agent_ptr.load_to_void_ptr(
                    nf_mul(i, sizeof(void*)),
                    sizeof(void*),
                    &block_ptr,
                    true
                );

                if (block_ptr != nullptr) {
                    actual_using_count = nf_add(actual_using_count, 1);
                }
            }
            using_block_count = actual_using_count;
        }

        // ====================== 10. 清理旧数据 ======================
        // 10.1 通过 agent_ptr 清除旧控制数组
        block_ptr_ram_space_agent_ptr.clear();

        // 10.2 释放旧控制数组内存（unique_ptr 自动完成）
        // 注意：这里不手动释放块内存，因为 unique_ptr 会自动释放

        // 10.3 更新成员变量
        block_ptr_ram_space = move(new_block_ptr_ram_space);
        block_ptr_ram_space_agent_ptr = move(new_agent_ptr);
        block_count_max = new_block_count_max;

        // ====================== 11. 成功返回 ======================
        mes_out = mes::a_mes();
    }

    void free_part::free_block(size_t block_index_in) {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }

        if (block_index_in >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_block_index_out_of_range)
        }

        // 获取块指针
        void* block_ptr_raw = nullptr;
        block_ptr_ram_space_agent_ptr.load_to_void_ptr(
            block_index_in * sizeof(void*),
            sizeof(void*),
            &block_ptr_raw,
            true
        );

        // 如果指针不为nullptr，释放块内存
        if (block_ptr_raw != nullptr) {
            // 通过unique_ptr释放内存
            block_ptr_ram_space[block_index_in].reset();

            // 更新指针数组
            block_ptr_raw = nullptr;
            block_ptr_ram_space_agent_ptr.store_from_void_ptr(
                &block_ptr_raw,
                sizeof(void*),
                block_index_in * sizeof(void*),
                true
            );

            // 更新使用计数
            using_block_count--;
        }
    }

    void free_part::free_range(
        size_t begin_in,
        size_t size_in
    ) {
        // 简化实现：只释放整块
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }

        check_range(begin_in, size_in);

        size_t start_block = begin_in / block_size;
        size_t end_block = (begin_in + size_in - 1) / block_size;

        // 只释放完全包含在范围内的块
        for (size_t i = start_block; i <= end_block; i++) {
            // 检查块是否完全在范围内
            size_t block_start = i * block_size;
            size_t block_end = block_start + block_size - 1;

            if (block_start >= begin_in && block_end < begin_in + size_in) {
                free_block(i);
            }
        }
    }

    //======================================== 数据交换 ========================================

    void free_part::load(
        size_t           load_begin_in,
        size_t           load_size_in,
        bmms_f::agent_ptr& save_agent_ptr_in,
        size_t           store_begin_in,
        bool             safe
    ) {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }

        if (load_size_in == 0) {
            return;
        }

        check_range(load_begin_in, load_size_in);

        size_t start_block = load_begin_in / block_size;
        size_t end_block = (load_begin_in + load_size_in - 1) / block_size;
        size_t offset_in_first_block = load_begin_in % block_size;

        size_t current_load_pos = 0;
        size_t current_store_pos = store_begin_in;
        size_t remaining = load_size_in;

        for (size_t i = start_block; i <= end_block && remaining > 0; i++) {
            void* block_ptr_raw = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                i * sizeof(void*),
                sizeof(void*),
                &block_ptr_raw,
                true
            );

            size_t bytes_in_this_block = block_size - (i == start_block ? offset_in_first_block : 0);
            bytes_in_this_block = std::min(bytes_in_this_block, remaining);

            if (block_ptr_raw != nullptr) {
                bmms_f::agent_ptr block_agent_ptr(block_ptr_raw, block_size);
                size_t block_offset = (i == start_block) ? offset_in_first_block : 0;

                block_agent_ptr.load(
                    block_offset,
                    bytes_in_this_block,
                    save_agent_ptr_in,
                    current_store_pos,
                    safe
                );
            }
            else {
                // 未分配的块，填充0
                save_agent_ptr_in.clear_range(current_store_pos, bytes_in_this_block);
            }

            current_store_pos += bytes_in_this_block;
            current_load_pos += bytes_in_this_block;
            remaining -= bytes_in_this_block;
        }
    }

    void free_part::store(
        bmms_f::agent_ptr& load_agent_ptr_in,
        size_t           load_begin_in,
        size_t           load_size_in,
        size_t           store_begin_in,
        bool             safe
    ) {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }

        if (load_size_in == 0) {
            return;
        }

        check_range(store_begin_in, load_size_in);

        size_t start_block = store_begin_in / block_size;
        size_t end_block = (store_begin_in + load_size_in - 1) / block_size;
        size_t offset_in_first_block = store_begin_in % block_size;

        size_t current_load_pos = load_begin_in;
        size_t current_store_pos = 0;
        size_t remaining = load_size_in;

        for (size_t i = start_block; i <= end_block && remaining > 0; i++) {
            // 确保块已分配
            void* block_ptr_raw = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                i * sizeof(void*),
                sizeof(void*),
                &block_ptr_raw,
                true
            );

            if (block_ptr_raw == nullptr) {
                // 分配新块
                try {
                    block_ptr_ram_space[i] = make_unique<byte_1[]>(block_size);
                    block_ptr_raw = block_ptr_ram_space[i].get();

                    // 初始化块为0
                    memset(block_ptr_raw, 0, block_size);

                    // 更新指针数组
                    block_ptr_ram_space_agent_ptr.store_from_void_ptr(
                        &block_ptr_raw,
                        sizeof(void*),
                        i * sizeof(void*),
                        true
                    );

                    using_block_count++;
                }
                catch (...) {
                    // 分配失败，中止操作
                    return;
                }
            }

            bmms_f::agent_ptr block_agent_ptr(block_ptr_raw, block_size);
            size_t block_offset = (i == start_block) ? offset_in_first_block : 0;
            size_t bytes_in_this_block = block_size - block_offset;
            bytes_in_this_block = std::min(bytes_in_this_block, remaining);

            block_agent_ptr.store(
                load_agent_ptr_in,
                current_load_pos,
                bytes_in_this_block,
                block_offset,
                safe
            );

            current_load_pos += bytes_in_this_block;
            current_store_pos += bytes_in_this_block;
            remaining -= bytes_in_this_block;
        }
    }

    void free_part::load_to_void_ptr(
        size_t load_begin_in,
        size_t load_size_in,
        void* store_ptr_in,
        bool   safe
    ) {
        bmms_f::agent_ptr temp_agent_ptr(store_ptr_in, load_size_in);
        load(load_begin_in, load_size_in, temp_agent_ptr, 0, safe);
    }

    void free_part::store_from_void_ptr(
        void* load_ptr_in,
        size_t load_size_in,
        size_t store_begin_in,
        bool   safe
    ) {
        bmms_f::agent_ptr temp_agent_ptr(load_ptr_in, load_size_in);
        store(temp_agent_ptr, 0, load_size_in, store_begin_in, safe);
    }

    //======================================== 数据获取 ========================================

    size_t free_part::get_total_size() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }
        return block_size * block_count_max;
    }

    size_t free_part::get_block_count_max() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }
        return block_count_max;
    }

    size_t free_part::get_block_size() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }
        return block_size;
    }

    size_t free_part::get_block_count_used() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }
        return using_block_count;
    }

    void free_part::get_block_ptr(
        size_t& block_index_in,
        bmms_f::agent_ptr& block_agent_ptr_out,
        bool& is_nullptr_out
    ) {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }

        if (block_index_in >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_range_out_of_range)
        }

        void* block_ptr = nullptr;
        block_ptr_ram_space_agent_ptr.load_to_void_ptr(
            block_index_in * sizeof(void*),
            sizeof(void*),
            &block_ptr,
            true
        );

        is_nullptr_out = (block_ptr == nullptr);

        if (!is_nullptr_out) {
            block_agent_ptr_out = bmms_f::agent_ptr(block_ptr, block_size);
        }
        else {
            block_agent_ptr_out = bmms_f::agent_ptr();
        }
    }

    void free_part::get_range_info(
        size_t& op_begin_in,
        size_t& op_size_in,
        size_t& index_begin_out,
        size_t& index_count_out,
        size_t& index_end_out,
        size_t& op_begin_of_first_block_out,
        size_t& op_size_of_end_block_out,
        bool& is_over_more_than_a_block_out
    ) {
        size_t op_end = op_begin_in + op_size_in;
        size_t end_block_index = (op_end - 1) / block_size;

        index_begin_out = op_begin_in / block_size;
        op_begin_of_first_block_out = op_begin_in % block_size;
        index_end_out = end_block_index;
        index_count_out = end_block_index - index_begin_out + 1;

        if (op_end % block_size != 0) {
            op_size_of_end_block_out = op_end % block_size;
        }
        else {
            op_size_of_end_block_out = block_size;
        }

        is_over_more_than_a_block_out = (index_begin_out != end_block_index);
    }

    uintptr_t free_part::__block_ptr_ram_space_begin__() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }
        return reinterpret_cast<uintptr_t>(block_ptr_ram_space.get());
    }

    uintptr_t free_part::__block_ptr_ram_space_end__() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_not_inited)
        }
        return __block_ptr_ram_space_begin__() + (block_count_max * sizeof(void*));
    }

    //======================================== 校验 ========================================

    void free_part::check_build_value_valid(mes::a_mes& mes_out) {
        if (block_size == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_build_block_size_zero)
        }

        if (block_count_max == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_build_block_count_zero)
        }

        if (block_count_max > bmms_f::index_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_build_block_count_over_index_max)
        }

        if (init_type_ != init_type::no_init_all_size &&
            init_type_ != init_type::init_all_size) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_build_init_type_invalid)
        }
    }

    void free_part::check_contro_valid(mes::a_mes& mes_out) {
        // free_part没有控制策略
    }

    void free_part::check_range(
        size_t begin_in,
        size_t size_in
    ) {
        if (size_in == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_range_zero)
        }
        if (begin_in + size_in > get_total_size()) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_range_out_of_range)
        }
    }

    //======================================== 辅助 ========================================

    size_t free_part::nf_add(size_t a, size_t b) {
        if (a > SIZE_MAX - b) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_add_overflow);
        }
        return a + b;
    }

    size_t free_part::nf_sub(size_t a, size_t b) {
        if (a < b) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_sub_overflow);
        }
        return a - b;
    }

    size_t free_part::nf_mul(size_t a, size_t b) {
        if (b != 0 && a > SIZE_MAX / b) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_mul_overflow);
        }
        return a * b;
    }

    size_t free_part::nf_div(size_t a, size_t b) {
        if (b == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_div_by_zero);
        }
        return a / b;
    }

    size_t free_part::nf_mod(size_t a, size_t b) {
        if (b == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_part_mod_by_zero);
        }
        if (b == 1) {
            return 0;
        }
        if ((b & (b - 1)) == 0) {
            return a & (b - 1);
        }
        return a % b;
    }

    void free_part::init_blocks(mes::a_mes& mes_out) {
        mes_out = mes::a_mes();

        if (init_type_ == init_type::init_all_size) {
            for (size_t i = 0; i < block_count_max; i++) {
                try {
                    block_ptr_ram_space[i] = make_unique<byte_1[]>(block_size);
                    void* block_ptr_raw = block_ptr_ram_space[i].get();

                    // 将指针写入数组
                    block_ptr_ram_space_agent_ptr.store_from_void_ptr(
                        &block_ptr_raw,
                        sizeof(void*),
                        i * sizeof(void*),
                        true
                    );

                    // 初始化块内容为0
                    memset(block_ptr_raw, 0, block_size);

                    using_block_count++;
                }
                catch (...) {
                    // 分配失败，需要释放已分配的块
                    for (size_t j = 0; j < i; j++) {
                        block_ptr_ram_space[j].reset();
                    }
                    mes_out = bmms_m::err::service_err_free_part_memory_alloc_failed();
                    return;
                }
            }
        }
        else if (init_type_ == init_type::no_init_all_size) {
            using_block_count = 0;
        }
    }

}