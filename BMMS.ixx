//BMMS.ixx
//basic_memory_management_system

export module BMMS;

#include "BMMS_dp.h"

export namespace bmms_f {
    size_t index_max = 0x00FFFFFFFFFFFFFF;

    class agent_ptr {
    public:
        friend class agent_ptr;

        //============================ 构造 ============================

        // 默认构造函数 得到空指针
        agent_ptr() = default;

        // 地址&&大小构造函数
        agent_ptr(
            void* ptr,
            size_t size
        );

        // 将自身数据读取到代理指针
        void load(
            size_t load_begin_in,
            size_t load_size_in,
            agent_ptr& save_agent_ptr_in,
            size_t     store_begin_in,
            bool       safe
        );

        // 从代理指针读取数据写入到自身
        void store(
            agent_ptr& load_agent_ptr_in,
            size_t     load_begin_in,
            size_t     load_size_in,
            size_t     store_begin_in,
            bool       safe
        );

        //============================ 读写 ============================

        // 将自身数据读取到void指针
        void load_to_void_ptr(
            size_t load_begin_in,
            size_t load_size_in,
            void* store_ptr_in,
            bool   safe
        );

        // 从void指针读数据写入自身
        void store_from_void_ptr(
            void* load_ptr_in,
            size_t load_size_in,
            size_t store_begin_in,
            bool   safe
        );

        // 清空数据
        void clear();

        // 范围清空数据
        void clear_range(
            size_t begin_in,
            size_t size_in
        );

        //============================ 数据获取 ============================

        // 获取指针
        void get_void_ptr(void*& ptr_out);

        // 获取是否为空
        void get_is_null(bool& is_null_in);

        // 获取大小
        void get_size(size_t& size_out);

        // 获取起点
        void get_begin(void*& begin_out);

        // 获取终点
        void get_end(void*& end_out);

        // 获取范围检查布尔
        void get_check_range_bool(
            size_t begin_in,
            size_t size_in,
            bool& is_valid_out
        );

        // 获取构造无溢出
        void check_building_no_overflow(
            void*& ptr_in,
            size_t size_in,
            bool& building_is_no_overflow
        );

        // 获取操作区间运算无溢出
        void  check_end_no_overflow(
            size_t begin_in,
            size_t size_in,
            bool& is_no_overflow
        );

#if defined(BMMS_INCLUDE_TEST)
        void test();
#endif

        //============================ 判断 ============================

        //========== 信息获取 ==========

        inline uintptr_t __self_begin__();

        inline uintptr_t __self_end__();

        inline uintptr_t __op_begin__(
            size_t begin_in
        );

        inline uintptr_t __op_end__(
            size_t op_begin_in,
            size_t op_size_in
        );

        inline uintptr_t __void_ptr_begin__(
            void*& op_ptr_in
        );

        inline uintptr_t __void_ptr_end__(
            void*& op_ptr_in,
            size_t op_size_in
        );

        //========== 公共方法专有校验 ==========
        void check_building(
            void*& ptr_in,
            size_t& size_in
        );

        //========== 定理校验 ==========

        inline bool no_overflow(uintptr_t a, size_t b);

        // 空代理指针检查
        inline void check_null_agent_ptr(
            agent_ptr& other_agent_ptr_in
        );

        // 大小检查
        inline void check_size(
            size_t& self_op_size_in,
            size_t& other_op_size_in
        );

        // 双方代理指针部分重叠管理空间检查
        inline void check_self_and_other_managemented_range(
            uintptr_t& self_self_begin_in,
            uintptr_t& self_self_end_in,
            uintptr_t& other_self_begin_in,
            uintptr_t& other_self_end_in
        );

        // 操作区间有效检查
        inline void check_op_range_overlap(
            uintptr_t& self_self_begin_in,
            uintptr_t& self_self_end_in,
            uintptr_t& self_op_begin_in,
            uintptr_t& self_op_end_in_in,
            size_t& self_op_size,
            uintptr_t& other_self_begin_in,
            uintptr_t& other_self_end_in,
            uintptr_t& other_op_begin_in,
            uintptr_t& other_op_end_in,
            size_t& other_op_size
        );

        // 原地操作数据检查
        inline void check_op_on_the_spot(
            uintptr_t& self_op_begin_in,
            uintptr_t& self_op_end_in,
            uintptr_t& other_op_begin_in,
            uintptr_t& other_op_end_in
        );

        // 操作范围不重叠检查
        inline void check_op_range_not_overlapping(
            uintptr_t& self_begin_in,
            uintptr_t& self_end_in,
            uintptr_t& other_begin_in,
            uintptr_t& other_end_in
        );

        // 空void指针检查
        inline void check_null_void_ptr(void* ptr_in);

        //代理指针管理的空间 与 void*指针操作的空间 不 部分重叠 检查
        inline void check_self_managemented_range_and_void_ptr_op_range_not_overlapping(
            uintptr_t& self_self_begin_in,
            uintptr_t& self_self_end_in,
            uintptr_t& self_op_begin_in,
            uintptr_t& self_op_end_in_in,
            uintptr_t& void_ptr_op_begin_in,
            uintptr_t& void_ptr_op_end_in
        );

        // self与void_ptr操作的大小有效性检验
        inline void check_self_and_void_ptr_op_size(
            size_t& self_op_size_in,
            size_t& void_ptr_op_size_in
        );

        // 操作的区间有效性检验
        inline void check_self_op_range(
            uintptr_t& self_self_begin_in,
            uintptr_t& self_self_end_in,
            uintptr_t& self_op_begin_in,
            uintptr_t& self_op_end_in_in
        );

        void* raw_ptr = 0;
        size_t size = 0;
    };

    class static_volume {
    public:

        enum init_type {
            no_init_all_size  = 0, // 初始化所有字节
            init_all_size     = 1, // 实际分配时才初始化写入的块
        };

        enum alignment_type {
            without_alignment   = 0,
            byte_4_each_block   = 1,
            byte_8_each_block   = 2,
            byte_16_each_block  = 3,
            byte_32_each_block  = 4,
            byte_64_each_block  = 5
        };

        //======================================== 访问控制 ========================================

        //禁止复制
        static_volume(const static_volume&) = delete;
        static_volume& operator=(const static_volume&) = delete;

        //允许移动
        static_volume(static_volume&& other) noexcept;
        static_volume& operator=(static_volume&& other) noexcept;

        //======================================== 构造与析构 ========================================

        // 默认构造函数
        static_volume() = delete;

        // 参数构造函数
        static_volume(
            size_t         block_size_in,
            size_t         block_count_max_in,
            init_type      init_type_in,
            alignment_type alignment_type_in,
            mes::a_mes&    mes_out
        );

        // 析构函数
        ~static_volume();

        //======================================== 内存分配 ========================================

        void new_block(
            bmms_f::agent_ptr& agent_ptr_out,
            mes::a_mes&      mes_out
        );

        void delete_block(
            bmms_f::agent_ptr& agent_ptr_in
        );

        //============================ 公共数据获取 ============================
        
        bool   get_is_init();

        size_t get_block_count_max();

        size_t get_free_block_count();

        size_t get_block_size();

        size_t __total_ram_size__();

        size_t __recycle_stack_size__();

        size_t __actually_block_size__();

        size_t __block_space_size__();

        uintptr_t __block_space_begin__();

        uintptr_t __block_space_end__();

        uintptr_t __stack_begin__();

        uintptr_t __stack_end__();

        //============================ 定理校验函数 ============================
        
        inline void check_total_size_no_overflow();

        inline void check_enum_init_type_valid(
            init_type init_type_in
        );

        inline void check_enum_alignment_type_valid(
            alignment_type alignment_type_in
        );

        inline void check_block_belong_to_self(
            bmms_f::agent_ptr& agent_ptr_in
        );

        inline void check_can_push(
            bmms_f::agent_ptr& agent_ptr_in
        );

        inline bool check_can_pop();

        //============================ 测试 ============================
        #if defined(BMMS_INCLUDE_TEST)
        void test();
        #endif

    private:
        //============================ 私有数据获取 ============================

        void* get_block_ptr(size_t block_index);

        size_t get_block_index(bmms_f::agent_ptr& block_agent_ptr_in);

        //============================ 运算 ============================

        // 带溢出检测的四则运算，因为溢出就视为编码错误，然后抛出异常或者结束进程
        inline size_t nf_add(size_t a, size_t b); // 无溢出加法
        inline size_t nf_sub(size_t a, size_t b); // 无溢出减法
        inline size_t nf_mul(size_t a, size_t b); // 无溢出乘法
        inline size_t nf_div(size_t a, size_t b); // 无溢出除法
        inline size_t nf_mod(size_t a, size_t b); // 无溢出取模


        //============================ 初始化 ============================

        inline void init_recycle_stack();

        inline void init_a_block(
            void* block_begin_void_ptr_in
        );

        inline void init_block_ram_space();


        //============================ 栈操作 ============================
        
        inline void push_to_recycle_stack(
            bmms_f::agent_ptr& block_agent_ptr_in
        );

        inline void pop_from_recycle_stack(
            bmms_f::agent_ptr& block_agent_ptr_out,
            mes::a_mes&      mes_out
        );

        // 核心状态
        bool           is_init         = false;                             // 1 byte
        byte_1         init_type_      = init_type::no_init_all_size;       // 1 byte
        byte_1         alignment_type_ = alignment_type::without_alignment; // 1 byte
        byte_1         fill_0[5];                                           // 5 byte
        size_t         block_size            = 0;                           // 8 byte
        size_t         block_count_max       = 0;                           // 8 byte
        size_t*        recycle_stack_top_ptr = nullptr;                     // 8 byte

        unique_ptr<byte_1[]> ram_space       = nullptr;                     // 8 byte
        byte_1               fill_1[24];                                    // 24 byte
        bmms_f::agent_ptr      block_ram_space;                             // 16 byte
        bmms_f::agent_ptr      stack_index_ram_space;                       // 16 byte
    };
    
    class static_volume_ptr {
    public:
        friend class static_volume_ptr;

        //======================================== 构造与析构 ========================================
        static_volume_ptr() = delete;

        static_volume_ptr(
            shared_ptr<static_volume>& static_volume_shared_ptr_in,
            mes::a_mes& mes_out
        );

        ~static_volume_ptr();
        //======================================== 访问控制 ========================================

        //禁止复制
        static_volume_ptr(const static_volume_ptr&) = delete;
        static_volume_ptr& operator=(const static_volume_ptr&) = delete;

        //允许移动
        static_volume_ptr(static_volume_ptr&& other) noexcept;
        static_volume_ptr& operator=(static_volume_ptr&& other) noexcept;

        //======================================== 数据控制 ========================================

        // 清空数据
        void clear();

        // 范围清空数据
        void clear_range(
            size_t begin_in,
            size_t size_in
        );

        //======================================== 数据交换 ========================================

        // 将自身数据读取到代理指针
        void load(
            size_t load_begin_in,
            size_t load_size_in,
            bmms_f::agent_ptr& save_agent_ptr_in,
            size_t     store_begin_in,
            bool       safe
        );

        // 从代理指针读取数据写入到自身
        void store(
            bmms_f::agent_ptr& load_agent_ptr_in,
            size_t     load_begin_in,
            size_t     load_size_in,
            size_t     store_begin_in,
            bool       safe
        );

        // 将自身数据读取到void指针
        void load_to_void_ptr(
            size_t load_begin_in,
            size_t load_size_in,
            void* store_ptr_in,
            bool   safe
        );

        // 从void指针读数据写入自身
        void store_from_void_ptr(
            void* load_ptr_in,
            size_t load_size_in,
            size_t store_begin_in,
            bool   safe
        );

        //======================================== 校验 ========================================
        
        void check_static_volume_value_valid();

        void check_is_init();

        //============================ 测试 ============================
        #if defined(BMMS_INCLUDE_TEST)
        void test();
        #endif

    private:
        bool is_init = false;
        byte_1 fill_0[7];
        bmms_f::agent_ptr agent_ptr = bmms_f::agent_ptr();
        shared_ptr<static_volume> static_volume_shared_ptr = nullptr;
    };
    
    class static_volume_part {
    public:
        //======================================== 构造与析构 ========================================
        
        enum init_type {
            no_init_all_size = 0, // 初始化时申请所有块
            init_all_size    = 1, // 实际使用时才申请块
        };

        enum control_type {
            allow_over_use     = 0,// 允许最大逻辑地址超出卷块极限使用，但是最大块数不得超出卷块总数
            not_allow_over_use = 1 // 不允许如上
        };
        
        static_volume_part() = delete;

        static_volume_part(
            shared_ptr<static_volume>& static_volume_shared_ptr_in,
            size_t                     block_count_max_in,
            init_type                  init_type_in,
            control_type               control_type_in,
            mes::a_mes&                mes_out
        );

        ~static_volume_part();

        //======================================== 访问控制 ========================================

        //禁止复制
        static_volume_part(const static_volume_part&) = delete;
        static_volume_part& operator=(const static_volume_part&) = delete;

        //允许移动
        static_volume_part(static_volume_part&& other) noexcept;
        static_volume_part& operator=(static_volume_part&& other) noexcept;

        //======================================== 数据控制 ========================================
        
        void clear();

        void clear_range(
            size_t begin_in,
            size_t size_in
        );

        void resize(
            size_t block_count_limit_in,
            mes::a_mes& mes_out
        );

        // 释放块，使得可以产生空洞以减少对静态卷的使用
        // 使用已经释放的块时会自动重新从静态卷取空间
        void free_block(
            size_t block_index_in
        );

        void free_range(
            size_t begin_in,
            size_t size_in
        );

        //======================================== 数据交换 ========================================

        // 将自身数据读取到代理指针
        void load(
            size_t           load_begin_in,
            size_t           load_size_in,
            bmms_f::agent_ptr& save_agent_ptr_in,
            size_t           store_begin_in,
            bool             safe,
            mes::a_mes&      mes_out
        );

        // 从代理指针读取数据写入到自身
        void store(
            bmms_f::agent_ptr& load_agent_ptr_in,
            size_t           load_begin_in,
            size_t           load_size_in,
            size_t           store_begin_in,
            bool             safe,
            mes::a_mes&      mes_out
        );

        // 将自身数据读取到void指针
        void load_to_void_ptr(
            size_t      load_begin_in,
            size_t      load_size_in,
            void*       store_ptr_in,
            bool        safe,
            mes::a_mes& mes_out
        );

        // 从void指针读数据写入自身
        void store_from_void_ptr(
            void*       load_ptr_in,
            size_t      load_size_in,
            size_t      store_begin_in,
            bool        safe,
            mes::a_mes& mes_out
        );

        //======================================== 数据获取 ========================================

        size_t get_total_size();

        size_t get_block_count_max();

        size_t get_block_size();

        size_t get_block_count_used();

        inline void   get_block_ptr(
            size_t& block_index_in,
            bmms_f::agent_ptr& block_agent_ptr_out,
            bool& is_nullptr_out
        );

        inline void   get_range_info(
            size_t& op_begin_in,
            size_t& op_size_in,
            size_t& index_begin_out,
            size_t& index_count_out,
            size_t& index_end_out,
            size_t& op_begin_of_first_block_out,
            size_t& op_size_of_end_block_out,
            bool  & is_over_more_than_a_block_out
        );

        inline uintptr_t __block_ptr_ram_space_begin__();

        inline uintptr_t __block_ptr_ram_space_end__();

        //======================================== 校验 ========================================
        
        inline void check_build_value_valid(
            mes::a_mes mes_out
        );

        inline void check_contro_valid(
            mes::a_mes mes_out
        );

        inline void check_range(
            size_t begin_in,
            size_t size_in
        );

        //======================================== 辅助 ========================================

        // 带溢出检测的四则运算，因为溢出就视为编码错误，然后抛出异常或者结束进程
        inline size_t nf_add(size_t a, size_t b); // 无溢出加法
        inline size_t nf_sub(size_t a, size_t b); // 无溢出减法
        inline size_t nf_mul(size_t a, size_t b); // 无溢出乘法
        inline size_t nf_div(size_t a, size_t b); // 无溢出除法
        inline size_t nf_mod(size_t a, size_t b); // 无溢出取模

        void init_blocks(mes::a_mes& mes_out);

        //============================ 测试 ============================
        #if defined(BMMS_INCLUDE_TEST)
        void test();
        #endif

        //======================================== 数据 ========================================

        bool is_init         = false;
        byte_1 init_type_    = init_type::no_init_all_size;
        byte_1 control_type_ = control_type::not_allow_over_use;

        byte_1 fill_0[5];

        size_t block_count_max   = 0;
        size_t using_block_count = 0;

        shared_ptr<static_volume> static_volume_shared_ptr    = nullptr;

        unique_ptr<void*[]>       block_ptr_ram_space         = nullptr;
        bmms_f::agent_ptr           block_ptr_ram_space_agent_ptr;
    };

    class free_part {
    public:
        friend class free_volume;
        //======================================== 构造与析构 ========================================

        enum init_type {
            no_init_all_size = 0, // 初始化时申请所有块
            init_all_size = 1,    // 实际使用时才申请块
        };

        free_part() = delete;

        free_part(
            size_t      block_size_in,
            size_t      block_count_max_in,
            init_type   init_type_in,
            mes::a_mes& mes_out
        );

        ~free_part();

        //======================================== 访问控制 ========================================

        //禁止复制
        free_part(const free_part&) = delete;
        free_part& operator=(const free_part&) = delete;

        //允许移动
        free_part(free_part&& other) noexcept;
        free_part& operator=(free_part&& other) noexcept;

        //======================================== 数据控制 ========================================

        void clear();

        void clear_range(
            size_t begin_in,
            size_t size_in
        );

        void resize(
            size_t block_count_limit_in,
            mes::a_mes& mes_out
        );

        // 释放块，使得可以产生空洞以减少对静态卷的使用
        // 使用已经释放的块时会自动重新从静态卷取空间
        void free_block(
            size_t block_index_in
        );

        void free_range(
            size_t begin_in,
            size_t size_in
        );

        //======================================== 数据交换 ========================================

        // 将自身数据读取到代理指针
        void load(
            size_t           load_begin_in,
            size_t           load_size_in,
            bmms_f::agent_ptr& save_agent_ptr_in,
            size_t           store_begin_in,
            bool             safe
        );

        // 从代理指针读取数据写入到自身
        void store(
            bmms_f::agent_ptr& load_agent_ptr_in,
            size_t           load_begin_in,
            size_t           load_size_in,
            size_t           store_begin_in,
            bool             safe
        );

        // 将自身数据读取到void指针
        void load_to_void_ptr(
            size_t load_begin_in,
            size_t load_size_in,
            void* store_ptr_in,
            bool   safe
        );

        // 从void指针读数据写入自身
        void store_from_void_ptr(
            void* load_ptr_in,
            size_t load_size_in,
            size_t store_begin_in,
            bool   safe
        );

        //======================================== 数据获取 ========================================

        size_t get_total_size();

        size_t get_block_count_max();

        size_t get_block_size();

        size_t get_block_count_used();

        inline void   get_block_ptr(
            size_t& block_index_in,
            bmms_f::agent_ptr& block_agent_ptr_out,
            bool& is_nullptr_out
        );

        inline void   get_range_info(
            size_t& op_begin_in,
            size_t& op_size_in,
            size_t& index_begin_out,
            size_t& index_count_out,
            size_t& index_end_out,
            size_t& op_begin_of_first_block_out,
            size_t& op_size_of_end_block_out,
            bool& is_over_more_than_a_block_out
        );

        inline uintptr_t __block_ptr_ram_space_begin__();

        inline uintptr_t __block_ptr_ram_space_end__();

        //======================================== 校验 ========================================

        inline void check_build_value_valid(
            mes::a_mes& mes_out
        );

        inline void check_contro_valid(
            mes::a_mes& mes_out
        );

        inline void check_range(
            size_t begin_in,
            size_t size_in
        );

        //======================================== 辅助 ========================================

        // 带溢出检测的四则运算，因为溢出就视为编码错误，然后抛出异常或者结束进程
        inline size_t nf_add(size_t a, size_t b); // 无溢出加法
        inline size_t nf_sub(size_t a, size_t b); // 无溢出减法
        inline size_t nf_mul(size_t a, size_t b); // 无溢出乘法
        inline size_t nf_div(size_t a, size_t b); // 无溢出除法
        inline size_t nf_mod(size_t a, size_t b); // 无溢出取模

        void init_blocks(mes::a_mes& mes_out);

        //============================ 测试 ============================
        #if defined(BMMS_INCLUDE_TEST)
        void test();
        #endif

    private:

        //======================================== 数据 ========================================

        bool is_init = false;
        byte_1 init_type_    = init_type::no_init_all_size;

        byte_1 fill_0[6];

        size_t block_size = 0;  // 添加这个成员
        size_t block_count_max = 0;
        size_t using_block_count = 0;

        unique_ptr<unique_ptr<byte_1[]>[]> block_ptr_ram_space = nullptr;
        bmms_f::agent_ptr                    block_ptr_ram_space_agent_ptr;

        byte_8 fill_1;
    };

    class free_volume{
    public:

        enum init_type {
            no_init_all_size = 0, // 初始化所有字节
            init_all_size    = 1, // 实际分配时才初始化写入的块，默认过用！！！
        };

        enum alignment_type {
            without_alignment = 0,
            byte_4_each_block = 1,
            byte_8_each_block = 2,
            byte_16_each_block = 3,
            byte_32_each_block = 4,
            byte_64_each_block = 5
        };

        //======================================== 访问控制 ========================================

        //禁止复制
        free_volume(const free_volume&) = delete;
        free_volume& operator=(const free_volume&) = delete;

        //允许移动
        free_volume(free_volume&& other) noexcept;
        free_volume& operator=(free_volume&& other) noexcept;

        //======================================== 构造与析构 ========================================

        // 默认构造函数
        free_volume() = delete;

        // 参数构造函数
        free_volume(
            size_t         block_size_in,
            size_t         block_count_max_in,
            init_type      init_type_in,
            alignment_type alignment_type_in,
            mes::a_mes& mes_out
        );

        // 析构函数
        ~free_volume();

        //======================================== 内存分配 ========================================

        void new_block(
            bmms_f::agent_ptr& agent_ptr_out,
            size_t&            index_out,
            mes::a_mes& mes_out
        );

        void delete_block(
            bmms_f::agent_ptr& agent_ptr_in,
            size_t&            index_in
        );

        //======================================== 物理内存释放 ========================================

        // 释放单个空闲块的物理内存（如果该块是空闲状态）
        // 块索引必须有效，如果块正在使用中则报编码错误
        void free_unused_block(
            size_t block_index_in
        );

        // 释放一段范围内所有空闲块的物理内存
        // 自动跳过已分配的块，对已释放物理内存的块无影响
        void free_range(
            size_t begin_index_in,
            size_t size_in
        );

        // 释放所有空闲块的物理内存
        // 等效于 free_range(0, block_count_max)
        void free_all_unused_blocks();

        //============================ 公共数据获取 ============================

        bool   get_is_init();

        size_t get_block_count_max();

        size_t get_block_count_used();

        size_t get_free_block_count();

        size_t get_block_size();
       
        inline size_t    __recycle_stack_size__();

        inline size_t    __actually_block_size__();

        inline uintptr_t __block_ptr_space_begin__();

        inline size_t    __block_ptr_space_size__();

        inline uintptr_t __block_ptr_space_end__();

        inline uintptr_t __stack_space_begin__();

        inline size_t    __stack_space_size__();

        inline uintptr_t __stack_space_end__();

        inline size_t __block_ptr_offset__(size_t block_index_in);

        //============================ 定理校验函数 ============================

        inline void check_init_value_valid();

        inline void check_block_belong_to_self(
            bmms_f::agent_ptr& block_agent_ptr_in,
            size_t block_index_in
        );

        inline void check_can_push(
            bmms_f::agent_ptr& block_agent_ptr_in,
            size_t block_index_in
        );

        inline bool check_can_pop();

        //============================ 测试 ============================
        #if defined(BMMS_INCLUDE_TEST)
        void test();
        #endif

    private:
        //============================ 私有数据获取 ============================

        inline size_t get_block_ptr_ram_space_offset(
            bmms_f::agent_ptr& block_agent_ptr_in,
            size_t block_index_in
        );

        //============================ 运算 ============================

        // 带溢出检测的四则运算，因为溢出就视为编码错误，然后抛出异常或者结束进程
        inline size_t nf_add(size_t a, size_t b); // 无溢出加法
        inline size_t nf_sub(size_t a, size_t b); // 无溢出减法
        inline size_t nf_mul(size_t a, size_t b); // 无溢出乘法
        inline size_t nf_div(size_t a, size_t b); // 无溢出除法
        inline size_t nf_mod(size_t a, size_t b); // 无溢出取模

        //============================ 初始化 ============================

        inline void init_recycle_stack();

        inline void init_a_block(
            size_t block_index_in,
            mes::a_mes& mes_out
        );

        inline void init_block_ram_space(
            mes::a_mes& mes_out
        );

        //============================ 私有辅助 ============================

        inline void free_a_block(size_t block_index_in);

        inline bool is_block_free(size_t block_index_in);

        inline bool is_block_allocated(size_t block_index_in);

        //============================ 栈操作 ============================

        inline void push_to_recycle_stack(
            bmms_f::agent_ptr& block_agent_ptr_in,
            size_t             index_in
        );

        inline void pop_from_recycle_stack(
            bmms_f::agent_ptr& block_agent_ptr_out,
            size_t& index_out,
            mes::a_mes& mes_out
        );

        // 核心状态
        bool           is_init         = false;                             // 1 byte
        byte_1         init_type_      = init_type::no_init_all_size;       // 1 byte
        byte_1         alignment_type_ = alignment_type::without_alignment; // 1 byte
        byte_1         fill_0[5];                                           // 5 byte

        size_t         block_size = 0;                                      // 8 byte
        size_t         block_count_max = 0;                                 // 8 byte
        size_t         using_block_count = 0;                               // 8 byte
        size_t         actually_block_size = 0;                             // 8 byte

        size_t*                            recycle_stack_top_ptr = nullptr; // 8 byte
        unique_ptr<byte_1[]>               stack_ram_space       = nullptr; // 8 byte
        unique_ptr<unique_ptr<byte_1[]>[]> block_ptr_ram_space   = nullptr; // 8 byte

        bmms_f::agent_ptr      block_ram_space;                             // 16 byte
        bmms_f::agent_ptr      stack_index_ram_space;                       // 16 byte
        byte_1                 fill_2[32];                                  // 32 byte
    };

    class free_volume_ptr {
    public:
        friend class free_volume_ptr;

        //======================================== 构造与析构 ========================================
        free_volume_ptr() = delete;

        // 参数构造函数：从 free_volume 分配一个块
        free_volume_ptr(
            shared_ptr<free_volume>& free_volume_shared_ptr_in,
            mes::a_mes& mes_out
        );

        // 析构函数：自动释放块
        ~free_volume_ptr();

        //======================================== 访问控制 ========================================

        // 禁止复制
        free_volume_ptr(const free_volume_ptr&) = delete;
        free_volume_ptr& operator=(const free_volume_ptr&) = delete;

        // 允许移动
        free_volume_ptr(free_volume_ptr&& other) noexcept;
        free_volume_ptr& operator=(free_volume_ptr&& other) noexcept;

        //======================================== 数据控制 ========================================

        // 清空整个块
        void clear();

        // 范围清空数据
        void clear_range(
            size_t begin_in,
            size_t size_in
        );

        //======================================== 数据交换 ========================================

        // 将自身数据读取到代理指针
        void load(
            size_t           load_begin_in,
            size_t           load_size_in,
            bmms_f::agent_ptr& save_agent_ptr_in,
            size_t           store_begin_in,
            bool             safe
        );

        // 从代理指针读取数据写入到自身
        void store(
            bmms_f::agent_ptr& load_agent_ptr_in,
            size_t           load_begin_in,
            size_t           load_size_in,
            size_t           store_begin_in,
            bool             safe
        );

        // 将自身数据读取到void指针
        void load_to_void_ptr(
            size_t load_begin_in,
            size_t load_size_in,
            void* store_ptr_in,
            bool   safe
        );

        // 从void指针读数据写入自身
        void store_from_void_ptr(
            void* load_ptr_in,
            size_t load_size_in,
            size_t store_begin_in,
            bool   safe
        );

        //======================================== 索引获取 ========================================

        // 获取块索引（free_volume 需要 index 才能 delete）
        size_t get_index() const;

        // 获取底层代理指针（用于直接操作）
        bmms_f::agent_ptr& get_agent_ptr();

        //======================================== 校验 ========================================

        void check_free_volume_value_valid();
        void check_is_init();

        //============================ 测试 ============================
        #if defined(BMMS_INCLUDE_TEST)
        void test();
        #endif

    private:
        //======================================== 状态 ========================================
        bool           is_init = false;
        byte_1         fill_0[7];                     // 填充到8字节

        size_t         block_index = 0;               // 块索引（关键！）
        bmms_f::agent_ptr agent_ptr = bmms_f::agent_ptr();

        shared_ptr<free_volume> free_volume_shared_ptr = nullptr;
    };

};