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

    //======================================== 访问控制 ========================================

    free_volume::free_volume(free_volume&& other) noexcept
        : is_init(other.is_init)
        , init_type_(other.init_type_)
        , alignment_type_(other.alignment_type_)
        , block_size(other.block_size)
        , block_count_max(other.block_count_max)
        , using_block_count(other.using_block_count)
        , actually_block_size(other.actually_block_size)
        , recycle_stack_top_ptr(other.recycle_stack_top_ptr)
        , stack_ram_space(move(other.stack_ram_space))
        , block_ptr_ram_space(move(other.block_ptr_ram_space))
        , block_ram_space(move(other.block_ram_space))
        , stack_index_ram_space(move(other.stack_index_ram_space))
    {
        // 重置原对象
        other.is_init = false;
        other.block_size = 0;
        other.block_count_max = 0;
        other.using_block_count = 0;
        other.actually_block_size = 0;
        other.recycle_stack_top_ptr = nullptr;
    }

    free_volume& free_volume::operator=(free_volume&& other) noexcept {
        if (this != &other) {
            // 移动所有成员
            is_init = other.is_init;
            init_type_ = other.init_type_;
            alignment_type_ = other.alignment_type_;
            block_size = other.block_size;
            block_count_max = other.block_count_max;
            using_block_count = other.using_block_count;
            actually_block_size = other.actually_block_size;
            recycle_stack_top_ptr = other.recycle_stack_top_ptr;
            stack_ram_space = move(other.stack_ram_space);
            block_ptr_ram_space = move(other.block_ptr_ram_space);
            block_ram_space = move(other.block_ram_space);
            stack_index_ram_space = move(other.stack_index_ram_space);

            // 重置原对象
            other.is_init = false;
            other.block_size = 0;
            other.block_count_max = 0;
            other.using_block_count = 0;
            other.actually_block_size = 0;
            other.recycle_stack_top_ptr = nullptr;
        }
        return *this;
    }

    //======================================== 构造与析构 ========================================

    // 参数构造函数
    free_volume::free_volume(
        size_t         block_size_in,
        size_t         block_count_max_in,
        init_type      init_type_in,
        alignment_type alignment_type_in,
        mes::a_mes& mes_out
    ):  block_size(block_size_in),
        block_count_max(block_count_max_in),
        init_type_(init_type_in),
        alignment_type_(alignment_type_in)
    {
        //1 重置消息
        mes_out = mes::a_mes();

        //2 校验初始化参数
        check_init_value_valid();

        //3 写入真实块大小
        actually_block_size = __actually_block_size__();

        //4 分配内存
        try {
            stack_ram_space = move(make_unique<byte_1[]>(__stack_space_size__()));
            block_ptr_ram_space = move(make_unique<unique_ptr<byte_1[]>[]>(__block_ptr_space_size__()));
        }
        catch (...) {
            mes_out = bmms_m::err::service_err_static_volume_memory_alloc_failed();
            return;
        }

        //5 移交内存管理权
        block_ram_space = move(
            bmms_f::agent_ptr(
                static_cast<void*>(block_ptr_ram_space.get()),
                __block_ptr_space_size__()
            )
        );
        stack_index_ram_space = move(
            bmms_f::agent_ptr(
                reinterpret_cast<void*>(stack_ram_space.get()),
                __recycle_stack_size__()
            )
        );

        //6 初始化回收栈与回收位图
        init_recycle_stack();

        //7 初始化内存
        if (init_type_ == init_type::init_all_size) {
            init_block_ram_space(mes_out);
            if (mes_out.code != 0) {
                return;
            }
        }
        
        //8 设置初始化成功
        is_init = true;
    };

    // 析构函数
    free_volume::~free_volume() {

    };

    //======================================== 内存分配 ========================================

    void free_volume::new_block(
        bmms_f::agent_ptr& agent_ptr_out,
        size_t& index_out,
        mes::a_mes& mes_out
    ) {
        pop_from_recycle_stack(agent_ptr_out, index_out, mes_out);
    }

    void free_volume::delete_block(
        bmms_f::agent_ptr& agent_ptr_in,
        size_t&            index_in
    ) {
        push_to_recycle_stack(agent_ptr_in, index_in);
    }

    void free_volume::free_all_unused_blocks() {
        // 1. 校验状态
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_not_inited);
        }

        // 2. 遍历所有块
        for (size_t i = 0; i < block_count_max; i = nf_add(i, 1)) {
            // 2.1 检查是否为空闲状态
            if (!is_block_free(i)) {
                continue;  // 已分配，跳过
            }

            // 2.2 检查是否已有物理内存
            if (block_ptr_ram_space[i] == nullptr) {
                continue;  // 已释放，跳过
            }

            // 2.3 释放物理内存
            free_a_block(i);
        }

        // 等效于：
        // free_range(0, block_count_max);
    }

    //======================================== 物理内存释放 ========================================

    void free_volume::free_unused_block(size_t block_index_in) {
        // 1. 校验状态
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_not_inited);
        }

        // 2. 校验索引
        if (block_index_in >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_index_invalid);
        }

        // 3. 检查是否为空闲状态
        if (!is_block_free(block_index_in)) {
            // 块正在使用中，不能释放物理内存
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_in_use);
        }

        // 4. 检查是否已有物理内存
        if (block_ptr_ram_space[block_index_in] == nullptr) {
            // 已经释放过了，静默成功
            return;
        }

        // 5. 释放物理内存
        free_a_block(block_index_in);

        // ！注意：不修改栈状态！
        // 该块仍然在栈中，标记位仍然是0x01，栈条目仍然有效
        // 下次分配时会自动重新分配物理内存
    }

    void free_volume::free_range(
        size_t begin_index_in,
        size_t size_in
    ) {
        // 1. 校验状态
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_not_inited);
        }

        // 2. 校验参数
        if (size_in == 0) {
            return;  // 空范围，直接返回
        }

        // 3. 计算范围，防止溢出
        size_t end_index = nf_add(begin_index_in, size_in);
        if (end_index > block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_range_invalid);
        }

        // 4. 遍历范围内的所有块
        for (size_t i = begin_index_in; i < end_index; i = nf_add(i, 1)) {
            // 4.1 检查是否为空闲状态
            if (!is_block_free(i)) {
                continue;  // 已分配，跳过
            }

            // 4.2 检查是否已有物理内存
            if (block_ptr_ram_space[i] == nullptr) {
                continue;  // 已释放，跳过
            }

            // 4.3 释放物理内存
            free_a_block(i);
        }

        // ！ 注意：整个过程不修改任何栈状态
        // 栈指针不变，标记位不变，栈条目不变
    }

    //============================ 公共数据获取 ============================

    bool free_volume::get_is_init() {
        return is_init;
    };

    size_t free_volume::get_block_count_max() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_not_inited)
        }
        return block_count_max;
    };

    size_t free_volume::get_block_count_used() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_not_inited)
        }
        return using_block_count;
    };

    size_t free_volume::get_free_block_count() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_not_inited)
        }
        return nf_sub(block_count_max, using_block_count);
    };

    size_t free_volume::get_block_size() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_not_inited)
        }
        return block_size;
    };

    size_t free_volume::__recycle_stack_size__() {
        // 回收栈存储块索引，每个块对应8字节
        return nf_mul(block_count_max, 8);
    };

    size_t free_volume::__actually_block_size__() {
        size_t alignment = 0;
        switch (alignment_type_) {
        case alignment_type::without_alignment:
            return block_size;
        case alignment_type::byte_4_each_block:   alignment = 4;  break;
        case alignment_type::byte_8_each_block:   alignment = 8;  break;
        case alignment_type::byte_16_each_block:  alignment = 16; break;
        case alignment_type::byte_32_each_block:  alignment = 32; break;
        case alignment_type::byte_64_each_block:  alignment = 64; break;
        default:
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_alignment_type_invalid);
            return block_size;
        };

        // 对齐算法：((size + alignment - 1) & ~(alignment - 1))
        size_t alignment_minus_one = nf_sub(alignment, 1);
        size_t temp = nf_add(block_size, alignment_minus_one);
        return temp & ~alignment_minus_one;
    };

    uintptr_t free_volume::__block_ptr_space_begin__() {
        return reinterpret_cast<uintptr_t>(block_ptr_ram_space.get());
    };

    size_t free_volume::__block_ptr_space_size__() {
        return nf_mul(block_count_max, 8);
    };

    uintptr_t free_volume::__block_ptr_space_end__() {
        return nf_add(__block_ptr_space_begin__(), __block_ptr_space_size__());
    };

    uintptr_t free_volume::__stack_space_begin__() {
        return reinterpret_cast<uintptr_t>(stack_ram_space.get());
    };

    size_t free_volume::__stack_space_size__() {
        return nf_mul(block_count_max, 8);
    };

    uintptr_t free_volume::__stack_space_end__() {
        return nf_add(
            __stack_space_begin__(),
            __stack_space_size__()
        );
    };

    size_t free_volume::__block_ptr_offset__(size_t block_index_in) {
        return nf_mul(block_index_in, 8);
    };

    //============================ 定理校验函数 ============================
    
    void free_volume::check_init_value_valid() {
        // 1. 校验枚举值是否有效
        // 1.1 校验 init_type 枚举
        if (!(init_type::no_init_all_size <= init_type_ &&
            init_type_ <= init_type::init_all_size)) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_init_type_invalid);
        }

        // 1.2 校验 alignment_type 枚举
        if (!(alignment_type::without_alignment <= alignment_type_ &&
            alignment_type_ <= alignment_type::byte_64_each_block)) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_alignment_type_invalid);
        }

        // 2. 校验块大小参数
        if (block_size == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_size_zero);
        }

        if (block_size > FREE_VOLUME_MAX_BLOCK_SIZE) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_size_invalid);
        }

        // 3. 校验最大块数参数
        if (block_count_max == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_count_zero);
        }

        if (block_count_max > FREE_VOLUME_MAX_BLOCK_COUNT) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_count_invalid);
        }

        // 4. 校验块数是否超过索引最大值（0x00FFFFFFFFFFFFFF）
        if (block_count_max > bmms_f::index_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_count_exceed_index_max);
        }

        // 5. 校验实际块大小计算是否会溢出
        // 触发实际块大小计算，如果计算过程中溢出，nf_add/nf_sub会抛出错误
        size_t actually_block_size = __actually_block_size__();

        // 6. 校验回收栈大小计算是否会溢出
        size_t recycle_stack_size = __recycle_stack_size__();

        // 7. 校验总内存使用是否合理（虽然没有连续内存池，但单个块的内存分配也要考虑）
        // 检查单个块内存分配是否会溢出
        nf_mul(actually_block_size, block_count_max);  // 只做溢出检查，不存储结果
    }

    void free_volume::check_block_belong_to_self(
        bmms_f::agent_ptr& block_agent_ptr_in,
        size_t block_index_in
    ) {
        //1 校验index
        if (block_index_in >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_index_invalid)
        };
        //2 校验index对应指针非空
        if (block_ptr_ram_space[block_index_in] == nullptr) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_pass_in_a_invalid_block_that_not_belong_to_self)
        }
        //3 校验代理指针非空
        if (block_agent_ptr_in.raw_ptr == nullptr) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_pass_in_a_invalid_block_that_not_belong_to_self)
        }
        //4 校验块释放属于自己
        uintptr_t true_block_ptr = reinterpret_cast<uintptr_t>(
            block_ptr_ram_space[block_index_in].get()
        );
        //4.1 校验块指针是否对齐
        if (true_block_ptr != reinterpret_cast<uintptr_t>(block_agent_ptr_in.raw_ptr)) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_pass_in_a_invalid_block_that_not_belong_to_self)
        }
        //4.2 校验块大小是否一致
        if (block_size != block_agent_ptr_in.size) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_pass_in_a_invalid_block_that_not_belong_to_self)
        }
    };

    void free_volume::check_can_push(
        bmms_f::agent_ptr& block_agent_ptr_in,
        size_t block_index_in
    ) {
        //1 校验块属于自己
        check_block_belong_to_self(block_agent_ptr_in, block_index_in);

        //2 校验双重释放
        size_t bit_data = 0;
        size_t bit_data_mask = 0xFF00000000000000;
        size_t bit_offset = nf_mul(block_index_in, 8);
        //2.1 读取位图原始数据
        stack_index_ram_space.load_to_void_ptr(
            bit_offset,
            8,
            &bit_data,
            true
        );
        //2.2 忽略低7字节
        bit_data = bit_data & bit_data_mask;
        //2.3 校验是否已经释放
        if (bit_data == 0x0100000000000000) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_already_deleted)
        }

        //3 校验栈满压栈
        if (reinterpret_cast<uintptr_t>(recycle_stack_top_ptr) == __stack_space_begin__()) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_recycle_stack_full)
        }
    };

    bool free_volume::check_can_pop() {
        if (reinterpret_cast<uintptr_t>(recycle_stack_top_ptr) < __stack_space_end__()) {
            return true;
        }
        else {
            return false;
        }
    };

    //============================ 私有数据获取 ============================

    size_t free_volume::get_block_ptr_ram_space_offset(
        bmms_f::agent_ptr& block_agent_ptr_in,
        size_t block_index_in
    ){
        check_block_belong_to_self(
            block_agent_ptr_in,
            block_index_in
        );
        return __block_ptr_offset__(block_index_in);
    };

    //============================ 运算 ============================


    size_t free_volume::nf_add(size_t a, size_t b) {
        if (a > SIZE_MAX - b) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_add_overflow);
        }
        return a + b;
    }

    size_t free_volume::nf_sub(size_t a, size_t b) {
        if (a < b) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_sub_overflow);
        }
        return a - b;
    }

    size_t free_volume::nf_mul(size_t a, size_t b) {
        if (b != 0 && a > SIZE_MAX / b) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_mul_overflow);
        }
        return a * b;
    }

    size_t free_volume::nf_div(size_t a, size_t b) {
        if (b == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_div_by_zero);
        }
        return a / b;
    }

    size_t free_volume::nf_mod(size_t a, size_t b) {
        // 定理：对于无符号整数，a % b 总是定义良好的，只要 b ≠ 0
        if (b == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_mod_by_zero);
        }

        // 特殊情况优化
        if (b == 1) {
            return 0;  // a % 1 = 0
        }

        // 检查是否是2的幂（可以用位运算优化）
        if ((b & (b - 1)) == 0) {
            // b是2的幂，a % b = a & (b - 1)
            return a & (b - 1);
        }

        return a % b;
    }

    //============================ 初始化 ============================

    void free_volume::init_recycle_stack() {
        // 1. 准备变量 最高1字节是回收标记
        size_t index = 0x0100000000000000;

        // 1. 清空栈区域
        stack_index_ram_space.clear();

        // 5. 填充栈（按块地址顺序）
        for (size_t i = 0; i < block_count_max; i++) {
            stack_index_ram_space.store_from_void_ptr(
                &index,                     // 块位图与index
                8,                          // 指针大小（8字节）
                nf_mul(i, 8),               // 栈位置
                true                        // 安全检查
            );

            // 计算下一个块 位图+index
            index++;
        }

        //6. 设置栈顶指针
        recycle_stack_top_ptr = reinterpret_cast<size_t*>(__stack_space_begin__());
    };

    void free_volume::init_a_block(
        size_t block_index_in,
        mes::a_mes& mes_out
    ) {
        //1 重置消息
        mes_out = mes::a_mes();

        //2 校验块索引是否有效
        if (block_index_in >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_index_invalid_when_init_a_block);
        }
        //3 校验指针空
        if (block_ptr_ram_space[block_index_in] != nullptr) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_init_block_again)
        }

        //4 申请块内存
        unique_ptr<byte_1[]> tmp_block_ptr = nullptr;
        try {
            tmp_block_ptr = move(make_unique<byte_1[]>(actually_block_size));
        }
        catch(...){
            mes_out = bmms_m::err::service_err_free_volume_init_block_failed();
            return;
        }

        //5 写入块指针
        block_ptr_ram_space[block_index_in] = move(tmp_block_ptr);
    };

    void free_volume::init_block_ram_space(
        mes::a_mes& mes_out
    ) {
        //1 重置消息
        mes_out = mes::a_mes();

        //2 初始化所有块
        for (size_t i = 0; i < block_count_max; i++) {
            // 初始化块
            init_a_block(i, mes_out);

            // 如果初始化失败，释放之前的所有块，从高往低处释放，避免碎片
            if (mes_out.code != 0) {
                size_t j = nf_sub(i, 1);
                while (true) {
                    free_a_block(j);

                    if (j == 0) { 
                        break;
                    }
                    else {
                        j = nf_sub(j, 1);
                    }
                }
                return;
            }
        }
    };

    //============================ 私有辅助 ============================

    void free_volume::free_a_block(size_t block_index_in) {
        //1 校验块索引是否有效
        if (block_index_in >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_index_invalid);
        }
        //2 校验指针非空
        if (block_ptr_ram_space[block_index_in] == nullptr) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_free_again)
        }
        //3 释放内存
        block_ptr_ram_space[block_index_in].reset();
        //4 置空指针
        block_ram_space.clear_range(__block_ptr_offset__(block_index_in), 8);
    };

    // 检查块是否为空闲状态（在栈中）
    bool free_volume::is_block_free(size_t block_index_in) {
        // 1. 校验索引
        if (block_index_in >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_index_invalid);
        }

        // 2. 读取标记位
        size_t bit_data = 0;
        size_t bit_offset = nf_mul(block_index_in, 8);

        stack_index_ram_space.load_to_void_ptr(
            bit_offset,
            8,
            &bit_data,
            true
        );

        // 3. 检查高字节是否为0x01
        return (bit_data & 0xFF00000000000000) == 0x0100000000000000;
    }

    // 检查块是否已分配物理内存
    bool free_volume::is_block_allocated(size_t block_index_in) {
        if (block_index_in >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_block_index_invalid);
        }
        return block_ptr_ram_space[block_index_in] != nullptr;
    }

    //============================ 栈操作 ============================

    void free_volume::push_to_recycle_stack(
        bmms_f::agent_ptr& block_agent_ptr_in,
        size_t             index_in
    ) {
        // 1. 校验
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_not_inited);
        }
        check_block_belong_to_self(block_agent_ptr_in, index_in);
        check_can_push(block_agent_ptr_in, index_in);

        // 2. 清空数据
        block_agent_ptr_in.clear();

        // ========== 3. 更新块 i 的标记位 ==========
        size_t bit_data = 0;
        size_t bit_offset = nf_mul(index_in, 8);

        // 3.1 读取原始位图数据
        stack_index_ram_space.load_to_void_ptr(
            bit_offset,
            8,
            &bit_data,
            true
        );

        // 3.2 最高字节打标记1（空闲），保留低7字节不变
        bit_data = bit_data | 0x0100000000000000;

        // 3.3 写回
        stack_index_ram_space.store_from_void_ptr(
            &bit_data,
            8,
            bit_offset,
            true
        );

        // ========== 4. 移动栈指针 ==========
        recycle_stack_top_ptr = reinterpret_cast<size_t*>(
            nf_sub(
                reinterpret_cast<uintptr_t>(recycle_stack_top_ptr),
                8
            )
            );

        // ========== 5. 写入栈条目 ==========
        // 栈条目位置 = 当前栈顶指针指向的位置
        size_t stack_data = 0;
        size_t stack_offset = nf_sub(
            reinterpret_cast<uintptr_t>(recycle_stack_top_ptr),
            __stack_space_begin__()
        );

        // 5.1 读取该位置的原始数据
        stack_index_ram_space.load_to_void_ptr(
            stack_offset,
            8,
            &stack_data,
            true
        );

        // 5.2 清除低7字节（保留最高字节，那是该位置对应块的固定标记位）
        stack_data = stack_data & 0xFF00000000000000;

        // 5.3 在低7字节写入块索引
        stack_data = stack_data | index_in;

        // 5.4 写回
        stack_index_ram_space.store_from_void_ptr(
            &stack_data,
            8,
            stack_offset,
            true
        );

        // 6. 销毁代理指针
        block_agent_ptr_in = bmms_f::agent_ptr();

        // 7. 更新使用计数
        using_block_count = nf_sub(using_block_count, 1);
    }

    void free_volume::pop_from_recycle_stack(
        bmms_f::agent_ptr& block_agent_ptr_out,
        size_t& index_out,
        mes::a_mes& mes_out
    ) {
        // 1. 初始化消息
        mes_out = mes::a_mes();

        // 2. 校验
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_not_inited);
        }

        if (!check_can_pop()) {
            mes_out = bmms_m::err::service_err_free_volume_recycle_stack_empty();
            return;
        }

        // ========== 3. 读取栈顶条目 ==========
        size_t stack_data = 0;
        size_t stack_offset = nf_sub(
            reinterpret_cast<uintptr_t>(recycle_stack_top_ptr),
            __stack_space_begin__()
        );

        stack_index_ram_space.load_to_void_ptr(
            stack_offset,
            8,
            &stack_data,
            true
        );

        // 3.1 取低7字节作为块索引
        size_t block_index = stack_data & 0x00FFFFFFFFFFFFFF;
        index_out = block_index;

        // ========== 4. 移动栈指针 ==========
        recycle_stack_top_ptr = reinterpret_cast<size_t*>(
            nf_add(
                reinterpret_cast<uintptr_t>(recycle_stack_top_ptr),
                8
            )
            );

        // 注意：此时栈条目还在那里，只是栈指针移动了
        // 栈条目中的低7字节（索引）仍然存在，但已无效
        // 高字节（该位置的固定标记位）保持不变

        // ========== 5. 获取物理内存 ==========
        // 5.1 检查是否需要分配物理内存
        if (block_ptr_ram_space[block_index] == nullptr) {
            init_a_block(block_index, mes_out);
            if (mes_out.code != 0) {
                // 分配失败，需要回滚栈指针
                recycle_stack_top_ptr = reinterpret_cast<size_t*>(
                    nf_sub(
                        reinterpret_cast<uintptr_t>(recycle_stack_top_ptr),
                        8
                    )
                    );
                return;
            }
        }

        // 5.2 获取块指针
        void* block_ptr = block_ptr_ram_space[block_index].get();

        // 5.3 创建代理指针
        block_agent_ptr_out = bmms_f::agent_ptr(block_ptr, block_size);

        // 6. 清空内存（可选，取决于init_type）
        if (init_type_ == init_type::init_all_size) {
            block_agent_ptr_out.clear();
        }

        // ========== 7. 更新标记位为已分配 ==========
        size_t bit_data = 0;
        size_t bit_offset = nf_mul(block_index, 8);

        // 7.1 读取原始位图数据
        stack_index_ram_space.load_to_void_ptr(
            bit_offset,
            8,
            &bit_data,
            true
        );

        // 7.2 清除最高字节（标记为已分配），保留低7字节
        bit_data = bit_data & 0x00FFFFFFFFFFFFFF;

        // 7.3 写回
        stack_index_ram_space.store_from_void_ptr(
            &bit_data,
            8,
            bit_offset,
            true
        );

        // 8. 更新使用计数
        using_block_count = nf_add(using_block_count, 1);
    }
};