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
    
    // 移动构造函数
    static_volume::static_volume(static_volume&& other) noexcept
        : is_init(other.is_init)
        , init_type_(other.init_type_)
        , alignment_type_(other.alignment_type_)
        , block_size(other.block_size)
        , block_count_max(other.block_count_max)
        , recycle_stack_top_ptr(other.recycle_stack_top_ptr)
        , ram_space(move(other.ram_space))
        , block_ram_space(move(other.block_ram_space))
        , stack_index_ram_space(move(other.stack_index_ram_space))
    {
        // 重置原对象状态
        other.is_init = false;
        other.block_size = 0;
        other.block_count_max = 0;
        other.recycle_stack_top_ptr = nullptr;

        // 注意：unique_ptr 和 agent_ptr 已经被 move，自动置空
    }

    // 移动赋值运算符
    static_volume& static_volume::operator=(static_volume&& other) noexcept
    {
        if (this != &other) {
            // 1. 清理当前对象资源
            // unique_ptr 会自动释放内存
            // agent_ptr 会自动清理

            // 2. 移动所有成员
            is_init = other.is_init;
            init_type_ = other.init_type_;
            alignment_type_ = other.alignment_type_;
            block_size = other.block_size;
            block_count_max = other.block_count_max;
            recycle_stack_top_ptr = other.recycle_stack_top_ptr;
            ram_space = move(other.ram_space);
            block_ram_space = move(other.block_ram_space);
            stack_index_ram_space = move(other.stack_index_ram_space);

            // 3. 重置原对象
            other.is_init = false;
            other.block_size = 0;
            other.block_count_max = 0;
            other.recycle_stack_top_ptr = nullptr;

            // unique_ptr 和 agent_ptr 已经在 move 中置空
        }
        return *this;
    }

    //======================================== 构造与析构 ========================================

    // 参数构造函数
    static_volume::static_volume(
        size_t         block_size_in,
        size_t         block_count_max_in,
        init_type      init_type_in,
        alignment_type alignment_type_in,
        mes::a_mes&    mes_out
    ) {
        //1.重置消息
        mes_out = mes::a_mes();

        //2.检测2个枚举是否值正常
        check_enum_init_type_valid(
            init_type_in
        );
        check_enum_alignment_type_valid(
            alignment_type_in
        );

        //3.校验参数是否合理
        if (block_size_in == 0 || block_size_in > STATIC_VOLUME_MAX_BLOCK_SIZE) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_block_size_invalid)
        }
        if (block_count_max_in == 0 || block_count_max_in > STATIC_VOLUME_MAX_BLOCK_COUNT
            || block_count_max_in > bmms_f::index_max
            ) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_block_count_invalid)
        }

        //4.初始化参数
        block_size      = block_size_in;
        block_count_max = block_count_max_in;
        init_type_      = init_type_in;
        alignment_type_ = alignment_type_in;

        //5.校验计算出所需的内存大小是否溢出
        check_total_size_no_overflow();

        //6.分配内存
        try {
            ram_space = move(make_unique<byte_1[]>(__total_ram_size__()));
        }
        catch (...) {
            mes_out = bmms_m::err::service_err_static_volume_memory_alloc_failed();
            return;
        }

        //7. 移交内存管理权
        block_ram_space = move(
            bmms_f::agent_ptr(
                static_cast<void*>(ram_space.get()),
                __block_space_size__()
            )
        );
        stack_index_ram_space = move(
            bmms_f::agent_ptr(
                reinterpret_cast<void*>(
                    nf_add(
                        reinterpret_cast<uintptr_t>(ram_space.get()),
                        __block_space_size__()
                    )
                ),
                __recycle_stack_size__()
            )
        );

        //8.初始化回收栈 与 回收位图
        init_recycle_stack();

        //9.初始化内存
        if (init_type_in == init_type::init_all_size) {
            init_block_ram_space();
        }

        //10.设置初始化成功
        is_init = true;
    };

    // 析构函数
    static_volume::~static_volume() {
        //uniqut_ptr自动回收内存
    };

    //======================================== 内存分配 ========================================

    void static_volume::new_block(
        bmms_f::agent_ptr& agent_ptr_out,
        mes::a_mes& mes_out
    ) {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_not_inited)
        }
        pop_from_recycle_stack(
            agent_ptr_out,
            mes_out
        );
    };

    void static_volume::delete_block(
        bmms_f::agent_ptr& agent_ptr_in
    ) {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_not_inited)
        }
        push_to_recycle_stack(agent_ptr_in);
    };

    //============================ 数据获取 ============================

    bool   static_volume::get_is_init() {
        return is_init;
    }

    size_t static_volume::get_block_count_max() {
        return block_count_max;
    };

    size_t static_volume::get_free_block_count() {
        return (
            nf_div(
                nf_sub(
                    __stack_end__(),
                    reinterpret_cast<uintptr_t>(recycle_stack_top_ptr)
                ),
                8
            )
        );
    };

    size_t static_volume::get_block_size() {
        return block_size;
    };

    size_t static_volume::__total_ram_size__() {
        return nf_mul(nf_add(__actually_block_size__(),8), block_count_max);
    };

    size_t static_volume::__block_space_size__() {
        return nf_mul(__actually_block_size__(), block_count_max);
    }

    size_t static_volume::__recycle_stack_size__() {
        return nf_mul(block_count_max,8);
    };

    size_t static_volume::__actually_block_size__() {
        switch (alignment_type_) {
        case alignment_type::without_alignment:
            return block_size;
        case alignment_type::byte_4_each_block:
            return nf_add(block_size, 3) & ~3ULL;
        case alignment_type::byte_8_each_block:
            return nf_add(block_size, 7) & ~7ULL;
        case alignment_type::byte_16_each_block:
            return nf_add(block_size, 15) & ~15ULL;
        case alignment_type::byte_32_each_block:
            return nf_add(block_size, 31) & ~31ULL;
        case alignment_type::byte_64_each_block:
            return nf_add(block_size, 63) & ~63ULL;
        default:
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_alignment_type_invalid);
            return block_size;
        }
    }

    uintptr_t static_volume::__block_space_begin__() {
        return reinterpret_cast<uintptr_t>(ram_space.get());
    };

    uintptr_t static_volume::__block_space_end__() {
        return nf_add(__block_space_begin__(), __block_space_size__());
    };
    
    uintptr_t static_volume::__stack_begin__() {
        return __block_space_end__();
    }

    uintptr_t static_volume::__stack_end__() {
        return nf_add(__block_space_begin__(), __total_ram_size__());
    };

    void* static_volume::get_block_ptr(size_t block_index) {
        return reinterpret_cast<void*>(
            nf_add(
                __block_space_begin__(),
                nf_mul(
                    block_index,
                    __actually_block_size__()
                )
            )
            );
    };

    size_t static_volume::get_block_index(bmms_f::agent_ptr& block_agent_ptr_in) {
        return nf_div(
            nf_sub(
                block_agent_ptr_in.__self_begin__(),
                __block_space_begin__()
            ),
            __actually_block_size__()
        );
    };

    //============================ 定理校验函数 ============================
    void static_volume::check_total_size_no_overflow() {
        nf_mul(nf_add(__actually_block_size__(),8), block_count_max);
    };

    void static_volume::check_enum_init_type_valid(
        init_type init_type_in
    ) {
        if (!(0 <= init_type_in && init_type_in <= 1)) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_init_type_invalid);
        }
    };

    void static_volume::check_enum_alignment_type_valid(
        alignment_type alignment_type_in
    ) {
        if (!(0 <= alignment_type_in && alignment_type_in <= 5)) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_alignment_type_invalid);
        }
    };

    void static_volume::check_block_belong_to_self(
        bmms_f::agent_ptr& agent_ptr_in
    ) {
        //1. 大小必须匹配，避免手搓的指针
        size_t agent_ptr_agent_size = 0; agent_ptr_in.get_size(agent_ptr_agent_size);
        if (agent_ptr_agent_size != block_size) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_invalid_delete)
        }

        //2. 指针起始必须在本卷的块空间内存范围内
        if (!
            (
                __block_space_begin__() <= agent_ptr_in.__self_begin__()
                &&
                agent_ptr_in.__self_begin__() <= nf_sub(__block_space_end__(), __actually_block_size__())
            )
            ) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_invalid_delete)
        }
    };

    void static_volume::check_can_push(
        bmms_f::agent_ptr& agent_ptr_in
    ) {
        // 1. 获取块索引
        size_t block_index = get_block_index(agent_ptr_in);

        // 2. 校验块索引有效
        if (block_index >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_block_index_invalid)
        }

        // 3. 通过 agent_ptr 安全读取位图状态
        size_t bit_data = 0;
        size_t bit_offset = nf_mul(block_index, 8);
        size_t bit_data_mask = 0xFF00000000000000;
        stack_index_ram_space.load_to_void_ptr(
            bit_offset,
            8,
            &bit_data,
            true  // 启用安全检查
        );

        // 4. 校验是否已经释放
        if ((bit_data & bit_data_mask) == 0x0100000000000000) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_block_already_deleted)
        }

        // 5. 校验栈满
        if (reinterpret_cast<uintptr_t>(recycle_stack_top_ptr) == __stack_begin__()) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_recycle_stack_full)
        }
    }

    bool static_volume::check_can_pop() {
        if (reinterpret_cast<uintptr_t>(recycle_stack_top_ptr) < __stack_end__()) {
            return true;
        }
        else {
            return false;
        }
    };

    //============================ 运算 ============================
    
    size_t static_volume::nf_add(size_t a, size_t b) {
        if (a > SIZE_MAX - b) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_add_overflow);
        }
        return a + b;
    }

    size_t static_volume::nf_sub(size_t a, size_t b) {
        if (a < b) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_sub_overflow);
        }
        return a - b;
    }

    size_t static_volume::nf_mul(size_t a, size_t b) {
        if (b != 0 && a > SIZE_MAX / b) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_mul_overflow);
        }
        return a * b;
    }

    size_t static_volume::nf_div(size_t a, size_t b) {
        if (b == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_div_by_zero);
        }
        return a / b;
    }

    size_t static_volume::nf_mod(size_t a, size_t b) {
        // 定理：对于无符号整数，a % b 总是定义良好的，只要 b ≠ 0
        if (b == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_mod_by_zero);
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

    void static_volume::init_recycle_stack() {
        // 1. 准备变量 最高1字节是回收标记
        size_t index = 0x0100000000000000;
        void* index_ptr =& index;

        // 1. 清空栈区域
        stack_index_ram_space.clear();

        // 5. 填充栈（按块地址顺序）
        for (size_t i = 0; i < block_count_max; i++) {
            stack_index_ram_space.store_from_void_ptr(
                index_ptr,                 // 块位图与index
                8,                         // 指针大小（8字节）
                nf_mul(i,8),               // 栈位置
                true                       // 安全检查
            );

            // 计算下一个块 位图+index
            index++;
        }

        //6. 设置栈顶指针
        recycle_stack_top_ptr = reinterpret_cast<size_t*>(__stack_begin__());
    };

    void static_volume::init_a_block(
        void* block_begin_void_ptr_in
    ) {
        uintptr_t self_begin  = block_ram_space.__self_begin__();
        size_t    clear_begin = nf_sub(
            reinterpret_cast<size_t>(block_begin_void_ptr_in),
            self_begin
        );
        block_ram_space.clear_range(
            clear_begin,
            __actually_block_size__()
        );
    };

    void static_volume::init_block_ram_space() {
        uintptr_t self_begin = block_ram_space.__self_begin__();
        void*     block_begin_ptr = reinterpret_cast<void*>(self_begin);
        for (size_t i = 0 ; i < block_count_max ; i++) {
            init_a_block(block_begin_ptr);
            block_begin_ptr = reinterpret_cast<void*>(
                nf_add(
                    reinterpret_cast<uintptr_t>(block_begin_ptr),
                    __actually_block_size__()
                )
            );
        }
    };

    //============================ 栈操作 ============================
    
    // 压栈到回收栈
    void static_volume::push_to_recycle_stack(
        bmms_f::agent_ptr& block_agent_ptr_in
    ) {
        // 1.校验是否是本卷的块
        check_block_belong_to_self(block_agent_ptr_in);

        // 2.校验是否可压栈
        check_can_push(block_agent_ptr_in);

        // 3.清空数据
        block_agent_ptr_in.clear();

        size_t block_index = get_block_index(block_agent_ptr_in);
        // 4.写入块位图标记
        size_t bit_data        = 0;
        size_t bit_data_mask   = 0x0100000000000000;
        size_t bit_offset = nf_mul(block_index, 8);
        // 4.1 读取位图原始数据
        stack_index_ram_space.load_to_void_ptr(
            bit_offset,
            8,
            &bit_data,
            true
        );
        // 4.2 位图最高字节打标记1，表示已经压栈，保留低7字节可能的其他块的栈信息
        bit_data = bit_data | bit_data_mask;
        // 4.3 写回数据
        stack_index_ram_space.store_from_void_ptr(
            &bit_data,
            8,
            bit_offset,
            true
        );

        // 5 移动栈指针
        recycle_stack_top_ptr = reinterpret_cast<size_t*>(
            nf_sub(
                reinterpret_cast<uintptr_t>(recycle_stack_top_ptr),
                8
            )
        );

        // 6 写入数据
        // 6.1 读取待写入栈位置原始数据
        size_t stack_data = 0;
        size_t stack_data_mask = 0x0100000000000000;
        size_t stack_data_offset =nf_sub(
            reinterpret_cast<uintptr_t>(recycle_stack_top_ptr),
            __stack_begin__()
        );
        stack_index_ram_space.load_to_void_ptr(
            stack_data_offset,
            8,
            &stack_data,
            true
        );
        // 6.2 将低7字节清零
        stack_data = stack_data & stack_data_mask;
        // 6.3 在低7字节写入块index，保留该位置上最高字节可能的其他的块的位图标记
        stack_data = stack_data | block_index;
        // 6.4 写回数据
        stack_index_ram_space.store_from_void_ptr(
            &stack_data,
            8,
            stack_data_offset,
            true
        );

        // 5 销毁代理指针
        block_agent_ptr_in = move(bmms_f::agent_ptr());
    };

    // 从回收栈弹栈
    void static_volume::pop_from_recycle_stack(
        bmms_f::agent_ptr& block_agent_ptr_out,
        mes::a_mes&      mes_out
    ) {
        // 1.校验是否可弹栈
        if (!check_can_pop()) {
            mes_out = move(bmms_m::err::service_err_static_volume_recycle_stack_empty());
            return;
        }

        // 2.取数据
        // 2.1 取原始数据
        size_t stack_data = 0;
        size_t stack_data_offset = nf_sub(
            reinterpret_cast<uintptr_t>(recycle_stack_top_ptr),
            __stack_begin__()
        );
        stack_index_ram_space.load_to_void_ptr(
            stack_data_offset,
            8,
            &stack_data,
            true
        );
        // 2.2 取块index，忽略最高字节
        size_t block_index = stack_data & 0x00FFFFFFFFFFFFFF;
        // 2.3 获取块指针
        void* block_ptr    = get_block_ptr(block_index);

        // 3.移动栈指针
        recycle_stack_top_ptr = reinterpret_cast<size_t*>(
            nf_add(
                reinterpret_cast<uintptr_t>(recycle_stack_top_ptr),
                8
            )
        );

        // 4.设置代理指针
        block_agent_ptr_out = move(bmms_f::agent_ptr(block_ptr, block_size));

        // 5.初始化内存
        block_agent_ptr_out.clear();

        // 6.更改标记为已分配
        size_t bit_data = 0;
        size_t bit_data_mask = 0x00FFFFFFFFFFFFFF;
        size_t bit_offset = nf_mul(block_index, 8);
        // 6.1 读取位图原始数据
        stack_index_ram_space.load_to_void_ptr(
            bit_offset,
            8,
            &bit_data,
            true
        );
        // 6.2 位图最高字节打标记0，表示已经弹栈，保留低7字节可能的其他块的栈信息
        bit_data = bit_data & bit_data_mask;
        // 6.3 写回数据
        stack_index_ram_space.store_from_void_ptr(
            &bit_data,
            8,
            bit_offset,
            true
        );
    };
};