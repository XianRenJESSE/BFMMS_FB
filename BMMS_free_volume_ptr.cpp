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
    free_volume_ptr::free_volume_ptr(free_volume_ptr&& other) noexcept
        : is_init(other.is_init)
        , block_index(other.block_index)
        , agent_ptr(move(other.agent_ptr))
        , free_volume_shared_ptr(move(other.free_volume_shared_ptr))
    {
        // 重置原对象状态
        other.is_init = false;
        other.block_index = 0;
        other.agent_ptr = bmms_f::agent_ptr();
        other.free_volume_shared_ptr = nullptr;
    }

    // 移动赋值运算符
    free_volume_ptr& free_volume_ptr::operator=(free_volume_ptr&& other) noexcept
    {
        if (this != &other) {
            // 1. 先释放当前资源（析构时会自动调用 delete_block）
            //    但这里不能直接调用析构，需要主动释放
            if (is_init) {
                free_volume_shared_ptr->delete_block(agent_ptr, block_index);
            }

            // 2. 移动所有成员
            is_init = other.is_init;
            block_index = other.block_index;
            agent_ptr = move(other.agent_ptr);
            free_volume_shared_ptr = move(other.free_volume_shared_ptr);

            // 3. 重置原对象
            other.is_init = false;
            other.block_index = 0;
            other.agent_ptr = bmms_f::agent_ptr();
            other.free_volume_shared_ptr = nullptr;
        }
        return *this;
    }

    //======================================== 构造与析构 ========================================

    // 参数构造函数
    free_volume_ptr::free_volume_ptr(
        shared_ptr<free_volume>& free_volume_shared_ptr_in,
        mes::a_mes& mes_out
    ) {
        // 1. 校验卷有效
        free_volume_shared_ptr = free_volume_shared_ptr_in;
        check_free_volume_value_valid();

        // 2. 重置消息
        mes_out = mes::a_mes();

        // 3. 申请内存（注意：free_volume::new_block 需要 index_out 参数）
        free_volume_shared_ptr_in->new_block(
            agent_ptr,
            block_index,    // 输出索引！
            mes_out
        );

        // 4. 校验申请内存是否成功
        if (mes_out.code != 0) {
            mes_out = bmms_m::err::service_err_free_volume_ptr_new_block_failed();
            return;
        }

        // 5. 成功
        is_init = true;
    };

    // 析构函数
    free_volume_ptr::~free_volume_ptr() {
        if (is_init) {
            free_volume_shared_ptr->delete_block(agent_ptr, block_index);
        }
    };

    //======================================== 数据控制 ========================================

    void free_volume_ptr::clear() {
        check_is_init();
        agent_ptr.clear();
    };

    void free_volume_ptr::clear_range(
        size_t begin_in,
        size_t size_in
    ) {
        check_is_init();
        agent_ptr.clear_range(begin_in, size_in);
    };

    //======================================== 数据交换 ========================================

    // 将自身数据读取到代理指针
    void free_volume_ptr::load(
        size_t load_begin_in,
        size_t load_size_in,
        bmms_f::agent_ptr& save_agent_ptr_in,
        size_t     store_begin_in,
        bool       safe
    ) {
        check_is_init();
        agent_ptr.load(
            load_begin_in,
            load_size_in,
            save_agent_ptr_in,
            store_begin_in,
            safe
        );
    };

    // 从代理指针读取数据写入到自身
    void free_volume_ptr::store(
        bmms_f::agent_ptr& load_agent_ptr_in,
        size_t     load_begin_in,
        size_t     load_size_in,
        size_t     store_begin_in,
        bool       safe
    ) {
        check_is_init();
        agent_ptr.store(
            load_agent_ptr_in,
            load_begin_in,
            load_size_in,
            store_begin_in,
            safe
        );
    };

    // 将自身数据读取到void指针
    void free_volume_ptr::load_to_void_ptr(
        size_t load_begin_in,
        size_t load_size_in,
        void* store_ptr_in,
        bool   safe
    ) {
        check_is_init();
        agent_ptr.load_to_void_ptr(
            load_begin_in,
            load_size_in,
            store_ptr_in,
            safe
        );
    };

    // 从void指针读数据写入自身
    void free_volume_ptr::store_from_void_ptr(
        void* load_ptr_in,
        size_t load_size_in,
        size_t store_begin_in,
        bool   safe
    ) {
        check_is_init();
        agent_ptr.store_from_void_ptr(
            load_ptr_in,
            load_size_in,
            store_begin_in,
            safe
        );
    };

    //======================================== 索引获取 ========================================

    size_t free_volume_ptr::get_index() const {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_ptr_not_inited);
        }
        return block_index;
    };

    bmms_f::agent_ptr& free_volume_ptr::get_agent_ptr() {
        check_is_init();
        return agent_ptr;
    };

    //======================================== 校验 ========================================

    void free_volume_ptr::check_free_volume_value_valid() {
        if (free_volume_shared_ptr == nullptr) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_ptr_ctor_nullptr);
        }
        if (free_volume_shared_ptr->get_is_init() == false) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_ptr_ctor_not_inited);
        }
    };

    void free_volume_ptr::check_is_init() {
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_free_volume_ptr_not_inited);
        }
    };

};