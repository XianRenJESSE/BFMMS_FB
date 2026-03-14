//LFMS.ixx
//logical file memory management system

export module LFMMS;

#include "LFMMS_dp.h"

export namespace lfmms_f {
	size_t          index_max               = 0x00FFFFFFFFFFFFFF;
    size_t          version                 = 0x0000000000000001;

    class math {
    public:
        static size_t nf_add(size_t a, size_t b); // 无溢出加法
        static size_t nf_sub(size_t a, size_t b); // 无溢出减法
        static size_t nf_mul(size_t a, size_t b); // 无溢出乘法
        static size_t nf_div(size_t a, size_t b); // 无溢出除法
        static size_t nf_mod(size_t a, size_t b); // 无溢出取模
    };

    class cache_agent_ptr : private bmms_f::agent_ptr {
    public:
        // 继承构造函数
        using bmms_f::agent_ptr::agent_ptr;

        // 禁止复制
        cache_agent_ptr(const cache_agent_ptr&) = delete;
        cache_agent_ptr& operator=(const cache_agent_ptr&) = delete;

        // 允许移动
        cache_agent_ptr(cache_agent_ptr&& other) noexcept;
        cache_agent_ptr& operator=(cache_agent_ptr&& other) noexcept;

        //============================ 构造 ============================

        // 默认构造函数
        cache_agent_ptr() = default;

        //============================ 读写操作 ============================

        // 将自身数据读取到代理指针
        void load(
            size_t load_begin_in,
            size_t load_size_in,
            cache_agent_ptr& save_agent_ptr_in,
            size_t store_begin_in,
            bool safe
        );

        // 从代理指针读取数据写入到自身
        void store(
            cache_agent_ptr& load_agent_ptr_in,
            size_t load_begin_in,
            size_t load_size_in,
            size_t store_begin_in,
            bool safe
        );

        //================= 零拷贝操作 =================

        // ！！！非专家请勿使用 ！！！

        // ！！！  请谨慎使用   ！！！

        // ！！！T.function必须检查this != nullptr 后才开始操作数据，否则会导致未定义行为         ！！！
        // ！！！ 严禁复制 输出的T*，甚至把复制的T*超出cache_RAII_lock的作用域使用，这是最严重的UB！！！
        /*
         * 任何违反都会导致：
         *       - 数据随机损坏
         *       - 段错误
         *       - 程序完全错乱
         *       - 难以调试的诡异bug
        */

        template<typename T>
        void make_type(
            T* ptr_out
        ) {
#define EXIT_FAILURE 1

#if defined(LFMMS_INCLUDE_TEST)
#define __CODING_ERROR__(mes)\
			    mes().out(); \
			    throw mes();
#else
#define __CODING_ERROR__(mes)\
			    mes().out();std::exit(EXIT_FAILURE);
#endif

            if (sizeof(T) != size) {
                __CODING_ERROR__(lfmms_m::err::coding_error_cache_agent_ptr_make_type_over_range);

            }
            if (made_void_ptr_ptr != nullptr) {
                __CODING_ERROR__(lfmms_m::err::coding_error_cache_agent_ptr_make_type_again);
            }

            ptr_out = static_cast<T*>(raw_ptr);
            made_void_ptr_ptr = &ptr_out;
        };

        // 清空数据
        using bmms_f::agent_ptr::clear;

        // 范围清空数据
        using bmms_f::agent_ptr::clear_range;

        // 置空
        void release();

#if defined(LFMMS_INCLUDE_TEST)
        void test();
#endif

    private:
        void* made_void_ptr_ptr = nullptr;
        size_t fill;
        // 完全隐藏元数据访问
        // 通过私有继承，基类的所有公共成员在派生类中都变为私有
    };

	class logical_volume {
    public:
        enum class init_type {
            no_init_all_size = 0, // 初始化所有字节
            init_all_size    = 1, // 实际分配时才初始化写入的块，默认过用！！！
            no_init          = 255
        };

        enum class use_control {
            not_allow_over_use = 0,
            allow_over_use     = 1,
            no_init            = 255
        };

        enum class logical_block_alignment_type {
            without_alignment   = 0,
            byte_4_each_block   = 1,
            byte_8_each_block   = 2,
            byte_16_each_block  = 3,
            byte_32_each_block  = 4,
            byte_64_each_block  = 5,
            byte_128_each_block = 6,
            no_init             = 255
        };

        enum class logical_block_alignment_control {
            not_over_physical_block = 0,  //逻辑块不跨越物理块边界
            over_physical_block     = 1,   //逻辑块跨越物理块边界
            no_init                 = 255
        };

        enum class file_control {
            open_existed     = 0,
            cover_and_create = 1,
            create           = 2,
            no_init          = 255
        };

        enum class permission {
            ___     = 0b0000,
            __x     = 0b0001,
            _w_     = 0b0010,
            _wx     = 0b0011,
            r__     = 0b0100,
            r_x     = 0b0101,
            rw_     = 0b0110,
            rwx     = 0b0111,
            no_init = 0b1111
        };

        enum class load_control {
            auto_load     = 0,  //自动识别卷文件,忽略指定的参数
            not_auto_load = 1,  //手动指定参数
            no_init       = 255
        };

        enum class cache_control {
            static_size  = 0,
            dynamic_size = 1,
            no_init      = 255
        };

        enum class memory_cache_control {
            no_cache   = 0,
            cache_all  = 1,
            cache_part = 2,
            no_init    = 255
        };

        enum class stack_accelerate {
            accelerate     = 0,
            not_accelerate = 1,
            no_init        = 255
        };

        //======================================== 访问控制 ========================================

        //禁止复制
        logical_volume(const logical_volume&)             = delete;
        logical_volume& operator=(const logical_volume&)  = delete;

        //禁止移动
        logical_volume(logical_volume&& other)            = delete;
        logical_volume& operator=(logical_volume&& other) = delete;

        //======================================== 构造与析构 ========================================

        // 默认构造函数
        logical_volume() = default;

        // 析构函数
        ~logical_volume();

        //======================================== 句柄分配 ========================================

        void new_block(
            size_t&     index_out,
            size_t&     version_out,
            mes::a_mes& mes_out
        );

        void delete_block(
            size_t& index_in,
            size_t& version_in,
            shared_ptr<logical_volume>& logical_volume_ptr_in
        );

        //======================================== 物理存储释放 ========================================

        // 释放单个空闲块的物理内存（如果该块是空闲状态）
        // 块索引必须有效，如果块正在使用中则报编码错误
        void free_unused_physical_block(
            size_t      block_index_in,
            mes::a_mes& mes_out
        );

        // 释放一段范围内所有空闲块的物理内存
        // 自动跳过已分配的块，对已释放物理内存的块无影响
        void free_range(
            size_t      begin_index_in,
            size_t      size_in,
            mes::a_mes& mes_out
        );

        // 释放所有空闲块的物理内存
        // 等效于 free_range(0, block_count_max)
        void free_all_unused_blocks(
            mes::a_mes& mes_out
        );

        // 释放逻辑块的内存缓存（自动pull）
        void free_logical_block_memory_cache(
            size_t      begin_index_in,
            size_t      size_in,
            mes::a_mes& mes_out
        );

    private:
        class BOOT {
        public:
            BOOT(logical_volume* this_ptr_in);

            void BOOT_START(mes::a_mes& mes_out);

            // 创建引导
            void CREATE_VOLUME(
                file_control file_control_in,  // 创建如果存在则失败 或 创建且覆盖
                str          logical_volume_file_path_in,
                mes::a_mes& mes_out
            );
            //基础数据
            void SET_DATA_SIZE(
                size_t   logical_block_size_in,        //逻辑块大小
                size_t   physical_block_size_in,       //物理块大小
                size_t   logical_block_count_max_in,   //最大逻辑块数量
                size_t   physical_block_count_max_in,  //最大物理块数量
                mes::a_mes& mes_out

            );
            void SET_DATA_CONTROL_MOD(
                logical_block_alignment_type    logical_block_alignment_type_in,    //对齐控制 
                logical_block_alignment_control logical_block_alignment_control_in, //跨越边界控制
                use_control                     use_control_in,                     //超用控制
                mes::a_mes& mes_out
            );

            //缓存控制 跳过则不使用缓存
            void USING_CACHE_ALL(mes::a_mes& mes_out);  //启用完全缓存，无需设置缓存大小
            void USING_CACHE_PART(mes::a_mes& mes_out); //启用部分缓存
            void SET_CACHE_BLOCK_COUNT_LIMIT(           //如果启用部分缓存，则需设置缓存大小
                size_t cache_logical_block_count_max_in,
                mes::a_mes& mes_out
            );

            void USING_MEMORY_STACK_ACCELERATE(mes::a_mes& mes_out); //启用内存栈加速，加速块分配
            void SET_STACK_BLOCK_COUNT_LIMIT(                        //如果启用内存栈加速，则需设置栈大小
                size_t stack_accelerate_logical_block_count_max_in,
                mes::a_mes& mes_out
            );

            void USING_PAIR_ECC(mes::a_mes& mes_out); //启用pair纠错
            void SET_PAIR_COUNT(   //如果启用PAIR_ECC，则需设置pair数
                size_t pair_count_in,
                mes::a_mes& mes_out
            );

            void USING_ENCRYPTION(); //启用加密
            void SET_ENCRYPTION_DATA(//如果启用加密，则需要设置加密解密函数和密码
                void* encryption_function,
                void* decryption_function_in,
                str         key_in,
                mes::a_mes& mes_out
            );

            //打开引导
            void OPEN_VOLUME(
                str          logical_volume_file_path_in,
                mes::a_mes& mes_out
            );
            // 如果卷被加密过，OPEN_VOLUME的mes_out会返回服务错误，接下来需要
            // USING_ENCRYPTION
            // SET_ENCRYPTION_DATA
            // 然后会再次自动尝试打开卷

            void BOOT_END(mes::a_mes& mes_out);

            void RUN(mes::a_mes& mes_out);

        private:
            logical_volume* this_ptr = nullptr;
        };
        friend class BOOT;
        BOOT BOOT_ = BOOT(this);

        class SERVICE_CONTROL {
        public:
            SERVICE_CONTROL(logical_volume* this_ptr_in);

            void STOP_SERVICE(mes::a_mes& mes_out);

            void CHANGE_CACHE_BLOCK_COUNT_LIMIT(
                size_t cache_logical_block_count_max_in,
                mes::a_mes& mes_out
            );
            void USELESS_CACHE();

            void CHANGE_STACK_BLOCK_COUNT_LIMIT(
                size_t stack_accelerate_logical_block_count_max_in,
                mes::a_mes& mes_out
            );
            void USELESS_SATCK_ACCELERATE();

            void RUN_SERVICE(mes::a_mes& mes_out);
        private:
            logical_volume* this_ptr = nullptr;
        };
        friend class SERVICE_CONTROL;
        SERVICE_CONTROL SERVICE_CONTROL_ = SERVICE_CONTROL(this);

        class CONTROL_BOARD {
        public:
            CONTROL_BOARD(logical_volume* this_ptr_in);

        private:
            logical_volume* this_ptr = nullptr;
        };
        friend class CONTROL_BOARD;
        CONTROL_BOARD CONTROL_BOARD_ = CONTROL_BOARD(this);

        class DRIVE {
        public:
            DRIVE(logical_volume* this_ptr_in);

            class check {
            public:
                check() = default;

                check(logical_volume* this_ptr_in);

                void this_ptr_not_null();

                void init_value(mes::a_mes& mes_out);

                void strategy_matching(mes::a_mes& mes_out);

            private:
                logical_volume* this_ptr = nullptr;

            };
            
            class head_control {
            public:
                head_control() = default;
                head_control(logical_volume* this_ptr_in);

                void get(
                    size_t             load_begin_in,
                    bmms_f::agent_ptr& data_agent_ptr_out,
                    mes::a_mes& mes_out
                );

                void set(
                    size_t             store_begin_in,
                    bmms_f::agent_ptr& data_agent_ptr_in,
                    mes::a_mes& mes_out
                );

                void fix_by_pair(
                    mes::a_mes& mes_out
                );

                void fix_by_1byte_ECC(
                    mes::a_mes& mes_out
                );

                void chk_by_pair(
                    bool& chk_res_out,
                    mes::a_mes& mes_out
                );

                void init_pair(
                    mes::a_mes& mes_out
                );

                enum class op {
                    get,
                    set
                };

                void name(
                    op          op_in,
                    str& str_io,
                    mes::a_mes& mes_out
                );

                void head_check_sum_value(
                    op          op_in,
                    size_t& sum_value_io,
                    mes::a_mes& mes_out
                );

                void version(
                    op          op_in,
                    size_t& version_io,
                    mes::a_mes& mes_out
                );

                void init_type(
                    op                         op_in,
                    logical_volume::init_type& init_type_io,
                    mes::a_mes& mes_out
                );

                void use_control(
                    op                         op_in,
                    logical_volume::use_control& use_control_io,
                    mes::a_mes& mes_out
                );

                void physical_block_size(
                    op           op_in,
                    size_t& physical_block_size_io,
                    mes::a_mes& mes_out
                );

                void logical_block_size(
                    op           op_in,
                    size_t& logical_block_size_io,
                    mes::a_mes& mes_out
                );

                void logical_block_alignment_type(
                    op                                            op_in,
                    logical_volume::logical_block_alignment_type& logical_block_alignment_type_io,
                    mes::a_mes& mes_out
                );
                void logical_block_alignment_control(
                    op                                               op_in,
                    logical_volume::logical_block_alignment_control& logical_block_alignment_control_io,
                    mes::a_mes& mes_out
                );

                void logical_block_count_max(
                    op     op_in,
                    size_t& logical_block_count_max_io,
                    mes::a_mes& mes_out
                );

                void cache_control(
                    op                             op_in,
                    logical_volume::cache_control& cache_control_io,
                    mes::a_mes& mes_out
                );

                void memory_cache_control(
                    op                                    op_in,
                    logical_volume::memory_cache_control& memory_cache_control_io,
                    mes::a_mes& mes_out
                );

                void stack_accelerate(
                    op                                op_in,
                    logical_volume::stack_accelerate& stack_accelerate_io,
                    mes::a_mes& mes_out
                );

                void pair(
                    op op_in,
                    byte_1& pair_io,
                    mes::a_mes& mes_out
                );

                void permission(
                    op op_in,
                    logical_volume::permission& permission_io,
                    mes::a_mes& mes_out
                );

                void password_encryption_result(
                    op  op_in,
                    size_t& password_encryption_result_io,
                    mes::a_mes& mes_out
                );

                void created_time(
                    op  op_in,
                    byte_8& created_time_io,
                    mes::a_mes& mes_out
                );

                void last_change_time(
                    op  op_in,
                    byte_8& last_change_io,
                    mes::a_mes& mes_out
                );

                void init_head(
                    mes::a_mes& mes_out
                );

            private:
                logical_volume* this_ptr = nullptr;
            };
            
            class stack_control {
            public:
                stack_control() = default;

                stack_control(logical_volume* this_ptr_in);

                void init_stack_info();

                void get(
                    size_t             load_begin_in,
                    bmms_f::agent_ptr& data_agent_ptr_out,
                    mes::a_mes& mes_out
                );

                void set(
                    size_t             store_begin_in,
                    bmms_f::agent_ptr& data_agent_ptr_in,
                    mes::a_mes& mes_out
                );

                void fix_by_pair_ECC(
                    mes::a_mes& mes_out
                );

                //pair = 0时使用
                void fix_by_4bitECC(
                    mes::a_mes& mes_out
                );

                void chk_by_pair(
                    bool& chk_res_out,
                    mes::a_mes& mes_out
                );

                void init_stack_space(
                    mes::a_mes& mes_out
                );

                void get_group_info_for_pop(
                    size_t  index_in,
                    size_t& physical_block_offset_out,
                    size_t& handle_version_out,
                    size_t& handle_version_offset_out,
                    bool& can_pop_out,
                    size_t& can_pop_offset_out,
                    size_t& logical_block_begin_offset_out,
                    mes::a_mes& mes_out
                );
                void get_group_info_for_push_with_check(
                    size_t& index_in,
                    size_t& version_in,
                    size_t& physical_block_offset_out,
                    size_t& handle_version_out,
                    size_t& handle_version_offset_out,
                    bool&   can_pop_out,
                    size_t& can_pop_offset_out,
                    size_t& logical_block_begin_offset_out,
                    mes::a_mes& mes_out
                );
                void mark_block_can_pop(
                    size_t& can_pop_offset_in,
                    mes::a_mes& mes_out
                );
                void mark_block_can_not_pop(
                    size_t& can_pop_offset_in,
                    mes::a_mes& mes_out
                );
                void increment_version(
                    size_t& handle_version_offset_in,
                    mes::a_mes& mes_out
                );
                void CAS_stack_offset_atomic_for_push(
                    size_t&     index_in,
                    mes::a_mes& mes_out
                );
                void CAS_stack_offset_atomic_for_pop(
                    size_t&     index_out,
                    mes::a_mes& mes_out
                );
                void push(
                    size_t&      logical_block_index_in,
                    size_t&      version_in,
                    mes::a_mes& mes_out
                );

                void pop(
                    size_t&     logical_block_index_out,
                    size_t&     version_out,
                    mes::a_mes& mes_out
                );

                //DOTO
                /*
                void disk_clean_all(
                    mes::a_mes& mes_out
                );

                //整理逻辑块使其有序，代理指针是代理的是{{index,version}，，，}
                void organize_range(
                    bmms_f::agent_ptr& index_and_version_group_io,
                    mes::a_mes& mes_out
                );

                //part_0 block count|part_1 block count|...|part_n block count|0xFF00000000000000|groups_1|...|groups_n|
                //因为index最大是0x00FFFFFFFFFFFFFF，所以分隔符0xFF00000000000000不会误读,且便于扩展（低7字节可扩展）
                //并且内部会自验证输入是否合法
                void full_organize(
                    bmms_f::agent_ptr& parts_io, 
                    mes::a_mes& mes_out
                );
                */
                size_t get_using_count(mes::a_mes& mes_out);

                size_t get_free_count(mes::a_mes& mes_out);


            private:
                logical_volume* this_ptr = nullptr;
            };

            

            check        check_;
            head_control head_control_;

            logical_volume* this_ptr = nullptr;
        };
        friend class DRIVE;
        friend class DRIVE::check;
        friend class DRIVE::head_control;
        DRIVE DRIVE_ = DRIVE(this);
        
        // 数据存储
        struct data {
            
            bool                            is_init_
                = false;

            //============== 内存部分 ==============

            init_type                       init_type_
                = init_type::no_init;

            use_control                     use_control_
                = use_control::no_init;

            size_t                          physical_block_size_
                = 0;

            size_t                          logical_block_size_
                = 0;

            logical_block_alignment_type    logical_block_alignment_type_
                = logical_block_alignment_type::no_init;

            logical_block_alignment_control logical_block_alignment_control_
                = logical_block_alignment_control::no_init;

            size_t                          logical_block_count_max_
                = 0;

            //============== 文件部分 ==============

            file_control                    file_control_
                = file_control::no_init;
            load_control                    load_control_
                = load_control::no_init;
            cache_control                   cache_control_
                = cache_control::no_init;
            memory_cache_control            memory_cache_control_
                = memory_cache_control::no_init;
            size_t                          cache_logical_block_count_max_
                = 0;
            stack_accelerate                stack_accelerate_
                = stack_accelerate::no_init;
            size_t                          stack_accelerate_size_
                = 0;
            byte_1                          pair_
                = 0;
            permission                      permission_
                = permission::no_init;
            str                             key_
                = "";
            size_t                          key_encryption_result_
                = 0;
            void* encryption_function_
                = nullptr;
            void* decryption_function_
                = nullptr;

            struct head_offsets {
                size_t name = 0;                       // 0
                size_t checksum = 72;                  // 72
                size_t version = 144;                  // 144
                size_t init_type = 216;                // 216
                size_t use_control = 232;              // 232
                size_t physical_block_size = 248;      // 248
                size_t logical_block_size = 320;       // 320
                size_t alignment_type = 392;           // 392
                size_t alignment_control = 408;        // 408
                size_t logical_block_count_max = 424;  // 424
                size_t cache_control = 488;            // 488
                size_t memory_cache_control = 512;     // 512 
                size_t stack_accelerate = 528;         // 528
                size_t pair = 544;                     // 544
                size_t permission = 560;               // 560 
                size_t password_encryption = 576;      // 576
                size_t created_time = 648;             // 648 
                size_t last_change_time = 720;         // 720
            };
            head_offsets head_off_;

            struct stack_info {
                size_t logical_block_stack_top_offset_0_offset    = 0;
                size_t logical_block_stack_top_offset_1_offset    = 0;
                size_t logical_block_stack_top_offset_flag_offset = 0;
                size_t logical_block_stack_offset                 = 0;
                size_t logical_block_stack_size                   = 0;
                size_t _4K_alignment_fill_offset                  = 0;
                size_t _4K_alignment_fill_size                    = 0;
                size_t physical_blocks_offset                     = 0;
                size_t physical_blocks_size                       = 0;
                size_t actually_logical_block_size_               = 0;
                bool   using_range       = false;
                bool   using_group       = false;
                size_t stack_group_count = 0;
                size_t stack_group_size  = 0;
                
            };
            stack_info stack_info_;

            struct memory_stack_accelerate {
                unique_ptr<byte_8> stack_info = nullptr;
                byte_8* stack_top_ptr         = nullptr;
                size_t  accelerate_offset_begin        = 0;
                size_t  accelerate_logical_block_count = 0;
            };
            
            // 数据控制区
            struct static_cache {
                enum class result {
                    HIT,    // 数据在缓存中，成功读取
                    MISS,   // 数据不在缓存中（空洞）
                    ERROR   // 其他错误
                };

                static_cache(
                    logical_volume* logical_volume_ptr_in,
                    mes::a_mes& mes_out
                );

                ~static_cache();
                //建议先disable_cache再析构,
                //否则会数据丢失，这是为了状态可知，避免静默文件损坏不可知

                void enable_cache(
                    mes::a_mes& mes_out
                );

                void disable_cache(
                    bool flush_in,
                    mes::a_mes& mes_out
                );

                void resize_cache(
                    bool save_dirty,
                    mes::a_mes& mes_out
                );

                void logout_blocks_cache(
                    bmms_f::agent_ptr& free_index_and_versions_in,// 以{size_t index,size_t version}连续排列
                    mes::a_mes& mes_out
                );
                //编码错误：未初始化缓存，free_index_and_version_in为空，
                // 输入数据长度不是16字节的倍数，批次中存在索引越界，批次中存在版本号不匹配，
                // 批次中存在重复释放；服务错误：批次中某个块写回脏页失败，批次中某个块从映射表移除失败

                void get_cache_stats(
                    size_t& total_blocks_out,
                    size_t& used_blocks_out,
                    size_t& dirty_blocks_out,
                    size_t& locked_blocks_out,
                    double& hit_rate_out,
                    mes::a_mes& mes_out
                );

                void flush_dirty_blocks(
                    bool wait_completion,
                    mes::a_mes& mes_out
                );

                void get_priority_info(
                    size_t      check_row_in,
                    size_t      check_row_size_out,
                    size_t      check_row_using_out,
                    size_t      check_row_free_out,
                    mes::a_mes& mes_out
                );

                //priority在缓存所有策略时忽视，部分缓存时强制填入
                void register_block(
                    size_t      index_in,
                    size_t      version_in,
                    size_t      priority_row_in,
                    size_t&     priority_column_out,
                    mes::a_mes& mes_out
                );

                void logout_block(
                    size_t      index_in,
                    size_t      version_in,
                    size_t      priority_row_in,
                    size_t&     priority_column_in,
                    mes::a_mes& mes_out
                );

                void logout_blocks_cache(//用于part析构时的一次性释放
                    bmms_f::agent_ptr& free_index_and_versions_in,// 以{size_t index,size_t version}连续排列
                    bool               flush_in,
                    mes::a_mes&        mes_out
                );

                void get_block_registed(
                    size_t      index_in,
                    size_t      version_in,
                    bool        block_registed_out,
                    mes::a_mes& mes_out
                );

                void set_block_priority(
                    size_t      index_in,
                    size_t      version_in,
                    size_t      old_priority_row_in,
                    size_t      old_priority_column_in,
                    size_t      new_priority_row_in,
                    size_t&     new_priority_column_out,
                    mes::a_mes& mes_out
                );

                void data_exchange(
                    size_t      load_index_in,
                    size_t      load_version_in,
                    size_t      load_begin_in,
                    size_t      load_size_in,
                    size_t      store_index_in,
                    size_t      store_version_in,
                    size_t      store_begin_in,
                    bool        safe,
                    bool        flush_instantly,
                    result&     result_out,//缓存未命中，不操作
                    mes::a_mes& mes_out
                );

                void load_to_agent_ptr(
                    size_t             load_index_in,
                    size_t             load_version_in,
                    size_t             load_begin_in,
                    size_t             load_size_in,
                    bmms_f::agent_ptr& store_agent_ptr_in,
                    size_t             store_begin_in,
                    bool               safe,
                    result&            result_out,
                    mes::a_mes&        mes_out
                );

                void store_from_agent_ptr(
                    bmms_f::agent_ptr& load_agent_ptr_in,
                    size_t             load_begin_in,
                    size_t             load_size_in,
                    size_t             store_index_in,
                    size_t             store_version_in,
                    size_t             store_begin_in,
                    bool               safe,
                    bool               flush_instantly,
                    result&            result_out,
                    mes::a_mes&        mes_out
                );

                //锁定缓存防止缓存驱逐
                void try_lock_cache(
                    size_t      lock_index_in,
                    size_t      lock_version_in,
                    mes::a_mes& mes_out
                );

                //零拷贝访问缓存
                void try_get_cache_agent_ptr(
                    size_t           get_index_in,
                    size_t           get_version_in,
                    cache_agent_ptr& cache_agent_ptr_out,
                    mes::a_mes&      mes_out
                );

                //解锁缓存
                void try_unlock_cache(
                    size_t unlock_index_in,
                    size_t unlock_version_in
                );

                struct block_cache_info_vector {
                    enum class op {
                        get,
                        set
                    };

                    // 4bit 布局
                    static constexpr uint8_t LOCK_BIT  = 0b0001;  // 位0：锁定标记
                    static constexpr uint8_t DIRTY_BIT = 0b0010;  // 位1：脏标记
                    static constexpr uint8_t VALID_BIT = 0b0100;  // 位2：数据有效
                    static constexpr uint8_t RESERVED  = 0b1000;  // 位3：预留

                    unique_ptr<byte_1[]> data = nullptr;  // 位图存储
                    size_t entry_count = 0;               // 条目数量（逻辑块总数）

                    // 初始化
                    void init(size_t count, mes::a_mes& mes_out);

                    // 辅助：获取4bit在字节中的位置
                    void get_position(size_t index, size_t& byte_idx, size_t& nibble_offset);

                    // 读取4bit值
                    uint8_t read_nibble(size_t index);

                    // 写入4bit值
                    void write_nibble(size_t index, uint8_t value);

                    // 锁定操作
                    void lock(op op_in, size_t index, bool& lock_io);

                    // 脏标记操作
                    void dirty(op op_in, size_t index, bool& dirty_io);

                    // 有效标记操作
                    void valid(op op_in, size_t index, bool& valid_io);

                    // 预留位操作（供未来扩展）
                    void reserved(op op_in, size_t index, bool& reserved_io);

                    // 批量获取状态
                    void get_all(size_t index, bool& locked, bool& dirty, bool& valid, bool& reserved);

                    // 批量设置状态
                    void set_all(size_t index, bool locked, bool dirty, bool valid, bool reserved);

                    // 清空单个条目
                    void clear(size_t index);

                    // 清空所有条目
                    void clear_all();

                    // 获取统计信息
                    void get_stats(size_t& used_count, size_t& dirty_count, size_t& locked_count);
                };
                
                struct priority_matrix {
                    enum class op {
                        set,
                        clean
                    };

                    // 核心参数
                    size_t row_count         = 0;   // 行数（优先级级别数）
                    size_t column_count      = 0;   // 列数（每行的容量）
                    size_t permanent_row_end = 0;   // 常驻行结束索引（0表示第0行常驻，1表示0-1行常驻，SIZE_MAX表示全部常驻）

                    // 位图存储
                    unique_ptr<byte_1[]> priority_matrix_info = nullptr;  // 位图，每个bit表示一个位置是否被占用

                    // 辅助：计算位图大小（字节数）
                    size_t bitmap_size() const;

                    // 辅助：计算位图中的位置
                    void get_bit_position(size_t row, size_t col, size_t& byte_idx, size_t& bit_idx) const;

                    // 辅助：读写位
                    bool test_bit(size_t byte_idx, size_t bit_idx) const;

                    void set_bit(size_t byte_idx, size_t bit_idx, bool value);

                    // 标记普通优先级项
                    void mark_normal_priority(
                        op          op_in,
                        size_t      row_in,
                        size_t&     column_io,  // set时输出，clean时输入
                        mes::a_mes  mes_out
                    );

                    // 从末尾释放普通优先级项（用于淘汰）
                    void free_normal_priority_from_end(
                        size_t      free_count,
                        mes::a_mes  mes_out
                    );

                    // 获取是否有普通项被释放（用于唤醒等待）
                    void get_normal_priority_freed(bool& freed_out);

                    // 标记常驻优先级项
                    void mark_permanent_priority(
                        op          op_in,
                        size_t& row_io,     // set时输出，clean时输入
                        size_t& column_io,  // set时输出，clean时输入
                        mes::a_mes  mes_out
                    );

                    // 获取行优先级使用信息
                    void get_row_priority_using_info(
                        size_t      row_in,
                        size_t& using_count_out,
                        size_t& free_count_out,
                        mes::a_mes  mes_out
                    );

                    // 获取常驻项使用信息
                    void get_permanent_using_info(
                        size_t& using_count_out,
                        size_t& free_count_out,
                        mes::a_mes  mes_out
                    );

                    // 获取项的优先级（行号越低优先级越高）
                    size_t get_priority(size_t row, size_t col) const;

                    // 判断项是否常驻
                    bool is_permanent(size_t row, size_t col) const;

                    // 初始化矩阵
                    void init(size_t rows, size_t cols, size_t permanent_rows, mes::a_mes& mes_out);
                }; 

                //存 {byte_8 version,block_data} 
                class static_cache_part : public bmms_f::static_volume_part {
                public:
                    // 继承构造函数
                    using static_volume_part::static_volume_part;

                    // 重写 load 方法 - 不会自动分配，未命中返回 MISS
                    enum class result {
                        HIT,    // 数据在缓存中，成功读取
                        MISS,   // 数据不在缓存中（空洞）
                        ERROR   // 其他错误
                    };

                    result load_with_status(
                        size_t           load_begin_in,
                        size_t           load_size_in,
                        bmms_f::agent_ptr& save_agent_ptr_in,
                        size_t           store_begin_in,
                        bool             safe,
                        mes::a_mes& mes_out
                    );

                    // 重写 store 方法 - 不会自动分配，未命中返回 MISS
                    result store_with_status(
                        bmms_f::agent_ptr& load_agent_ptr_in,
                        size_t             load_begin_in,
                        size_t             load_size_in,
                        size_t             store_begin_in,
                        bool               safe,
                        mes::a_mes& mes_out
                    );

                    // 为了方便，也提供不返回状态的版本（但会保持原语义）
                    using static_volume_part::load;
                    using static_volume_part::store;
                    using static_volume_part::load_to_void_ptr;
                    using static_volume_part::store_from_void_ptr;
                };

                block_cache_info_vector           block_cache_info_vector_;
                priority_matrix                   priority_matrix_;
                unique_ptr<bmms_f::static_volume> volume_ptr_                = nullptr;
                unique_ptr<static_cache_part>     cahce_block_part_ptr_      = nullptr;
                logical_volume*                   logical_volume_ptr         = nullptr;
                size_t                            cache_using_count          = 0;
                bool                              is_init                    = false;
            };

            struct accelerate_stack {
                //尽力补充满
                accelerate_stack(
                    logical_volume* logical_volume_ptr_in,
                    mes::a_mes& mes_out
                ); 
                ~accelerate_stack();

                bool get_is_enable();

                //尽力补充满
                void complement_block(
                    mes::a_mes& mes_out
                );

                void enable(
                    mes::a_mes& mes_out
                );

                void disable(
                    mes::a_mes& mes_out
                );

                void reszie(mes::a_mes& mes_out);

                void new_block(
                    size_t& index_out,
                    size_t& version_out,
                    mes::a_mes& mes_out
                );

                void delete_block(
                    size_t& index_in,
                    size_t& version_in,
                    mes::a_mes& mes_out
                );

                void get_info(
                    size_t& total_blocks_out,
                    size_t& used_blocks_out,
                    size_t& free_blocks_out,
                    mes::a_mes& mes_out
                );

                logical_volume*      logical_volume_ptr = nullptr;
                unique_ptr<byte_8[]> accelerate_stack_ram_space = nullptr; //存{index,version}
                size_t               accelerate_stack_size = 0;
                byte_8               accelerate_stack_top_index = 0;
                bool                 is_init = false;
            };
            unique_ptr<accelerate_stack>          accelerate_stack_ = nullptr;
            struct dynamic_cache {
                unique_ptr<bmms_f::free_volume>   dynamic_cache_volume_ptr_ = nullptr;
            };
            unique_ptr<static_cache>              static_cache_     = nullptr;
            unique_ptr<bfs_f::bin_file>           bin_file_ptr_     = nullptr;
            
        };
        data data_;
	};

    class handle {
    public:
        //======================================== 构造与析构 ========================================
        handle() = default;

        handle(
            shared_ptr<logical_volume> logical_volume_ptr_in,
            mes::a_mes& mes_out
        );

        ~handle();

        //======================================== 访问控制 ========================================

        //禁止复制
        handle(const handle&) = delete;
        handle& operator=(const handle&) = delete;

        //允许移动
        handle(handle&& other)           noexcept;
        handle& operator=(handle&& other)noexcept;

        //======================================== 数据交换 ========================================

        // 从句柄读取数据到自身
        void load(
            size_t      load_begin_in,
            size_t      load_size_in,
            handle& save_handle_in,
            size_t      store_begin_in,
            bool        safe,
            bool        flush_instantly,
            mes::a_mes& mes_out
        );

        // 从自身数据写入句柄
        void store(
            handle& load_handle_in,
            size_t      load_begin_in,
            size_t      load_size_in,
            size_t      store_begin_in,
            bool        safe,
            bool        flush_instantly,
            mes::a_mes& mes_out
        );

        // 将自身数据读取到代理指针
        void load_to_agent_ptr(
            size_t             load_begin_in,
            size_t             load_size_in,
            bmms_f::agent_ptr& save_agent_ptr_in,
            size_t             store_begin_in,
            bool               safe,
            mes::a_mes& mes_out
        );

        // 从代理指针读取数据写入到自身
        void store_from_agent_ptr(
            bmms_f::agent_ptr& load_agent_ptr_in,
            size_t           load_begin_in,
            size_t           load_size_in,
            size_t           store_begin_in,
            bool             safe,
            bool             flush_instantly,
            mes::a_mes& mes_out
        );

        //======================================== 零拷贝访问 ========================================

        //锁定缓存防止缓存驱逐
        void try_lock_cache(mes::a_mes& mes_out);
        //零拷贝访问缓存
        void try_get_cache_agent_ptr(
            cache_agent_ptr& cache_agent_ptr_out,
            mes::a_mes&      mes_out
        );
        //解锁缓存
        void try_unlock_cache();

        //======================================== 校验接口 ========================================

        bool get_is_init();

    private:
        size_t                     block_version      = 0;
        size_t                     index              = 0;
        size_t                     priority_row       = 0xFFFFFFFFFFFFFFFF;
        size_t                     priority_column    = 0xFFFFFFFFFFFFFFFF;
        shared_ptr<logical_volume> logical_volume_ptr = nullptr;
    };

    //缓存行RAII锁,析构后缓存代理指针自动置空，自动拒绝访问
    class cache_RAII_lock {
    public:

        //禁止复制
        cache_RAII_lock(const cache_RAII_lock&) = delete;
        cache_RAII_lock& operator=(const cache_RAII_lock&) = delete;

        //禁止移动
        cache_RAII_lock(cache_RAII_lock&& other) = delete;
        cache_RAII_lock& operator=(cache_RAII_lock&& other) = delete;
        
        //禁止空构造
        cache_RAII_lock() = delete;

        cache_RAII_lock(
            shared_ptr<handle>& handle_ptr_in,
            cache_agent_ptr&    cache_agent_ptr_out,
            mes::a_mes&         mes_out
        );

        ~cache_RAII_lock();

    private:
        cache_agent_ptr*   cache_agent_ptr_ptr = nullptr;
        shared_ptr<handle> handle_ptr          = nullptr;
    };

	class logical_volume_ptr {

	};

	class logical_part {

	};
    
};// namespace lfms_f