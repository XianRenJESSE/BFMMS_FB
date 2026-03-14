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

    static_volume_part::static_volume_part(
        shared_ptr<static_volume>& static_volume_shared_ptr_in,
        size_t                     block_count_max_in,
        init_type                  init_type_in,
        control_type               control_type_in,
        mes::a_mes&                mes_out
    ):  init_type_(init_type_in),
        control_type_(control_type_in),
        block_count_max(block_count_max_in),
        static_volume_shared_ptr(static_volume_shared_ptr_in)
    {
        // 1 重置消息
        mes_out = mes::a_mes();

        // 2 参数校验
        check_build_value_valid(mes_out);
        if (mes_out.code != 0) return;

        // 3 控制策略校验
        check_contro_valid(mes_out);
        if (mes_out.code != 0) return;

        // 4 开辟block_ptr_ram_space内存
        try {
            block_ptr_ram_space = move(make_unique<void*[]>(block_count_max));
        }
        catch (...) {
            mes_out = bmms_m::err::service_err_static_volume_part_memory_alloc_failed();
            return;
        }

        // 5 移交管理权
        block_ptr_ram_space_agent_ptr = move(
            bmms_f::agent_ptr(block_ptr_ram_space.get(), nf_mul(block_count_max,8))
        );

        // 6 初始化block_ptr_ram_space内存
        block_ptr_ram_space_agent_ptr.clear();

        // 7 根据策略初始化块 - 传入 mes_out
        init_blocks(mes_out);  // 现在 init_blocks 会设置 mes_out
        if (mes_out.code != 0) {
            // 初始化失败，清理资源
            block_ptr_ram_space.reset();
            block_ptr_ram_space_agent_ptr = bmms_f::agent_ptr();
            block_count_max = 0;
            using_block_count = 0;
            is_init = false;
            return;  // 直接返回，mes_out 已经包含错误信息
        }

        // 8 成功
        is_init = true;
    };

    static_volume_part::~static_volume_part() {
        if (!is_init) return;
        void* block_ptr = nullptr;
        // 释放所有已分配的块回静态卷
        for (size_t i = 0; i < block_count_max; i++) {
            // 从指针数组中读取指针
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                i * 8,  // 每个指针8字节
                8,
                &block_ptr,
                true
            );

            // 如果指针不为nullptr，说明这个块是已分配的
            if (block_ptr != nullptr) {
                // 创建代理指针来释放块
                bmms_f::agent_ptr tmp_ap(block_ptr, get_block_size());
                static_volume_shared_ptr->delete_block(tmp_ap);
            }
        }

        // unique_ptr会自动释放block_ptr_ram_space内存
        // shared_ptr引用计数会自动减少
    }

    //======================================== 访问控制 ========================================

    static_volume_part::static_volume_part(static_volume_part&& other) noexcept
        : is_init(other.is_init)
        , init_type_(other.init_type_)
        , control_type_(other.control_type_)
        , block_count_max(other.block_count_max)
        , using_block_count(other.using_block_count)
        , static_volume_shared_ptr(move(other.static_volume_shared_ptr))
        , block_ptr_ram_space(move(other.block_ptr_ram_space))
        , block_ptr_ram_space_agent_ptr(move(other.block_ptr_ram_space_agent_ptr))
    {
        // 转移后重置原对象状态
        other.is_init = false;
        other.init_type_ = init_type::no_init_all_size;
        other.control_type_ = control_type::not_allow_over_use;
        other.block_count_max = 0;
        other.using_block_count = 0;
        other.static_volume_shared_ptr = nullptr;
        // unique_ptr和agent_ptr在move后已自动置空
    }

    static_volume_part& static_volume_part::operator=(static_volume_part&& other) noexcept
    {
        if (this != &other) {
            // 转移所有资源
            is_init = other.is_init;
            init_type_ = other.init_type_;
            control_type_ = other.control_type_;
            block_count_max = other.block_count_max;
            using_block_count = other.using_block_count;
            static_volume_shared_ptr = move(other.static_volume_shared_ptr);
            block_ptr_ram_space = move(other.block_ptr_ram_space);
            block_ptr_ram_space_agent_ptr = move(other.block_ptr_ram_space_agent_ptr);

            // 重置原对象状态
            other.is_init = false;
            other.init_type_ = init_type::no_init_all_size;
            other.control_type_ = control_type::not_allow_over_use;
            other.block_count_max = 0;
            other.using_block_count = 0;
            other.static_volume_shared_ptr = nullptr;
            // unique_ptr和agent_ptr在move后已自动置空
        }
        return *this;
    }

    //======================================== 数据控制 ========================================

    void static_volume_part::clear() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }

        bmms_f::agent_ptr block_agent_ptr = bmms_f::agent_ptr();
        bool            is_nullptr = false;
        for (size_t i = 0; i < block_count_max; i++) {
            get_block_ptr(i, block_agent_ptr, is_nullptr);
            if (!is_nullptr) {
                block_agent_ptr.clear();
            }
        }
    };

    void static_volume_part::clear_range(
        size_t begin_in,
        size_t size_in
    ) {
        //1 初始化校验
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        
        //2 区间值校验
        check_range(begin_in, size_in);

        //3 变量准备
        size_t index_begin_out = 0;
        size_t index_count_out = 0;
        size_t index_end = 0;
        size_t op_begin_of_first_block = 0;
        size_t op_size_of_end_block = 0;
        bmms_f::agent_ptr block_agent_ptr = bmms_f::agent_ptr();
        bool   is_over_more_than_a_block = 0;
        bool   is_nullptr = false;
        
        //4 数据获取
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

        if (!is_over_more_than_a_block) {
            get_block_ptr(index_begin_out, block_agent_ptr, is_nullptr);

            if (is_nullptr) {
                return;
            }
            else {
                block_agent_ptr.clear_range(op_begin_of_first_block, size_in);
            }
        }
        else {
            // 处理第一个块
            get_block_ptr(index_begin_out, block_agent_ptr, is_nullptr);
            if (!is_nullptr) {
                block_agent_ptr.clear_range(
                    op_begin_of_first_block, 
                    nf_sub(block_agent_ptr.size, op_begin_of_first_block)
                );
            }

            // 处理中间块
            if (index_count_out > 2) {
                for (
                    size_t i = index_begin_out + 1; 
                    i < nf_sub(nf_add(index_begin_out,index_count_out),1); 
                    i++
                ){
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
    };

    void static_volume_part::resize(
        size_t block_count_limit_in,
        mes::a_mes& mes_out
    ) {
        // 1 初始化校验
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }

        // 2 参数校验
        if (block_count_limit_in == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_resize_block_count_zero)
        }

        if (block_count_limit_in > bmms_f::index_max) {
            mes_out = bmms_m::err::coding_err_static_volume_part_build_block_count_over_index_max();
            return;
        }

        // 3 控制策略匹配校验
        if (control_type_ == control_type::not_allow_over_use) {
            if (block_count_limit_in > static_volume_shared_ptr->get_block_count_max()) {
                mes_out = bmms_m::err::service_err_static_volume_part_resize_over_volume_limit();
                return;
            }
        }

        // ====================== ✅ 新增：空闲块预检查 ======================
        if (block_count_limit_in > block_count_max) {
            // 需要扩大
            size_t needed_blocks = nf_sub(block_count_limit_in, block_count_max);

            if (init_type_ == init_type::init_all_size) {
                // 需要分配新块
                if (needed_blocks > static_volume_shared_ptr->get_free_block_count()) {
                    mes_out = bmms_m::err::service_err_static_volume_part_resize_not_enough_blocks();
                    return;
                }
            }
            // 如果是 no_init_all_size，不需要分配新块，所以不需要检查
        }

        // 4 如果是缩小且初始化类型是init_all_size，需要释放超出部分的块
        if (block_count_limit_in < block_count_max && init_type_ == init_type::init_all_size) {
            // 释放超出新限制的块
            for (size_t i = block_count_limit_in; i < block_count_max; i = nf_add(i, 1)) {
                bmms_f::agent_ptr block_agent_ptr;
                bool is_nullptr = false;
                get_block_ptr(i, block_agent_ptr, is_nullptr);

                if (!is_nullptr) {
                    // 释放块回静态卷
                    static_volume_shared_ptr->delete_block(block_agent_ptr);
                    using_block_count = nf_sub(using_block_count, 1);
                }
            }
        }

        // 5 申请新的指针数组空间
        unique_ptr<void* []> new_block_ptr_ram_space = nullptr;
        bmms_f::agent_ptr new_block_ptr_ram_space_agent_ptr;

        try {
            new_block_ptr_ram_space = move(make_unique<void* []>(block_count_limit_in));
        }
        catch (...) {
            mes_out = bmms_m::err::service_err_static_volume_part_resize_memory_alloc_failed();
            return;
        }

        // 创建新指针数组的代理指针
        new_block_ptr_ram_space_agent_ptr = bmms_f::agent_ptr(
            new_block_ptr_ram_space.get(),
            nf_mul(block_count_limit_in, 8)
        );

        // 初始化新数组（全部置nullptr）
        new_block_ptr_ram_space_agent_ptr.clear();

        // 6 复制旧数据到新空间
        size_t copy_count = block_count_limit_in < block_count_max ?
            block_count_limit_in : block_count_max;

        for (size_t i = 0; i < copy_count; i = nf_add(i, 1)) {
            void* block_ptr = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                nf_mul(i, 8),
                8,
                &block_ptr,
                true
            );

            new_block_ptr_ram_space_agent_ptr.store_from_void_ptr(
                &block_ptr,
                8,
                nf_mul(i, 8),
                true
            );
        }

        // 7 如果是扩大且初始化类型是init_all_size，需要初始化新增部分
        if (block_count_limit_in > block_count_max && init_type_ == init_type::init_all_size) {
            mes::a_mes allocation_mes;
            for (size_t i = block_count_max; i < block_count_limit_in; i = nf_add(i, 1)) {
                bmms_f::agent_ptr new_block_agent_ptr;

                // 申请新块
                static_volume_shared_ptr->new_block(new_block_agent_ptr, allocation_mes);
                if (allocation_mes.code != 0) {
                    // 分配失败，需要回滚：释放已分配的新块
                    for (size_t j = block_count_max; j < i; j = nf_add(j, 1)) {
                        bmms_f::agent_ptr allocated_block;
                        void* ptr = nullptr;

                        new_block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                            nf_mul(j, 8),
                            8,
                            &ptr,
                            true
                        );

                        if (ptr != nullptr) {
                            allocated_block = bmms_f::agent_ptr(ptr, get_block_size());
                            static_volume_shared_ptr->delete_block(allocated_block);
                        }
                    }

                    mes_out = bmms_m::err::service_err_static_volume_part_resize_not_enough_blocks();
                    return;
                }

                // 将新块的指针写入数组
                void* block_ptr = nullptr;
                new_block_agent_ptr.get_void_ptr(block_ptr);
                new_block_ptr_ram_space_agent_ptr.store_from_void_ptr(
                    &block_ptr,
                    8,
                    nf_mul(i, 8),
                    true
                );

                using_block_count = nf_add(using_block_count, 1);
            }
        }

        // 8 更新统计数据
        size_t old_block_count_max = block_count_max;
        block_count_max = block_count_limit_in;

        // 如果缩小了，需要更新已使用的块计数
        if (block_count_limit_in < old_block_count_max) {
            size_t new_using_count = 0;
            for (size_t i = 0; i < block_count_limit_in; i = nf_add(i, 1)) {
                void* block_ptr = nullptr;
                new_block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                    nf_mul(i, 8),
                    8,
                    &block_ptr,
                    true
                );

                if (block_ptr != nullptr) {
                    new_using_count = nf_add(new_using_count, 1);
                }
            }
            using_block_count = new_using_count;
        }

        // 9 替换旧空间
        block_ptr_ram_space = move(new_block_ptr_ram_space);
        block_ptr_ram_space_agent_ptr = move(new_block_ptr_ram_space_agent_ptr);

        // 10 成功
        mes_out = mes::a_mes();
    }

    // 释放块，使得可以产生空洞以减少对静态卷的使用
    // 使用已经释放的块时会自动重新从静态卷取空间
    void static_volume_part::free_block(size_t block_index_in) {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        if (block_index_in >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_block_index_out_of_range)
        }

        // 读取块指针
        void* block_ptr = nullptr;
        block_ptr_ram_space_agent_ptr.load_to_void_ptr(
            nf_mul(block_index_in, 8), 8, &block_ptr, true
        );

        if (block_ptr != nullptr) {
            // 创建代理指针释放块
            bmms_f::agent_ptr tmp_ap(block_ptr, get_block_size());
            static_volume_shared_ptr->delete_block(tmp_ap);

            // 清空指针数组
            void* null_ptr = nullptr;
            block_ptr_ram_space_agent_ptr.store_from_void_ptr(
                &null_ptr, 8, nf_mul(block_index_in, 8), true
            );

            using_block_count = nf_sub(using_block_count, 1);
        }
        // 如果 block_ptr == nullptr，静默成功（已释放）
    }

    void static_volume_part::free_range(
        size_t begin_in,
        size_t size_in
    ) {
        //1 初始化校验
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }

        //2 区间值校验
        check_range(begin_in, size_in);

        //3 变量准备
        size_t index_begin_out = 0;
        size_t index_count_out = 0;
        size_t index_end_out = 0;
        size_t op_begin_of_first_block = 0;
        size_t op_size_of_end_block = 0;
        bmms_f::agent_ptr block_agent_ptr = bmms_f::agent_ptr();
        bool   is_over_more_than_a_block = 0;
        bool   is_nullptr = false;

        //4 数据获取
        get_range_info(
            begin_in,
            size_in,
            index_begin_out,
            index_count_out,
            index_end_out,
            op_begin_of_first_block,
            op_size_of_end_block,
            is_over_more_than_a_block
        );

        // 5 释放起始块
        if (op_begin_of_first_block == 0) {
            // 5.1 获取块代理指针
            get_block_ptr(index_begin_out, block_agent_ptr, is_nullptr);
            if (!is_nullptr) {
                // 5.2 置空信息
                block_ptr_ram_space_agent_ptr.clear_range(
                    nf_mul(index_begin_out, 8),
                    8
                );

                // 5.3 释放块
                static_volume_shared_ptr->delete_block(block_agent_ptr);

                // 5.4 调整块计数
                using_block_count = nf_sub(using_block_count, 1);
            }
        }

        // 6 释放中间块
        if (is_over_more_than_a_block && index_count_out > 2) {
            for (
                size_t i = index_begin_out + 1;
                i < nf_sub(nf_add(index_begin_out, index_count_out), 1);
                i++
                ) {
                // 6.1 获取块代理指针
                get_block_ptr(i, block_agent_ptr, is_nullptr);


                if (!is_nullptr) {
                    // 6.2 置空信息
                    block_ptr_ram_space_agent_ptr.clear_range(
                        nf_mul(i, 8),
                        8
                    );

                    // 6.3 释放块
                    if (!is_nullptr) {
                        static_volume_shared_ptr->delete_block(block_agent_ptr);
                    }
                    
                    // 6.4 调整块计数
                    using_block_count = nf_sub(using_block_count, 1);
                }
            }
            
        }

        // 7 释放末尾块
        if (op_size_of_end_block == get_block_size()) {
            // 7.1 获取块代理指针
            get_block_ptr(index_end_out, block_agent_ptr, is_nullptr);
            if (!is_nullptr) {
                // 7.2 置空信息
                block_ptr_ram_space_agent_ptr.clear_range(
                    nf_mul(index_end_out, 8),
                    8
                );

                // 7.3 释放块
                static_volume_shared_ptr->delete_block(block_agent_ptr);

                // 7.4 调整块计数
                using_block_count = nf_sub(using_block_count, 1);
            }
        }
    };

    //======================================== 数据交换 ========================================

    // 将自身数据读取到代理指针
    void static_volume_part::load(
        size_t           load_begin_in,
        size_t           load_size_in,
        bmms_f::agent_ptr& save_agent_ptr_in,
        size_t           store_begin_in,
        bool             safe,
        mes::a_mes& mes_out
    ) {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        if (load_size_in == 0) {
            mes_out = mes::a_mes();
            return;
        }
        check_range(load_begin_in, load_size_in);

        size_t idx_begin, idx_count, idx_end, off_first, size_end;
        bool over;
        get_range_info(load_begin_in, load_size_in,
            idx_begin, idx_count, idx_end,
            off_first, size_end, over);

        size_t current_store = store_begin_in;
        size_t remaining = load_size_in;
        size_t blk_size = get_block_size();

        for (size_t i = idx_begin; i <= idx_end && remaining > 0; i = nf_add(i, 1)) {
            void* block_ptr = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                nf_mul(i, 8), 8, &block_ptr, true);

            size_t block_off = (i == idx_begin) ? off_first : 0;
            size_t bytes_this = blk_size - block_off;
            if (i == idx_end && over) {
                bytes_this = size_end;
            }
            bytes_this = std::min(bytes_this, remaining);

            if (block_ptr != nullptr) {
                bmms_f::agent_ptr block_agent(block_ptr, blk_size);
                block_agent.load(block_off, bytes_this,
                    save_agent_ptr_in, current_store, safe);
            }
            else {
                save_agent_ptr_in.clear_range(current_store, bytes_this);
            }

            current_store = nf_add(current_store, bytes_this);
            remaining = nf_sub(remaining, bytes_this);
        }
        mes_out = mes::a_mes();
    }

    // 从代理指针读取数据写入到自身
    void static_volume_part::store(
        bmms_f::agent_ptr& load_agent_ptr_in,
        size_t             load_begin_in,
        size_t             load_size_in,
        size_t             store_begin_in,
        bool               safe,
        mes::a_mes& mes_out
    ) {
        // 1 初始化校验
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        if (load_size_in == 0) {
            mes_out = mes::a_mes();
            return;
        }
        check_range(store_begin_in, load_size_in);

        // 2 计算范围信息
        size_t index_begin, index_count, index_end;
        size_t op_begin_first, op_size_end;
        bool is_over;
        get_range_info(store_begin_in, load_size_in,
            index_begin, index_count, index_end,
            op_begin_first, op_size_end, is_over);

        // 3 第一阶段：识别需要分配的块
        size_t blocks_to_allocate = 0;
        bool* need_allocate = nullptr;

        // 使用 unique_ptr 管理临时数组
        auto need_allocate_deleter = [](bool* p) { delete[] p; };
        unique_ptr<bool[], decltype(need_allocate_deleter)>
            need_allocate_guard(nullptr, need_allocate_deleter);

        try {
            need_allocate = new bool[index_count]();
            need_allocate_guard.reset(need_allocate);
        }
        catch (...) {
            mes_out = bmms_m::err::service_err_static_volume_part_store_not_enough_space();
            return;
        }

        // 遍历所有块，标记需要分配的
        for (size_t idx = 0; idx < index_count; ++idx) {
            size_t i = nf_add(index_begin, idx);
            void* block_ptr = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                nf_mul(i, 8), 8, &block_ptr, true);

            if (block_ptr == nullptr) {
                need_allocate[idx] = true;
                blocks_to_allocate = nf_add(blocks_to_allocate, 1);
            }
        }

        // 4 第二阶段：预先分配所有需要的块
        // 使用 unique_ptr 数组存储新分配的块指针
        using BlockArray = unique_ptr<bmms_f::agent_ptr[]>;
        BlockArray new_blocks;
        size_t* new_indices = nullptr;

        // 用于回滚的 guard
        auto indices_deleter = [](size_t* p) { delete[] p; };
        unique_ptr<size_t[], decltype(indices_deleter)>
            indices_guard(nullptr, indices_deleter);

        if (blocks_to_allocate > 0) {
            try {
                new_blocks = make_unique<bmms_f::agent_ptr[]>(blocks_to_allocate);
                new_indices = new size_t[blocks_to_allocate]();
                indices_guard.reset(new_indices);
            }
            catch (...) {
                mes_out = bmms_m::err::service_err_static_volume_part_store_not_enough_space();
                return;
            }

            // 分配所有需要的块
            size_t alloc_idx = 0;
            mes::a_mes alloc_mes;

            for (size_t idx = 0; idx < index_count; ++idx) {
                if (!need_allocate[idx]) continue;

                size_t i = nf_add(index_begin, idx);
                static_volume_shared_ptr->new_block(new_blocks[alloc_idx], alloc_mes);

                if (alloc_mes.code != 0) {
                    // 分配失败，需要释放已分配的块
                    for (size_t j = 0; j < alloc_idx; ++j) {
                        static_volume_shared_ptr->delete_block(new_blocks[j]);
                    }
                    mes_out = bmms_m::err::service_err_static_volume_part_store_not_enough_space();
                    return;
                }

                new_indices[alloc_idx] = i;
                alloc_idx = nf_add(alloc_idx, 1);
            }
        }

        // 5 第三阶段：更新指针数组（原子操作）
        size_t alloc_idx = 0;
        for (size_t idx = 0; idx < index_count; ++idx) {
            if (!need_allocate[idx]) continue;

            size_t i = new_indices[alloc_idx];
            void* block_ptr = nullptr;
            new_blocks[alloc_idx].get_void_ptr(block_ptr);

            // 写入指针数组
            block_ptr_ram_space_agent_ptr.store_from_void_ptr(
                &block_ptr, 8, nf_mul(i, 8), true);

            alloc_idx = nf_add(alloc_idx, 1);
        }

        // 更新使用计数
        using_block_count = nf_add(using_block_count, blocks_to_allocate);

        // 6 第四阶段：实际传输数据
        size_t current_load = load_begin_in;
        size_t remaining = load_size_in;
        size_t block_size_val = get_block_size();

        for (size_t idx = 0; idx < index_count && remaining > 0; ++idx) {
            size_t i = nf_add(index_begin, idx);

            // 获取块指针（可能来自新分配的或原有的）
            void* block_ptr = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                nf_mul(i, 8), 8, &block_ptr, true);

            // block_ptr 此时肯定非空（要么原有，要么新分配成功）
            size_t block_offset = (idx == 0) ? op_begin_first : 0;
            size_t bytes_this = block_size_val - block_offset;
            if (idx == index_count - 1 && is_over) {
                bytes_this = op_size_end;
            }
            bytes_this = std::min(bytes_this, remaining);

            bmms_f::agent_ptr block_agent(block_ptr, block_size_val);
            block_agent.store(load_agent_ptr_in,
                current_load, bytes_this,
                block_offset, safe);

            current_load = nf_add(current_load, bytes_this);
            remaining = nf_sub(remaining, bytes_this);
        }

        mes_out = mes::a_mes(); // 成功

        // unique_ptr 自动释放临时资源
    }

    // 将自身数据读取到void指针
    void static_volume_part::load_to_void_ptr(
        size_t      load_begin_in,
        size_t      load_size_in,
        void*       store_ptr_in,
        bool        safe,
        mes::a_mes& mes_out
    ) {
        // 构造临时的agent_ptr
        bmms_f::agent_ptr temp_agent_ptr(store_ptr_in, load_size_in);
        // 调用load函数
        load(
            load_begin_in,
            load_size_in,
            temp_agent_ptr,
            0,
            true,
            mes_out
        );
    };

    // 从void指针读数据写入自身
    void static_volume_part::store_from_void_ptr(
        void*       load_ptr_in,
        size_t      load_size_in,
        size_t      store_begin_in,
        bool        safe,
        mes::a_mes& mes_out
    ) {
        // 构造临时的agent_ptr
        bmms_f::agent_ptr temp_agent_ptr(load_ptr_in, load_size_in);
        // 调用store函数
        static_volume_part::store(
            temp_agent_ptr,
            0,
            load_size_in,
            store_begin_in,
            true,
            mes_out
        );
    };

    //======================================== 数据获取 ========================================

    size_t    static_volume_part::get_total_size() {
        // 1. 检查对象是否已初始化
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }

        // 2. 获取总大小
        return nf_mul(get_block_size(), block_count_max);
    };

    size_t    static_volume_part::get_block_count_max() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        else {
            return block_count_max;
        }
    };

    size_t    static_volume_part::get_block_size() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        else {
            return static_volume_shared_ptr->get_block_size();
        }
    };

    size_t    static_volume_part::get_block_count_used() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        else {
            return using_block_count;
        }
    };
    
    void      static_volume_part::get_block_ptr(
        size_t&          block_index_in,
        bmms_f::agent_ptr& block_agent_ptr_out,
        bool&            is_nullptr_out
    ) {
        // 1. 检查对象是否已初始化
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }

        // 2. 检查索引是否有效
        if (block_index_in >= block_count_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_range_out_of_range)
        }

        // 3. 从指针数组中获取块指针
        void* block_ptr = nullptr;
        block_ptr_ram_space_agent_ptr.load_to_void_ptr(
            block_index_in * 8,  // 每个指针8字节
            8,
            &block_ptr,
            true
        );

        // 4. 设置输出参数
        is_nullptr_out = (block_ptr == nullptr);

        // 5. 如果指针不为nullptr，创建agent_ptr返回
        if (!is_nullptr_out) {
            // 获取块大小
            size_t block_size = get_block_size();

            // 创建新的agent_ptr（注意：这里不转移所有权，只是提供访问接口）
            block_agent_ptr_out = bmms_f::agent_ptr(block_ptr, block_size);
        }
        else {
            // 如果指针为nullptr，返回空agent_ptr
            block_agent_ptr_out = bmms_f::agent_ptr();
        }
    }

    void      static_volume_part::get_range_info(
        size_t& op_begin_in,
        size_t& op_size_in,
        size_t& index_begin_out,
        size_t& index_count_out,
        size_t& index_end_out,
        size_t& op_begin_of_first_block_out,
        size_t& op_size_of_end_block_out,
        bool& is_over_more_than_a_block_out
    ) {
        size_t block_size = get_block_size();
        size_t op_end = 0;
        size_t end_block_index = 0;
        // 1. 起始块索引
        // op_begin_in / block_size
        index_begin_out = nf_div(op_begin_in, block_size);

        // 2. 起始块内的偏移
        // op_begin_in % block_size
        op_begin_of_first_block_out = nf_mod(op_begin_in, block_size);

        // 3. 总块数
        // op_end = op_begin_in + op_size_in
        op_end = nf_add(op_begin_in, op_size_in);
        // end_block_index = (op_end - 1) / block_size
        end_block_index = nf_div(nf_sub(op_end, 1), block_size);
        index_end_out = end_block_index;
        // index_count_out = end_block_index - index_begin_out + 1
        index_count_out = nf_add(nf_sub(end_block_index, index_begin_out), 1);

        // 4. 结束块中起始到需要操作的结尾的大小
        // 例子：
        // 当op_end在块内时:
        // [0,10)[10,20)[20,30)[30,40)[40,50) begin = 7 (第8字节) ，op_size = 10
        // => 操作第8到17字节 => 操作的区间是 [7,18) => 最后一个块[10,20)需要操作的区间是 [10,18)
        // => 最后一个块需要操作8字节
        // => op_size_of_end_block_out = op_end % block_size
        // 当op_end在块结尾时:
        // [0,10)[10,20)[20,30)[30,40)[40,50) begin = 10 (第11字节) ，op_size = 10 操作区间是[10,20)
        // =>op_size_of_end_block_out = block_size
        if (nf_mod(op_end, block_size) != 0) {
            op_size_of_end_block_out = nf_mod(op_end, block_size);
        }
        else {
            op_size_of_end_block_out = block_size;
        }

        if (index_begin_out != end_block_index) {
            is_over_more_than_a_block_out = true;
        }
        else {
            is_over_more_than_a_block_out = false;
        }
    }

    uintptr_t static_volume_part::__block_ptr_ram_space_begin__() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        else {
            return reinterpret_cast<uintptr_t>(block_ptr_ram_space.get());
        }
    };

    uintptr_t static_volume_part::__block_ptr_ram_space_end__() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        else {
            return nf_add(
                __block_ptr_ram_space_begin__(),
                nf_mul(block_count_max, 8)
            );
        }
    };

    //======================================== 校验 ========================================

    void static_volume_part::check_build_value_valid(mes::a_mes mes_out) {
        // 1 校验卷
        // 1.1 指针非空
        if (static_volume_shared_ptr == nullptr) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_build_from_null_ptr)
        }
        // 1.2 卷未初始化
        if (static_volume_shared_ptr->get_is_init() == false) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_build_volume_not_inited)
        }

        // 2 校验初始化类型
        if (   init_type_ != init_type::no_init_all_size 
            && init_type_ != init_type::init_all_size) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_build_init_type_invalid)
        }

        // 3 校验控制类型
        if (   control_type_ != control_type::allow_over_use 
            && control_type_ != control_type::not_allow_over_use) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_build_control_type_invalid)
        }

        // 4 校验块数量限制
        // 4.1 块数量非0
        if (block_count_max == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_build_block_count_zero)
        }
        // 4.2 块数量不超过index最大值
        if (block_count_max > bmms_f::index_max) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_build_block_count_over_index_max)
        }
        // 4.3 块数量与卷与控制策略匹配
        //if (control_type_ == control_type::allow_over_use) // 允许过用时就不需要匹配校验了
        if (   control_type_ == control_type::not_allow_over_use
            && block_count_max > static_volume_shared_ptr->get_free_block_count()) {
            mes_out = bmms_m::err::service_err_static_volume_part_build_static_volume_free_block_not_enough();
            return;
        }
    };

    void static_volume_part::check_contro_valid(
        mes::a_mes mes_out
    ) {
        if (init_type_ == init_type::init_all_size) {
            if (control_type_ == control_type::allow_over_use) {
                __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_build_init_and_control_invalid)
            }
            else if (control_type_ == control_type::not_allow_over_use) {
                if (block_count_max > static_volume_shared_ptr->get_free_block_count()) {
                    mes_out = bmms_m::err::service_err_static_volume_part_build_not_enough_free_blocks();
                    return;
                }
            }
        }
        //else if (init_type_ == init_type::no_init_all_size) {
            //pass
        //}
    };

    void static_volume_part::check_range(
        size_t begin_in,
        size_t size_in
    ) {
        if (size_in == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_range_zero)
        }
        if (nf_add(begin_in,size_in) > get_total_size()) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_range_out_of_range)
        }
    };

    //======================================== 辅助 ========================================

    size_t static_volume_part::nf_add(size_t a, size_t b) {
        if (a > SIZE_MAX - b) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_add_overflow);
        }
        return a + b;
    }

    size_t static_volume_part::nf_sub(size_t a, size_t b) {
        if (a < b) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_sub_overflow);
        }
        return a - b;
    }

    size_t static_volume_part::nf_mul(size_t a, size_t b) {
        if (b != 0 && a > SIZE_MAX / b) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_mul_overflow);
        }
        return a * b;
    }

    size_t static_volume_part::nf_div(size_t a, size_t b) {
        if (b == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_div_by_zero);
        }
        return a / b;
    }

    size_t static_volume_part::nf_mod(size_t a, size_t b) {
        // 定理：对于无符号整数，a % b 总是定义良好的，只要 b ≠ 0
        if (b == 0) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_mod_by_zero);
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

    void static_volume_part::init_blocks(mes::a_mes& mes_out) {
        mes_out = mes::a_mes();
        bmms_f::agent_ptr tmp_ap;
        void* block_void_ptr = nullptr;

        if (init_type_ == init_type::init_all_size) {
            for (size_t i = 0; i < block_count_max; i++) {
                static_volume_shared_ptr->new_block(tmp_ap, mes_out);

                if (mes_out.code != 0) {
                    // 回滚已分配的块
                    for (size_t j = 0; j < i; j++) {
                        void* ptr_to_free = nullptr;
                        block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                            nf_mul(j, 8), 8, &ptr_to_free, true
                        );
                        if (ptr_to_free != nullptr) {
                            bmms_f::agent_ptr block_to_free(ptr_to_free, get_block_size());
                            static_volume_shared_ptr->delete_block(block_to_free);
                        }
                    }
                    using_block_count = 0;
                    mes_out = bmms_m::err::service_err_static_volume_part_build_not_enough_free_blocks();
                    return;
                }

                tmp_ap.get_void_ptr(block_void_ptr);
                block_ptr_ram_space_agent_ptr.store_from_void_ptr(
                    &block_void_ptr, 8, nf_mul(i, 8), true
                );
            }
            using_block_count = block_count_max;
        }
        else {
            using_block_count = 0;
        }
        mes_out = mes::a_mes();
    }
};