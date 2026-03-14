// file_agent_ptr.cpp
module BFS;

#include "BFS_dp.h"

namespace bfs_f {

    #define EXIT_FAILURE 1

    #if defined(BFS_INCLUDE_TEST)
    #define __CODING_ERROR__(mes)\
            mes().out(); \
            throw mes();
    #else
    #define __CODING_ERROR__(mes)\
             mes().out();std::exit(EXIT_FAILURE);
    #endif

    //============================ 构造 ============================

    // 从bin_file 构造函数
    file_agent_ptr::file_agent_ptr(
        const shared_ptr<bfs_f::bin_file>& bin_file_in,
        size_t           begin_in,
        size_t           size_in,
        bmms_f::agent_ptr& cache_agent_ptr_in,
        mes::a_mes& mes_out
    ) : inited(false), dirty(false), bin_file_shared_ptr(nullptr) {
        mes_out = mes::a_mes(0, 0, "");

        try {
            // 0. 需要先检查 bin_file_in 是否为空
            if (!bin_file_in) {
                __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_bin_file_shared_ptr_is_nullptr_when_construct)
            }

            // 1. 文件必须已打开
            if (!bin_file_in->__check_inited__()) {
                mes_out = bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_construct();
                return;
            }

            // 2. 文件范围有效性检查
            if (!bin_file_in->__check_file_range__(begin_in, size_in)) {
                mes_out = bfs_m::err::service_error_file_agent_ptr_file_range_outside_file_when_construct();
                return;
            }

            // 3. 范围大小必须为正
            if (size_in == 0) {
                __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_zero_size_when_construct)
            }

            // 4. 缓存代理指针非空检查
            bool cache_is_null = false;
            cache_agent_ptr_in.get_is_null(cache_is_null);
            if (cache_is_null) {
                mes_out = bfs_m::err::service_error_file_agent_ptr_cache_agent_ptr_is_null_when_construct();
                return;
            }

            // 5. 缓存容量足够检查
            size_t cache_size = 0;
            cache_agent_ptr_in.get_size(cache_size);
            if (cache_size < size_in) {
                mes_out = bfs_m::err::service_error_file_agent_ptr_cache_agent_ptr_size_too_small_when_construct();
                return;
            }

            // 赋值可能抛出异常的操作
            cache_ptr = cache_agent_ptr_in;
            cache_begin_about_bin_file = begin_in;
            bin_file_shared_ptr = bin_file_in;

            // 所有检查通过，标记为已初始化
            inited = true;
            dirty = false;

            mes_out = mes::a_mes(0, 0, "");
        }
        catch (const std::bad_alloc) {
            __cleanup_on_construct_failure__();
            mes_out = bfs_m::err::service_error_file_agent_ptr_memory_allocation_failed_when_construct();
        }
        catch (const std::exception) {
            __cleanup_on_construct_failure__();
            mes_out = bfs_m::err::service_error_file_agent_ptr_unknown_error_when_construct();
        }
        catch (mes::a_mes) {
            throw;
        }
        catch (...) {
            __cleanup_on_construct_failure__();
            mes_out = bfs_m::err::service_error_file_agent_ptr_unknown_error_when_construct();
        }
    }

    // 析构函数
    file_agent_ptr::~file_agent_ptr() {
        // 简单直接：如果脏就尝试写回，不关心结果
        if (inited && dirty && bin_file_shared_ptr != nullptr) {
            try {
                // 临时变量接收错误，但不处理
                mes::a_mes push_mes;

                // 直接调用，不需要备份或恢复状态
                push_cache_to_bin_file(push_mes);

                // 不管成功失败，都不需要恢复状态
                // 对象即将被销毁
            }
            catch (...) {

            }
        }
    }

    // 移动构造
    file_agent_ptr::file_agent_ptr(file_agent_ptr&& other) noexcept
        : cache_ptr(move(other.cache_ptr)),
        bin_file_shared_ptr(move(other.bin_file_shared_ptr)),
        cache_begin_about_bin_file(other.cache_begin_about_bin_file),
        inited(other.inited),
        dirty(other.dirty) {
        // 转移后，源对象处于有效但为空的状态
        other.inited = false;
        other.dirty = false;
        other.cache_begin_about_bin_file = 0;
    }

    //移动赋值
    file_agent_ptr& file_agent_ptr::operator=(file_agent_ptr&& other) noexcept {
        if (this != &other) {
            // 直接清理当前资源（如果脏则写回）
            if (inited && dirty) {
                mes::a_mes push_mes;
                push_cache_to_bin_file(push_mes);
            }

            // 转移资源
            cache_ptr = move(other.cache_ptr);
            bin_file_shared_ptr = move(other.bin_file_shared_ptr);
            cache_begin_about_bin_file = other.cache_begin_about_bin_file;
            inited = other.inited;
            dirty = other.dirty;

            // 清空源对象
            other.inited = false;
            other.dirty = false;
            other.cache_begin_about_bin_file = 0;
        }
        return *this;
    }

    //============================ 数据控制 ============================

    // 清空数据（清空整个缓存）
    void file_agent_ptr::clear(mes::a_mes& mes_out) {
        mes_out = mes::a_mes(0, 0, "");

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        try {
            // 2. 清空缓存
            cache_ptr.clear();

            // 3. 标记为脏（缓存被修改，与文件不一致）
            __mark_dirty__();

            // 成功
            mes_out = mes::a_mes(0, 0, "");
        }
        catch (const std::exception) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_unknown_error_when_clear();
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_unknown_error_when_clear();
        }
    }

    // 范围清空数据
    void file_agent_ptr::clear_range(
        size_t begin_in,
        size_t size_in,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        // 1. 检查自身状态和范围
        if (!__check_cache_range_valid__(begin_in, size_in, mes_out)) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overflow_when_clear_range)
        }

        try {
            // 2. 清空指定范围
            cache_ptr.clear_range(begin_in, size_in);

            // 3. 标记为脏
            __mark_dirty__();

            // 成功
            mes_out = mes::a_mes(0, 0, "");
        }
        catch (const std::exception) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_unknown_error_when_clear_range();
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_unknown_error_when_clear_range();
        }
    }

    // 写回缓存数据到文件
    void file_agent_ptr::push_cache_to_bin_file(mes::a_mes& mes_out) {
        mes_out = mes::a_mes(0, 0, "");
        // 先备份脏标志
        bool was_dirty = dirty;

        // 1. 检查自身状态
        if (!inited) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_not_inited_when_push();
            return;
        }

        // 2. 检查关联的文件是否仍然有效
        if (bin_file_shared_ptr == nullptr || !bin_file_shared_ptr->__check_inited__()) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_push();
            return;
        }

        // 3. 检查缓存代理指针有效性
        bool cache_is_null = false;
        cache_ptr.get_is_null(cache_is_null);
        if (cache_is_null) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_cache_agent_ptr_is_null_when_push)
        }

        // 4. 检查文件范围是否仍然有效
        size_t cache_size = 0;
        cache_ptr.get_size(cache_size);

        size_t file_size = bin_file_shared_ptr->__file_size__();
        if (cache_begin_about_bin_file + cache_size > file_size) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_range_outside_file_when_push();
            return;
        }

        // 5. 检查脏标志
        if (!dirty) {
            // 未脏，直接返回成功
            mes_out = mes::a_mes(0, 0, "");
            return;
        }

        // 6. 执行写回操作
        try {
            
            // 获取缓存原始指针
            void* cache_raw_ptr = nullptr;
            cache_ptr.get_void_ptr(cache_raw_ptr);
            if (cache_raw_ptr == nullptr) {
                __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_cache_agent_ptr_is_null_when_push);
            }

            // 使用 bin_file 的 store_from_void_ptr 方法
            bin_file_shared_ptr->store_from_void_ptr(
                cache_raw_ptr,
                cache_size,
                cache_begin_about_bin_file,
                true,  // safe模式
                mes_out
            );

            if (mes_out.code == 0) {
                // 写回成功，清除脏标志
                dirty = false;
            }
            else {
                // 写回失败，恢复脏标志
                dirty = was_dirty;
            }
        }
        catch (mes::a_mes) {
            throw;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_unknown_error_when_push();
            // 写回失败，恢复脏标志
            dirty = was_dirty;
        }
    }

    // 从文件读取数据到缓存
    void file_agent_ptr::pull_cache_from_bin_file(mes::a_mes& mes_out) {
        mes_out = mes::a_mes(0, 0, "");

        // 备份原始脏标志状态
        bool original_dirty = dirty;

        // 1. 检查自身状态
        if (!inited) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_not_inited_when_pull();
            // 保持原始脏标志状态
            dirty = original_dirty;
            return;
        }

        // 2. 检查关联的文件是否仍然有效
        if (bin_file_shared_ptr == nullptr) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_pull();
            // 保持原始脏标志状态
            dirty = original_dirty;
            return;
        }

        if (!bin_file_shared_ptr->__check_inited__()) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_pull();
            // 保持原始脏标志状态
            dirty = original_dirty;
            return;
        }

        // 3. 检查缓存代理指针有效性
        bool cache_is_null = false;
        cache_ptr.get_is_null(cache_is_null);
        if (cache_is_null) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_cache_agent_ptr_is_null_when_pull();
            return;
        }

        // 4. 获取缓存大小
        size_t cache_size = 0;
        cache_ptr.get_size(cache_size);
        if (cache_size == 0) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_cache_agent_ptr_size_zero_when_pull)
        }

        // 5. 检查文件范围是否仍然有效
        size_t file_size = 0;
        try {
            file_size = bin_file_shared_ptr->__file_size__();
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_unknown_error_when_pull();
            // 保持原始脏标志状态
            dirty = original_dirty;
            return;
        }

        // 检查是否溢出
        if (cache_begin_about_bin_file > SIZE_MAX - cache_size) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overflow_when_pull)
        }

        if (cache_begin_about_bin_file + cache_size > file_size) {
            // 文件被缩小了，需要调整或报告错误
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_range_outside_file_when_pull();
            // 读取失败，但保持原始脏标志状态
            dirty = original_dirty;
            return;
        }

        // 6. 执行读取操作
        try {
            // 获取缓存原始指针
            void* cache_raw_ptr = nullptr;
            cache_ptr.get_void_ptr(cache_raw_ptr);
            if (cache_raw_ptr == nullptr) {
                __CODING_ERROR__(bfs_m::err::service_error_file_agent_ptr_cache_agent_ptr_is_null_when_pull)
                // 读取失败，恢复原始脏标志状态
                dirty = original_dirty;
                return;
            }

            // 使用 bin_file 的 load_to_void_ptr 方法
            bin_file_shared_ptr->load_to_void_ptr(
                cache_begin_about_bin_file,
                cache_size,
                cache_raw_ptr,
                true,  // safe模式
                mes_out
            );

            if (mes_out.code == 0) {
                // 读取成功，清除脏标志（缓存与文件一致）
                dirty = false;
            }
            else {
                // 读取失败，恢复原始脏标志状态
                dirty = original_dirty;
                // mes_out 已经包含错误信息
            }
        }
        catch (const std::bad_alloc) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_memory_allocation_failed_when_pull();
            // 恢复原始脏标志状态
            dirty = original_dirty;
        }
        catch (const std::ios_base::failure) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_io_error_when_pull();
            // 恢复原始脏标志状态
            dirty = original_dirty;
        }
        catch (const std::exception) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_unknown_error_when_pull();
            // 恢复原始脏标志状态
            dirty = original_dirty;
        }
        catch (const mes::a_mes) {
            throw;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_unknown_error_when_pull();
            // 恢复原始脏标志状态
            dirty = original_dirty;
        }
    }

    //============================ 数据交换 ============================

    // 将自身数据读取到file_agent_ptr
    void file_agent_ptr::load(
        size_t          load_begin_in,
        size_t          load_size_in,
        file_agent_ptr& save_file_agent_ptr_in,
        size_t          store_begin_in,
        bool            safe,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 检查对方状态
        if (!save_file_agent_ptr_in.__check_self_valid__(mes_out)) {
            return;
        }

        // 3. 检查操作大小
        if (load_size_in == 0) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_zero_op_size_when_load_or_store)
        }

        // 4. 检查溢出（在调用底层API前）
        if (load_begin_in > SIZE_MAX - load_size_in) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overflow_when_load_or_store)
        }
        if (store_begin_in > SIZE_MAX - load_size_in) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overflow_when_load_or_store)
        }

        // 5. 检查缓存范围有效性
        if (!__check_cache_range_valid__(load_begin_in, load_size_in, mes_out)) {
            return;
        }
        if (!save_file_agent_ptr_in.__check_cache_range_valid__(store_begin_in, load_size_in, mes_out)) {
            return;
        }

        // 6. 如果是safe模式，检查缓存重叠
        if (safe) {
            void* self_ptr = nullptr;
            void* other_ptr = nullptr;
            cache_ptr.get_void_ptr(self_ptr);
            save_file_agent_ptr_in.cache_ptr.get_void_ptr(other_ptr);

            if (self_ptr == other_ptr) {
                // 同一个缓存，检查区间重叠
                uintptr_t self_op_begin = reinterpret_cast<uintptr_t>(self_ptr) + load_begin_in;
                uintptr_t self_op_end = self_op_begin + load_size_in;
                uintptr_t other_op_begin = reinterpret_cast<uintptr_t>(other_ptr) + store_begin_in;
                uintptr_t other_op_end = other_op_begin + load_size_in;

                if (!(self_op_end <= other_op_begin || other_op_end <= self_op_begin)) {
                     __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overlap_when_load_or_store)
                }
            }
        }

        // 7. 执行数据移动（使用agent_ptr的load方法）
        // 注意：agent_ptr::load已经包含了安全检查和memcpy
        // 但我们通过上面的检查确保了安全性
        cache_ptr.load(
            load_begin_in,
            load_size_in,
            save_file_agent_ptr_in.cache_ptr,
            store_begin_in,
            safe  // 传递给agent_ptr的safe参数
        );

        // 8. 更新脏标志
        // 源对象：读取操作不改变数据，不标记脏
        // 目标对象：写入新数据，标记为脏
        save_file_agent_ptr_in.__mark_dirty__();

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 从file_agent_ptr读取数据写入到自身
    void file_agent_ptr::store(
        file_agent_ptr& load_file_agent_ptr_in,
        size_t          load_begin_in,
        size_t          load_size_in,
        size_t          store_begin_in,
        bool            safe,
        mes::a_mes& mes_out
    ) {
        // 重用load函数，交换源和目标
        load_file_agent_ptr_in.load(
            load_begin_in,
            load_size_in,
            *this,
            store_begin_in,
            safe,
            mes_out
        );
    }

    // 将自身数据读取到agent_ptr
    void file_agent_ptr::load_to_agent_ptr(
        size_t           load_begin_in,
        size_t           load_size_in,
        bmms_f::agent_ptr& save_agent_ptr_in,
        size_t           store_begin_in,
        bool             safe,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 检查对方代理指针有效性
        bool other_is_null = false;
        save_agent_ptr_in.get_is_null(other_is_null);
        if (other_is_null) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_other_agent_ptr_is_null_when_load_or_store)
        }

        // 3. 检查操作大小
        if (load_size_in == 0) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_zero_op_size_when_load_or_store)
        }

        // 4. 检查溢出
        if (load_begin_in > SIZE_MAX - load_size_in) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overflow_when_load_or_store)

        }
        if (store_begin_in > SIZE_MAX - load_size_in) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overflow_when_load_or_store)
        }

        // 5. 检查自身缓存范围
        if (!__check_cache_range_valid__(load_begin_in, load_size_in, mes_out)) {
            return;
        }

        // 6. 检查对方代理指针范围
        bool other_range_valid = false;
        save_agent_ptr_in.get_check_range_bool(store_begin_in, load_size_in, other_range_valid);
        if (!other_range_valid) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_other_agent_ptr_range_outside_memory_when_load_or_store)
        }

        // 7. 如果是safe模式，检查缓存重叠
        if (safe) {
            void* self_ptr = nullptr;
            void* other_ptr = nullptr;
            cache_ptr.get_void_ptr(self_ptr);
            save_agent_ptr_in.get_void_ptr(other_ptr);

            if (self_ptr == other_ptr) {
                // 同一个代理指针，检查区间重叠
                uintptr_t self_op_begin = reinterpret_cast<uintptr_t>(self_ptr) + load_begin_in;
                uintptr_t self_op_end = self_op_begin + load_size_in;
                uintptr_t other_op_begin = reinterpret_cast<uintptr_t>(other_ptr) + store_begin_in;
                uintptr_t other_op_end = other_op_begin + load_size_in;

                if (!(self_op_end <= other_op_begin || other_op_end <= self_op_begin)) {
                     __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overlap_when_load_or_store)
                }
            }
        }

        // 8. 执行数据移动（使用agent_ptr的load方法）
        cache_ptr.load(
            load_begin_in,
            load_size_in,
            save_agent_ptr_in,
            store_begin_in,
            safe
        );

        // 9. 源对象不标记脏（读取操作）
        // 目标对象脏标志由调用者管理（不是file_agent_ptr的一部分）

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 从agent_ptr读取数据写入到自身
    void file_agent_ptr::store_from_agent_ptr(
        bmms_f::agent_ptr& load_agent_ptr_in,
        size_t           load_begin_in,
        size_t           load_size_in,
        size_t           store_begin_in,
        bool             safe,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 检查源代理指针有效性
        bool load_is_null = false;
        load_agent_ptr_in.get_is_null(load_is_null);
        if (load_is_null) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_other_agent_ptr_is_null_when_load_or_store)
        }

        // 3. 检查操作大小
        if (load_size_in == 0) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_zero_op_size_when_load_or_store)
        }

        // 4. 检查溢出
        if (load_begin_in > SIZE_MAX - load_size_in) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overflow_when_load_or_store)
        }
        if (store_begin_in > SIZE_MAX - load_size_in) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overflow_when_load_or_store)
        }

        // 5. 检查自身缓存范围
        if (!__check_cache_range_valid__(store_begin_in, load_size_in, mes_out)) {
        }

        // 6. 检查源代理指针范围
        bool load_range_valid = false;
        load_agent_ptr_in.get_check_range_bool(load_begin_in, load_size_in, load_range_valid);
        if (!load_range_valid) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_other_agent_ptr_range_outside_memory_when_load_or_store)
        }

        // 7. 如果是safe模式，检查缓存重叠
        if (safe) {
            void* self_ptr = nullptr;
            void* load_ptr = nullptr;
            cache_ptr.get_void_ptr(self_ptr);
            load_agent_ptr_in.get_void_ptr(load_ptr);

            if (self_ptr == load_ptr) {
                // 同一个代理指针，检查区间重叠
                uintptr_t self_op_begin = reinterpret_cast<uintptr_t>(self_ptr) + store_begin_in;
                uintptr_t self_op_end = self_op_begin + load_size_in;
                uintptr_t load_op_begin = reinterpret_cast<uintptr_t>(load_ptr) + load_begin_in;
                uintptr_t load_op_end = load_op_begin + load_size_in;

                if (!(self_op_end <= load_op_begin || load_op_end <= self_op_begin)) {
                     __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overlap_when_load_or_store)
                }
            }
        }

        // 8. 执行数据移动（使用agent_ptr的store方法）
        cache_ptr.store(
            load_agent_ptr_in,
            load_begin_in,
            load_size_in,
            store_begin_in,
            safe
        );

        // 9. 标记为脏（写入了新数据）
        __mark_dirty__();

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 将自身数据读取到void指针
    void file_agent_ptr::load_to_void_ptr(
        size_t      load_begin_in,
        size_t      load_size_in,
        void* store_ptr_in,
        bool        safe,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 检查void指针非空
        if (store_ptr_in == nullptr) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_void_ptr_is_nullptr_when_load_or_store)
        }

        // 3. 检查操作大小
        if (load_size_in == 0) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_zero_op_size_when_load_or_store)
        }

        // 4. 检查溢出
        if (load_begin_in > SIZE_MAX - load_size_in) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overflow_when_load_or_store)
        }

        // 5. 检查缓存范围
        if (!__check_cache_range_valid__(load_begin_in, load_size_in, mes_out)) {
            return;
        }

        // 6. 如果是safe模式，检查内存重叠
        if (safe) {
            void* self_ptr = nullptr;
            cache_ptr.get_void_ptr(self_ptr);

            uintptr_t self_op_begin = reinterpret_cast<uintptr_t>(self_ptr) + load_begin_in;
            uintptr_t self_op_end = self_op_begin + load_size_in;
            uintptr_t store_ptr_begin = reinterpret_cast<uintptr_t>(store_ptr_in);
            uintptr_t store_ptr_end = store_ptr_begin + load_size_in;

            // 检查是否部分重叠
            if (!(self_op_end <= store_ptr_begin || store_ptr_end <= self_op_begin)) {
                 __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overlap_when_load_or_store)
            }
        }

        // 7. 执行数据移动（使用agent_ptr的load_to_void_ptr方法）
        cache_ptr.load_to_void_ptr(
            load_begin_in,
            load_size_in,
            store_ptr_in,
            safe
        );

        // 8. 源对象不标记脏（读取操作）

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 从void指针读数据写入自身
    void file_agent_ptr::store_from_void_ptr(
        void* load_ptr_in,
        size_t      load_size_in,
        size_t      store_begin_in,
        bool        safe,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 检查void指针非空
        if (load_ptr_in == nullptr) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_void_ptr_is_nullptr_when_load_or_store)
        }

        // 3. 检查操作大小
        if (load_size_in == 0) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_zero_op_size_when_load_or_store)
        }

        // 4. 检查溢出
        if (store_begin_in > SIZE_MAX - load_size_in) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overflow_when_load_or_store)
        }

        // 5. 检查缓存范围
        if (!__check_cache_range_valid__(store_begin_in, load_size_in, mes_out)) {
            return;
        }

        // 6. 如果是safe模式，检查内存重叠
        if (safe) {
            void* self_ptr = nullptr;
            cache_ptr.get_void_ptr(self_ptr);

            uintptr_t self_op_begin = reinterpret_cast<uintptr_t>(self_ptr) + store_begin_in;
            uintptr_t self_op_end = self_op_begin + load_size_in;
            uintptr_t load_ptr_begin = reinterpret_cast<uintptr_t>(load_ptr_in);
            uintptr_t load_ptr_end = load_ptr_begin + load_size_in;

            // 检查是否部分重叠
            if (!(self_op_end <= load_ptr_begin || load_ptr_end <= self_op_begin)) {
                 __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_op_range_overlap_when_load_or_store)
            }
        }

        // 7. 执行数据移动（使用agent_ptr的store_from_void_ptr方法）
        cache_ptr.store_from_void_ptr(
            load_ptr_in,
            load_size_in,
            store_begin_in,
            safe
        );

        // 8. 标记为脏（写入了新数据）
        __mark_dirty__();

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    //============================ 信息获取 ============================
    
    // 获取是否已初始化
    void file_agent_ptr::get_is_inited(bool& is_inited_out) {
        is_inited_out = inited;
    }

    // 获取绑定的文件范围起始位置
    void file_agent_ptr::get_cache_begin_about_bin_file(size_t& begin_out) {
        if (!inited) {
            begin_out = 0;
            return;
        }
        begin_out = cache_begin_about_bin_file;
    };

    // 获取缓存代理指针
    void file_agent_ptr::get_cache_agent_ptr(bmms_f::agent_ptr& cache_agent_ptr_out) {
        if (!inited) {
            cache_agent_ptr_out = bmms_f::agent_ptr();
            return;
        }
        cache_agent_ptr_out = cache_ptr;
    };

    // 获取缓存代理指针的原始指针
    void file_agent_ptr::get_cache_raw_ptr(void*& raw_ptr_out) {
        if (!inited) {
            raw_ptr_out = nullptr;
            return;
        }
        cache_ptr.get_void_ptr(raw_ptr_out);
    }

    // 获取缓存大小
    void file_agent_ptr::get_cache_size(size_t& cache_size_out) {
        if (!inited) {
            cache_size_out = 0;
            return;
        }
        cache_ptr.get_size(cache_size_out);
    }

    // 获取脏标志状态
    void file_agent_ptr::get_dirty_flag(bool& is_dirty_out) {
        is_dirty_out = dirty;
    };

    // 检查指定文件范围是否在当前管理范围内
    void file_agent_ptr::check_file_range_in_managed(
        const shared_ptr<bfs_f::bin_file>& bin_file_in,
        size_t file_begin,
        size_t size_in,
        bool& is_in_managed_out,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");
        is_in_managed_out = false;

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 检查是否是同一个文件（通过 shared_ptr 比较）
        if (bin_file_shared_ptr != bin_file_in) {
            // 不是同一个文件
            mes_out = mes::a_mes(0, 0, "");
            is_in_managed_out = false;
            return;
        }

        // 3. 检查文件范围有效性
        if (!__check_file_range_valid__(*bin_file_in, file_begin, size_in, mes_out)) {
            return;
        }

        // 4. 检查是否在管理范围内
        is_in_managed_out = (file_begin >= cache_begin_about_bin_file);

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 检查指定缓存范围是否在当前缓存内
    void file_agent_ptr::check_cache_range_in_managed(
        size_t cache_begin,
        size_t size_in,
        bool& is_in_managed_out,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");
        is_in_managed_out = false;

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 检查缓存范围有效性
        if (!__check_cache_range_valid__(cache_begin, size_in, mes_out)) {
            return;
        }

        // 3. 总是在缓存内，因为 __check_cache_range_valid__ 已经验证了
        is_in_managed_out = true;

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 获取管理的文件范围大小
    void file_agent_ptr::get_managed_file_size(
        size_t& managed_size_out,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");
        managed_size_out = 0;

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 检查文件是否打开
        if (bin_file_shared_ptr == nullptr || !bin_file_shared_ptr->__check_inited__()) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_get_managed_size();
            return;
        }

        // 3. 获取文件大小
        size_t file_size = bin_file_shared_ptr->__file_size__();

        // 4. 计算管理的文件范围大小
        if (cache_begin_about_bin_file > file_size) {
            // 文件被缩小了，管理的范围大小为0
            managed_size_out = 0;
        }
        else {
            managed_size_out = file_size - cache_begin_about_bin_file;
        }

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 获取缓存使用情况（已用大小）
    void file_agent_ptr::get_cache_used_size(
        size_t& used_size_out,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");
        used_size_out = 0;

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 获取缓存总大小
        size_t cache_size = 0;
        cache_ptr.get_size(cache_size);

        // 当前设计：缓存完全用于文件范围，所以使用大小等于缓存大小
        // 如果有脏标志或部分使用，这里需要调整
        used_size_out = cache_size;

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 检查文件是否仍然有效（通过测试 seek）
    void file_agent_ptr::check_file_still_valid(
        bool& is_valid_out,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");
        is_valid_out = false;

        // 1. 检查文件是否打开
        if (bin_file_shared_ptr == nullptr || !bin_file_shared_ptr->__check_inited__()) {
            is_valid_out = false;
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_check_validity();
            return;
        }

        // 2. 尝试检查文件状态
        try {
            is_valid_out = bin_file_shared_ptr->__check_inited__();

            // 检查文件范围是否仍然有效
            size_t file_size = bin_file_shared_ptr->__file_size__();
            if (cache_begin_about_bin_file > file_size) {
                // 文件被缩小，超出了管理范围
                is_valid_out = false;
                mes_out = bfs_m::err::service_error_file_agent_ptr_file_modified_externally();
                return;
            }
        }
        catch (...) {
            is_valid_out = false;
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_unknown_error_when_check_validity();
            return;
        }

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // ====================== 辅助信息函数 ======================

    // 计算文件绝对位置对应的缓存位置
    void file_agent_ptr::calculate_cache_position(
        size_t file_absolute_pos,
        size_t& cache_pos_out,
        bool& is_mappable_out,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");
        cache_pos_out = 0;
        is_mappable_out = false;

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 检查文件位置是否在管理范围内
        if (file_absolute_pos < cache_begin_about_bin_file) {
            // 在管理范围之前
            is_mappable_out = false;
            return;
        }

        // 3. 获取缓存大小
        size_t cache_size = 0;
        cache_ptr.get_size(cache_size);

        // 4. 计算缓存位置
        size_t offset = file_absolute_pos - cache_begin_about_bin_file;
        if (offset >= cache_size) {
            // 超出缓存大小
            is_mappable_out = false;
            return;
        }

        cache_pos_out = offset;
        is_mappable_out = true;

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 计算缓存位置对应的文件绝对位置
    void file_agent_ptr::calculate_file_position(
        size_t cache_pos,
        size_t& file_absolute_pos_out,
        bool& is_mappable_out,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");
        file_absolute_pos_out = 0;
        is_mappable_out = false;

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return;
        }

        // 2. 检查缓存位置是否有效
        size_t cache_size = 0;
        cache_ptr.get_size(cache_size);

        if (cache_pos >= cache_size) {
            // 超出缓存大小
            is_mappable_out = false;
            return;
        }

        // 3. 计算文件绝对位置
        file_absolute_pos_out = cache_begin_about_bin_file + cache_pos;
        is_mappable_out = true;

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 标记为脏（用于数据写入操作后调用）
    void file_agent_ptr::__mark_dirty__() {
        if (inited) {
            dirty = true;
        }
    };

    // 清除脏标志（用于数据读取操作后调用）
    void file_agent_ptr::__clear_dirty__() {
        if (inited) {
            dirty = false;
        }
    };

    void file_agent_ptr::__cleanup_on_construct_failure__() {
        inited = false;
        dirty  = false;
        cache_ptr = bmms_f::agent_ptr();
        cache_begin_about_bin_file = 0;
        bin_file_shared_ptr = nullptr;
    };

    // 获取关联的 bin_file shared_ptr
    void file_agent_ptr::get_associated_bin_file_shared_ptr(
        shared_ptr<bfs_f::bin_file>& bin_file_out
    ) {
        if (!inited) {
            bin_file_out = nullptr;
            return;
        }
        bin_file_out = bin_file_shared_ptr;
    }

    // 检查是否关联到指定的 bin_file
    void file_agent_ptr::check_associated_with_bin_file_shared_ptr(
        shared_ptr<bfs_f::bin_file> bin_file_in,
        bool& is_associated_out,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");
        is_associated_out = false;

        if (!inited) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_not_inited_when_checking)
            return;
        }

        is_associated_out = (bin_file_shared_ptr == bin_file_in);

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    //============================ 定理校验 ============================
    
    // 定理 FAP1：自身状态有效性检查
    bool file_agent_ptr::__check_self_valid__(mes::a_mes& mes_out) {
        mes_out = mes::a_mes(0, 0, "");

        if (!inited) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_not_inited_when_checking)
        }

        bool cache_is_null = false;
        cache_ptr.get_is_null(cache_is_null);
        if (cache_is_null) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_not_inited_when_checking)
        }

        size_t cache_size = 0;
        cache_ptr.get_size(cache_size);
        if (cache_size == 0) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_cache_agent_ptr_size_zero_when_checking)
        }

        return true;
    }

    // 定理 FAP2：文件范围有效性检查（复用 bin_file 的公共接口）
    bool file_agent_ptr::__check_file_range_valid__(
        bfs_f::bin_file& file_in,
        size_t& file_begin,
        size_t& size_in,
        mes::a_mes& mes_out
    ) {
        // 重置消息
        mes_out = mes::a_mes(0, 0, "");

        // 1. 文件必须打开（复用 bin_file 的检查）
        if (!file_in.__check_inited__()) {
            mes_out = bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_check_file_range();
            return false;
        }

        // 2. 复用 bin_file 的范围检查
        if (!file_in.__check_file_range__(file_begin, size_in)) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_file_range_outside_file_when_operate)
        }

        // 3. 操作大小必须为正（对于数据操作）
        if (size_in == 0) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_zero_op_size_when_checking)
        }

        return true;
    }

    // 定理 FAP3：缓存范围有效性检查（复用 agent_ptr 的公共接口）
    bool file_agent_ptr::__check_cache_range_valid__(size_t& cache_begin,
        size_t& size_in,
        mes::a_mes& mes_out
    ) {
        // 重置消息
        mes_out = mes::a_mes(0, 0, "");

        // 1. 先检查自身状态
        if (!__check_self_valid__(mes_out)) {

            return false;
        }

        // 2. 复用 agent_ptr 的范围检查
        bool range_valid = false;
        cache_ptr.get_check_range_bool(cache_begin, size_in, range_valid);
        if (!range_valid) {
            __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_cache_range_outside_memory_when_operate)
        }

        // 3. 操作大小必须为正（对于数据操作）
        if (size_in == 0) {
             __CODING_ERROR__(bfs_m::err::coding_error_file_agent_ptr_zero_op_size_when_checking)
        }

        return true;
    }

    // 定理 FAP4：两个 BFS_AGENT_PTR 操作兼容性检查
    bool file_agent_ptr::__check_two_fap_compatible__(file_agent_ptr& other_fap,
        size_t& self_cache_begin,
        size_t& other_cache_begin,
        size_t& size_in,
        mes::a_mes& mes_out
    ) {
        // 重置消息
        mes_out = mes::a_mes(0, 0, "");

        // 1. 检查自身状态
        if (!__check_self_valid__(mes_out)) {
            return false;
        }

        // 2. 检查对方状态
        mes::a_mes other_mes;
        if (!other_fap.__check_self_valid__(other_mes)) {
            mes_out = other_mes; // 传递错误
            return false;
        }

        // 3. 检查自身缓存范围
        if (!__check_cache_range_valid__(self_cache_begin, size_in, mes_out)) {
            return false;
        }

        // 4. 检查对方缓存范围
        if (!other_fap.__check_cache_range_valid__(other_cache_begin, size_in, mes_out)) {
            return false;
        }

        return true;
    }
}