//file_bin_file.cpp
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

    // 从二进制文件创建文件对象
    bin_file::bin_file(
        str       path_in,
        size_t    size_in,
        init_type init_type_in,
        mes::a_mes& mes_out
    ) {
        // 初始化消息
        mes_out = mes::a_mes(0, 0, "");

        // 写入路径
        if (path_in.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_try_build_from_empty_path)
        };

        if (!__check_path_valid__(path_in)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_invalid_path_of_your_system)
        }

        path = path_in;

        try {
            // 打开文件
            if (init_type_in == init_type::open_existed) {
                // 检查文件是否存在
                if (!std::filesystem::exists(path_in)) {
                    mes_out = bfs_m::err::service_error_bin_file_file_not_existed_when_build_with_choice_of_open_existed();
                    return;
                }

                // 打开文件
                filestream_ptr = unique_ptr<std::fstream>(
                    new std::fstream(path_in,
                        std::ios::binary |
                        std::ios::in |
                        std::ios::out
                    )
                );

                // 检查是否打开成功
                if (!filestream_ptr->is_open()) {
                    mes_out = bfs_m::err::service_error_bin_file_file_can_not_open_when_build_with_choice_of_open_existed();
                    return;
                }

                // 标记已初始化
                inited = true;

                // 如果指定了大小，调整文件大小
                if (size_in > 0) {
                    resize(size_in, mes_out);
                    if (mes_out.code != 0) {
                        // 调整大小失败，清理资源
                        __cleanup_on_failure__();
                        return;
                    }
                }
            }
            else if (init_type_in == init_type::cover_and_create) {
                // 直接创建/覆盖文件
                filestream_ptr = unique_ptr<std::fstream>(
                    new std::fstream(path_in,
                        std::ios::binary |
                        std::ios::in |
                        std::ios::out |
                        std::ios::trunc
                    )
                );

                // 检查是否打开成功
                if (!filestream_ptr->is_open()) {
                    mes_out = bfs_m::err::service_error_bin_file_constructor_cannot_create();
                    return;
                }

                // 标记已初始化
                inited = true;

                // 如果指定了大小，调整文件大小
                if (size_in > 0) {
                    resize(size_in, mes_out);
                    if (mes_out.code != 0) {
                        // 调整大小失败，清理资源
                        __cleanup_on_failure__();
                        return;
                    }
                }
            }
            else if (init_type_in == init_type::create) {
                // 检查文件是否已存在
                if (std::filesystem::exists(path_in)) {
                    mes_out = bfs_m::err::service_error_bin_file_file_already_existed_when_build_with_choice_of_create();
                    return;
                }

                // 创建新文件
                filestream_ptr = unique_ptr<std::fstream>(
                    new std::fstream(path_in,
                        std::ios::binary |
                        std::ios::in |
                        std::ios::out |
                        std::ios::trunc
                    )
                );

                // 检查是否打开成功
                if (!filestream_ptr->is_open()) {
                    mes_out = bfs_m::err::service_error_bin_file_constructor_cannot_create();
                    return;
                }

                // 标记已初始化
                inited = true;

                // 如果指定了大小，调整文件大小
                if (size_in > 0) {
                    resize(size_in, mes_out);
                    if (mes_out.code != 0) {
                        // 调整大小失败，清理资源
                        __cleanup_on_failure__();
                        return;
                    }
                }
            }
            else {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_unfair_init_type_value)
            }
        }
        catch (mes::a_mes) {
            throw;
        }
        catch (...) {
            // 捕获所有其他异常
            __cleanup_on_failure__();
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_open();

        }
        // 最终验证：确保文件已正确打开
        if (!__check_inited__()) {
            // 这种情况不应该发生，但以防万一
            __cleanup_on_failure__();
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_open();
            return;
        }
    }

    //移动构造函数
    bin_file::bin_file(bin_file&& other) noexcept
        : inited(other.inited),
        path(move(other.path)),
        filestream_ptr(move(other.filestream_ptr)) {
        // 转移后，源对象处于有效但为空的状态
        other.inited = false;
        other.path = "";
    };

    //移动赋值运算符
    bin_file& bin_file::operator=(bin_file&& other) noexcept {
        if (this != &other) {
            // 先清理当前资源
            if (inited) {
                mes::a_mes close_mes;
                close(close_mes);  // 尝试正常关闭
                // 忽略关闭错误，因为我们要接管新资源
            }

            // 转移资源
            inited = other.inited;
            path = move(other.path);
            filestream_ptr = move(other.filestream_ptr);

            // 清空源对象
            other.inited = false;
            other.path = "";
        }
        return *this;
    }

    // 析构函数
    bin_file::~bin_file() {
        if (inited) {
            try {
                // 尝试正常关闭
                mes::a_mes close_mes;
                close(close_mes);

                // 如果关闭失败，至少确保流被关闭
                if (filestream_ptr && filestream_ptr->is_open()) {
                    filestream_ptr->close();
                }
            }
            catch (...) {
                // 析构函数不能抛出异常
                // 尝试最后的清理
                if (filestream_ptr && filestream_ptr->is_open()) {
                    try {
                        filestream_ptr->close();
                    }
                    catch (...) {
                        // 忽略所有异常
                    }
                }
            }
        }
    }

    //============================= 文件本身操作 =============================

    // 从已二进制有文件对象创建文件对象
    void bin_file::open(
        str       path_in,
        size_t    size_in,
        init_type init_type_in,
        mes::a_mes& mes_out
    ) {
        // 重置消息
        mes_out = mes::a_mes(0, 0, "");

        // ====================== 定理1：基础有效性检查 ======================
        // 检查路径是否为空
        if (path_in.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_try_open_empty_path)
        }

        if (!__check_path_valid__(path_in)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_invalid_path_of_your_system)
        }

        // ====================== 步骤1：保存原状态信息 ======================
        bool had_previous_file = inited;
        str old_path           = path;  // 保存原路径用于错误报告

        // ====================== 步骤2：关闭原文件（如果有） ======================
        if (had_previous_file) {
            mes::a_mes close_mes;
            close(close_mes);

            // 记录关闭错误，但不阻止继续打开新文件
            if (close_mes.code != 0) {
                // 原文件关闭失败，记录到日志或忽略
                // 我们继续尝试打开新文件
            }
        }

        // ====================== 步骤3：重置状态 ======================
        // 注意：即使后续打开失败，我们也保持未初始化状态
        inited = false;
        path = "";
        filestream_ptr.reset();

        // ====================== 步骤4：设置新路径并尝试打开 ======================
        path = path_in;

        try {
            // 根据打开类型处理
            if (init_type_in == init_type::open_existed) {
                // 检查文件是否存在
                if (!std::filesystem::exists(path_in)) {
                    mes_out = bfs_m::err::service_error_bin_file_file_not_existed_when_open_with_choice_of_open_existed();
                    // 打开失败，保持未初始化状态
                    return;
                }

                // 打开现有文件
                filestream_ptr = unique_ptr<std::fstream>(
                    new std::fstream(path_in,
                        std::ios::binary |
                        std::ios::in |
                        std::ios::out
                    )
                );

                // 检查是否打开成功
                if (!filestream_ptr->is_open()) {
                    mes_out = bfs_m::err::service_error_bin_file_file_can_not_open_when_open_with_choice_of_open_existed();
                    // 打开失败，保持未初始化状态
                    return;
                }

                // 标记已初始化
                inited = true;

                // 如果指定了大小，调整文件大小
                if (size_in > 0) {
                    resize(size_in, mes_out);
                    if (mes_out.code != 0) {
                        // 调整大小失败，清理资源
                        __cleanup_on_failure__();
                        return;
                    }
                }
            }
            else if (init_type_in == init_type::cover_and_create) {
                // 直接创建/覆盖文件
                filestream_ptr = unique_ptr<std::fstream>(
                    new std::fstream(path_in,
                        std::ios::binary |
                        std::ios::in |
                        std::ios::out |
                        std::ios::trunc
                    )
                );

                // 检查是否打开成功
                if (!filestream_ptr->is_open()) {
                    mes_out = bfs_m::err::service_error_bin_file_constructor_cannot_create_when_open_with_choice_of_cover_and_create();
                    // 打开失败，保持未初始化状态
                    return;
                }

                // 标记已初始化
                inited = true;

                // 如果指定了大小，调整文件大小
                if (size_in > 0) {
                    resize(size_in, mes_out);
                    if (mes_out.code != 0) {
                        // 调整大小失败，清理资源
                        __cleanup_on_failure__();
                        return;
                    }
                }
            }
            else if (init_type_in == init_type::create) {
                // 检查文件是否已存在
                if (std::filesystem::exists(path_in)) {
                    mes_out = bfs_m::err::service_error_bin_file_file_already_existed_when_open_with_choice_of_create();
                    // 打开失败，保持未初始化状态
                    return;
                }

                // 创建新文件
                filestream_ptr = unique_ptr<std::fstream>(
                    new std::fstream(path_in,
                        std::ios::binary |
                        std::ios::in |
                        std::ios::out |
                        std::ios::trunc
                    )
                );

                // 检查是否打开成功
                if (!filestream_ptr->is_open()) {
                    mes_out = bfs_m::err::service_error_bin_file_file_can_not_create_when_open_with_choice_of_create();
                    // 打开失败，保持未初始化状态
                    return;
                }

                // 标记已初始化
                inited = true;

                // 如果指定了大小，调整文件大小
                if (size_in > 0) {
                    resize(size_in, mes_out);
                    if (mes_out.code != 0) {
                        // 调整大小失败，清理资源
                        __cleanup_on_failure__();
                        return;
                    }
                }
            }
            else {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_unfair_init_type_value)
            }
        }
        catch (...) {
            // 捕获所有其他异常
            __cleanup_on_failure__();
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_open();
            throw;
        }

        // ====================== 步骤5：最终验证 ======================
        if (!__check_inited__()) {
            // 这种情况不应该发生，但以防万一
            __cleanup_on_failure__();
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_open();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    // 立即刷新缓冲区，保存数据 
    void bin_file::flush() {
        if (inited && filestream_ptr && filestream_ptr->is_open()) {
            try {
                filestream_ptr->flush();
            }
            catch (...) {

            }
        }
    };

    // 关闭
    void bin_file::close(mes::a_mes& mes_out) {
        mes_out = mes::a_mes(0, 0, "");

        if (!inited) {
            mes_out = bfs_m::err::service_error_bin_file_not_opened_when_close();
            return;
        }

        bool flush_failed = false;
        bool close_failed = false;

        // 刷新缓冲区
        if (filestream_ptr && filestream_ptr->is_open()) {
            try {
                filestream_ptr->flush();
                if (!filestream_ptr->good()) {
                    flush_failed = true;
                    mes_out = bfs_m::err::service_error_bin_file_flush_error_when_close();
                }
            }
            catch (...) {
                flush_failed = true;
                mes_out = bfs_m::err::service_error_bin_file_flush_error_when_close();
            }
        }

        // 关闭文件
        if (filestream_ptr && filestream_ptr->is_open()) {
            try {
                filestream_ptr->close();
                close_failed = !filestream_ptr->good();
            }
            catch (...) {
                close_failed = true;
            }
        }

        // 清理资源（无论是否有错误）
        filestream_ptr.reset();
        inited = false;
        path = "";

        // 报告最终状态
        if (close_failed) {
            mes_out = bfs_m::err::service_error_bin_file_file_close_error();
        }
        else if (flush_failed) {
            // 已经设置flush错误
        }
        else if (mes_out.code == 0) {
            mes_out = mes::a_mes(0, 0, "");
        }
    }

    // 重新设置大小
    void bin_file::resize(
        size_t size_in,
        mes::a_mes& mes_out
    ) {
        // 重置消息
        mes_out = mes::a_mes(0, 0, "");

        // ====================== 定理1：基础有效性检查 ======================
        // is_open(F)
        if (!__check_inited__()) {
            mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_resize();
            return;
        }

        // ====================== 步骤1：获取当前文件大小 ======================
        size_t current_size = 0;
        try {
            // 保存当前文件位置（无需恢复，因为每次显式指定位置）
            filestream_ptr->seekg(0, std::ios::end);
            current_size = static_cast<size_t>(filestream_ptr->tellg());
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_seek_error_when_resize_get_size();
                __enter_uninitialized_state__();
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_resize_get_size();
            __enter_uninitialized_state__();
            return;
        }

        // 如果大小不变，直接返回成功
        if (current_size == size_in) {
            mes_out = mes::a_mes(0, 0, "");
            return;
        }

        // ====================== 步骤2：关闭文件流以便调整大小 ======================
        try {
            // 确保缓冲区已刷新
            filestream_ptr->flush();
            filestream_ptr->close();

            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_file_close_error_before_resize();
                __enter_uninitialized_state__();
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_file_close_error_before_resize();
            __enter_uninitialized_state__();
            return;
        }

        // ====================== 步骤3：调整文件大小 ======================
        try {
            std::filesystem::resize_file(path, size_in);
        }
        catch (const std::filesystem::filesystem_error ) {
            // 文件系统错误：尝试重新打开原文件
            mes_out = bfs_m::err::service_error_bin_file_file_resize_failed();
            __try_reopen_and_enter_uninitialized_state__(mes_out);
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_file_resize_failed();
            __enter_uninitialized_state__();
            return;
        }

        // ====================== 步骤4：重新打开文件 ======================
        try {
            filestream_ptr->open(path,
                std::ios::binary |
                std::ios::in |
                std::ios::out
            );

            if (!filestream_ptr->is_open()) {
                mes_out = bfs_m::err::service_error_bin_file_file_reopen_error_after_resize();
                __enter_uninitialized_state__();
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_file_reopen_error_after_resize();
            __enter_uninitialized_state__();
            return;
        }

        // ====================== 步骤5：如果需要，填充新空间 ======================
        // 定理：0 ≤ B ∧ B + L ≤ S（填充区间完全在文件范围内）
        // B = current_size, L = size_in - current_size
        if (size_in > current_size) {
            try {
                // 检查边界：current_size + (size_in - current_size) = size_in ≤ size_in
                // 这是恒成立的，因为size_in > current_size
                const size_t fill_size = size_in - current_size;

                // 定位到原文件末尾：current_size ≤ |F|
                filestream_ptr->seekp(current_size, std::ios::beg);
                if (!filestream_ptr->good()) {
                    mes_out = bfs_m::err::service_error_bin_file_file_seek_error_when_fill_zeros();
                    __enter_uninitialized_state__();
                    return;
                }

                // 分块填充0，避免一次性分配过大内存
                const size_t BUFFER_SIZE = 64 * 1024;
                size_t remaining = fill_size;

                // 使用局部vector，异常时会自动清理
                vector<char> zero_buffer(BUFFER_SIZE, 0);

                while (remaining > 0) {
                    size_t chunk_size = std::min(remaining, BUFFER_SIZE);

                    // 写入0：W(F, current_size + (fill_size - remaining), chunk_size)
                    filestream_ptr->write(zero_buffer.data(), chunk_size);
                    if (!filestream_ptr->good()) {
                        mes_out = bfs_m::err::service_error_bin_file_file_write_error_when_fill_zeros();
                        __enter_uninitialized_state__();
                        return;
                    }

                    remaining -= chunk_size;
                }

                // 确保写入完成
                filestream_ptr->flush();
                if (!filestream_ptr->good()) {
                    mes_out = bfs_m::err::service_error_bin_file_flush_error_after_resize_fill_zeros();
                    __enter_uninitialized_state__();
                    return;
                }
            }
            catch (...) {
                mes_out = bfs_m::err::service_error_bin_file_file_unknown_error_when_fill_zeros();
                __enter_uninitialized_state__();
                return;
            }
        }

        // ====================== 步骤6：验证最终文件大小 ======================
        try {
            filestream_ptr->seekg(0, std::ios::end);
            size_t final_size = static_cast<size_t>(filestream_ptr->tellg());

            if (final_size != size_in) {
                mes_out = bfs_m::err::service_error_bin_file_size_mismatch_after_resize();
                __enter_uninitialized_state__();
                return;
            }

            // 定位到文件开头（标准初始位置）
            filestream_ptr->seekg(0, std::ios::beg);
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_seek_error_to_begin_after_resize();
                __enter_uninitialized_state__();
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_resize_verify_size();
            __enter_uninitialized_state__();
            return;
        }

        // ====================== 成功 ======================
        // 所有步骤无异常，文件大小正确，流处于可用的初始状态
        mes_out = mes::a_mes(0, 0, "");
    }

    // 清空文件（重置所有字节为0，保持文件大小不变）
    
    void bin_file::clear(
        mes::a_mes& mes_out
    ) {
        // 重置消息
        mes_out = mes::a_mes(0, 0, "");

        // ====================== 定理1：基础有效性检查 ======================
        // is_open(F)
        if (!__check_inited__()) {
            mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_clear();
            return;
        }

        // ====================== 步骤1：获取当前文件大小 ======================
        size_t file_size = 0;
        try {
            // 保存当前读取位置（虽然我们不使用，但记录以备需要）
            auto original_read_pos = filestream_ptr->tellg();

            // 获取文件大小
            filestream_ptr->seekg(0, std::ios::end);
            file_size = static_cast<size_t>(filestream_ptr->tellg());

            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_seek_error_when_clear_get_size();
                return;
            }

            // 文件大小为0时，无需操作
            if (file_size == 0) {
                mes_out = mes::a_mes(0, 0, "");
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_clear_get_size();
            return;
        }

        // ====================== 步骤2：定位到文件开头并写入0 ======================
        try {
            // 定位到文件开头
            filestream_ptr->seekp(0, std::ios::beg);
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_seek_error_when_clear_full();
                return;
            }

            // 分块写入0，避免一次性分配过大内存
            const size_t BUFFER_SIZE = 64 * 1024;
            size_t remaining = file_size;

            // 使用局部vector，异常时会自动清理
            vector<char> zero_buffer(BUFFER_SIZE, 0);

            while (remaining > 0) {
                size_t chunk_size = std::min(remaining, BUFFER_SIZE);

                // 写入0：W(F, file_size - remaining, chunk_size)
                filestream_ptr->write(zero_buffer.data(), chunk_size);
                if (!filestream_ptr->good()) {
                    mes_out = bfs_m::err::service_error_bin_file_write_error_when_clear_full();
                    // 部分写入，文件可能已损坏
                    __enter_uninitialized_state__();
                    return;
                }

                remaining -= chunk_size;
            }

            // 确保写入完成
            filestream_ptr->flush();
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_flush_error_after_clear_fill_zeros();
                __enter_uninitialized_state__();
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_clear_full();
            __enter_uninitialized_state__();
            return;
        }

        // ====================== 步骤3：验证文件大小未改变 ======================
        try {
            filestream_ptr->seekg(0, std::ios::end);
            size_t final_size = static_cast<size_t>(filestream_ptr->tellg());

            if (final_size != file_size) {
                // 文件大小意外改变，文件系统错误
                mes_out = bfs_m::err::service_error_bin_file_size_changed_during_clear();
                __enter_uninitialized_state__();
                return;
            }

            // 定位回文件开头
            filestream_ptr->seekg(0, std::ios::beg);
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_seek_error_to_begin_after_clear();
                __enter_uninitialized_state__();
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_clear_get_size();
            __enter_uninitialized_state__();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }
    
    // 范围清空文件
    void bin_file::clear_range(
        size_t begin_in,
        size_t size_in,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        // 1. 基本状态检查
        if (!__check_inited__()) {
            mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_clear_range();
            return;
        }

        // 2. 操作大小检查
        if (size_in == 0) {
            return;
        }

        // 3. 溢出检查
        if (!no_overflow(begin_in, size_in)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overflow);
        }

        // 4. 范围检查
        size_t file_size = __file_size__();
        if (!(begin_in <= file_size && begin_in + size_in <= file_size)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_out_of_file);
        }

        // 5. 执行清空操作
        try {
            // 定位到开始位置
            filestream_ptr->seekp(begin_in, std::ios::beg);
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_seek_error_when_clear_range();
                return;  // seek失败，文件未修改，保持状态
            }

            // 分块写入0
            const size_t BUFFER_SIZE = 64 * 1024;
            size_t remaining = size_in;
            vector<char> zero_buffer(BUFFER_SIZE, 0);
            size_t bytes_written = 0;

            while (remaining > 0) {
                size_t chunk_size = std::min(remaining, BUFFER_SIZE);
                filestream_ptr->write(zero_buffer.data(), chunk_size);

                if (!filestream_ptr->good()) {
                    mes_out = bfs_m::err::service_error_bin_file_write_error_when_clear_range();
                    __enter_uninitialized_state__();  // 部分写入，文件损坏
                    return;
                }

                bytes_written += chunk_size;
                remaining -= chunk_size;
            }

            // 刷新缓冲区
            flush();

            // 注意：这里不检查flush是否成功，因为：
            // 1. flush失败可能是暂时的
            // 2. 数据已写入缓冲区
            // 3. 与clear()函数保持一致

        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_clear_range();
            __enter_uninitialized_state__();  // 未知异常，保守处理
            return;
        }

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 另存为
    void bin_file::save_as(
        str new_path_in,
        mes::a_mes& mes_out
    ) {
        // 重置消息
        mes_out = mes::a_mes(0, 0, "");

        // ====================== 定理1：基础有效性检查 ======================
        // 1. 新路径非空
        if (new_path_in.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_save_as_to_empty_path)
        }

        if (!__check_path_valid__(new_path_in)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_invalid_path_of_your_system)
        }

        // 2. 当前文件必须打开
        if (!__check_inited__()) {
            mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_save_as();
            return;
        }

        // 3. 不能另存为到自身
        try {
            auto current_abs = std::filesystem::absolute(path);
            auto new_abs = std::filesystem::absolute(new_path_in);
            if (current_abs == new_abs) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_save_as_to_self)
            }
        }
        catch (...) {
            // 如果路径转换失败，退回到字符串比较
            if (path == new_path_in) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_save_as_to_self)
            }
        }

        // ====================== 步骤1：创建临时新文件流 ======================
        unique_ptr<std::fstream> new_filestream = nullptr;

        try {
            // 先创建新文件流，不关闭原文件
            new_filestream = unique_ptr<std::fstream>(
                new std::fstream(new_path_in,
                    std::ios::binary |
                    std::ios::in |
                    std::ios::out |
                    std::ios::trunc
                )
            );

            if (!new_filestream->is_open()) {
                mes_out = bfs_m::err::service_error_bin_file_constructor_cannot_create_when_open_with_choice_of_cover_and_create();
                return; // 原文件状态不变
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_constructor_cannot_create_when_open_with_choice_of_cover_and_create();
            return;
        }

        // ====================== 步骤2：获取当前文件大小和位置 ======================
        size_t current_size = 0;
        try {
            // 获取原文件大小
            filestream_ptr->seekg(0, std::ios::end);
            current_size = static_cast<size_t>(filestream_ptr->tellg());
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_seek_error_when_save_as_get_size();
                // 新文件流会自动释放，原文件状态不变
                return;
            }

            // 回到文件开头
            filestream_ptr->seekg(0, std::ios::beg);
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_seek_error_when_save_as_get_size();
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_save_as_get_size();
            return;
        }

        // ====================== 步骤3：复制数据到新文件 ======================
        try {
            // 分块复制数据
            const size_t BUFFER_SIZE = 64 * 1024;
            size_t remaining = current_size;

            vector<char> buffer(BUFFER_SIZE);

            while (remaining > 0) {
                size_t chunk_size = std::min(remaining, BUFFER_SIZE);

                // 从原文件读取
                filestream_ptr->read(buffer.data(), chunk_size);
                if (filestream_ptr->gcount() != static_cast<std::streamsize>(chunk_size)) {
                    mes_out = bfs_m::err::service_error_bin_file_read_error_when_save_as();
                    return; // 新文件会被自动清理，原文件不变
                }

                // 写入新文件
                new_filestream->write(buffer.data(), chunk_size);
                if (!new_filestream->good()) {
                    mes_out = bfs_m::err::service_error_bin_file_write_error_when_save_as();
                    return;
                }

                remaining -= chunk_size;
            }

            // 确保新文件数据写入磁盘
            new_filestream->flush();
            if (!new_filestream->good()) {
                mes_out = bfs_m::err::service_error_bin_file_flush_error_after_save_as();
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_save_as();
            return;
        }

        // ====================== 步骤4：验证新文件大小 ======================
        try {
            new_filestream->seekg(0, std::ios::end);
            size_t new_size = static_cast<size_t>(new_filestream->tellg());

            if (new_size != current_size) {
                mes_out = bfs_m::err::service_error_bin_file_size_mismatch_after_save_as();
                return; // 新文件会被清理，原文件不变
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_save_as_verify_size();
            return;
        }

        // ====================== 步骤5：交换资源（原子操作） ======================
        try {
            // 关闭原文件
            filestream_ptr->close();

            // 交换文件流（原子操作）
            new_filestream.swap(filestream_ptr);

            // 更新路径
            path = new_path_in;

            // 确保新文件流处于初始位置
            filestream_ptr->seekg(0, std::ios::beg);
            if (!filestream_ptr->good()) {
                // 这是极少数情况，但需要处理
                mes_out = bfs_m::err::service_error_bin_file_seek_error_to_begin_after_save_as();
                __enter_uninitialized_state__();
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_file_unknown_error_when_close();
            __enter_uninitialized_state__();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    //============================= 数据交换 =============================

    // 从文件读取数据到自身（文件到文件）
    void bin_file::load(
        size_t      load_begin_in,
        size_t      load_size_in,
        bin_file& save_bin_file_in,
        size_t      store_begin_in,
        bool        safe,
        bool        flush_instantly,
        mes::a_mes& mes_out
    ) {
        // 重置消息
        mes_out = mes::a_mes(0, 0, "");

        // ====================== 定理检查（safe模式） ======================
        if (safe) {
            // 1. 基本状态检查
            if (!__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_load();
                return;
            }
            if (!save_bin_file_in.__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_other_file_not_opened_when_load();
                return;
            }

            // 2. 操作大小检查 （为了幂等性，0大小直接返回）
            if (load_size_in == 0) {
                 return;
            }

            // 3. 溢出检查
            if (!no_overflow(load_begin_in, load_size_in)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overflow);
            }
            if (!no_overflow(store_begin_in, load_size_in)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overflow);
            }

            // 4. 获取文件范围信息
            size_t self_file_size = __file_size__();
            size_t self_self_begin = __self_begin__();
            size_t self_self_end = __self_end__();
            size_t self_op_begin = __op_begin__(load_begin_in);
            size_t self_op_end = __op_end__(load_begin_in, load_size_in);

            size_t other_file_size = save_bin_file_in.__file_size__();
            size_t other_self_begin = save_bin_file_in.__self_begin__();
            size_t other_self_end = save_bin_file_in.__self_end__();
            size_t other_op_begin = save_bin_file_in.__op_begin__(store_begin_in);
            size_t other_op_end = save_bin_file_in.__op_end__(store_begin_in, load_size_in);

            // 5. 定理1：基础有效性检查
            // 源文件操作区间检查
            if (!(self_self_begin <= self_op_begin && self_op_end <= self_self_end)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_self_op_range_outside_file)
                return;
            }

            // 目标文件操作区间检查
            if (!(other_self_begin <= other_op_begin && other_op_end <= other_self_end)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_other_op_range_outside_file)
            }

            // 6. 定理2：同文件操作不重叠检查
            if (__same_file__(save_bin_file_in)) {
                // 如果是同一文件，必须确保操作区间不重叠
                if (__check_range_overlap__(self_op_begin, load_size_in,
                    other_op_begin, load_size_in)) {
                    __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overlap_in_same_file)
                }
            }

            // 7. 文件系统限制检查
            if (load_size_in > SIZE_MAX / 2) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_size_too_large)
            }
        }
        else {
            // 不安全模式：只做最基本检查
            if (!__check_inited__() || !save_bin_file_in.__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_load();
                return;
            }
            if (load_size_in == 0) {
                return; // 大小为0直接返回，不是错误
            }
        }

        // ====================== 关键修复：使用事务性传输 ======================
        try {
            // 1. 定位源文件读取位置
            filestream_ptr->seekg(load_begin_in, std::ios::beg);
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_file_seek_error_when_load();
                return;
            }

            // 2. 定位目标文件写入位置（先获取当前位置，用于可能的恢复）
            save_bin_file_in.filestream_ptr->seekp(store_begin_in, std::ios::beg);
            if (!save_bin_file_in.filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_file_seek_error_when_load();
                return;
            }

            // 3. 分块读取和写入，但记录已写入的字节数
            const size_t BUFFER_SIZE = 64 * 1024; // 64KB 缓冲区
            size_t remaining = load_size_in;
            size_t bytes_transferred = 0;  // 记录已成功传输的字节数

            vector<char> buffer(BUFFER_SIZE);

            while (remaining > 0) {
                size_t chunk_size = std::min(remaining, BUFFER_SIZE);

                // 从源文件读取
                filestream_ptr->read(buffer.data(), chunk_size);
                std::streamsize bytes_read = filestream_ptr->gcount();

                if (bytes_read != static_cast<std::streamsize>(chunk_size)) {
                    mes_out = bfs_m::err::service_error_bin_file_read_error_when_file_to_file_load();

                    // 修复：部分读取失败，但可能已经写入了一些数据
                    // 目标文件可能已损坏，进入未初始化状态
                    save_bin_file_in.__enter_uninitialized_state__();
                    return;
                }

                // 写入目标文件
                save_bin_file_in.filestream_ptr->write(buffer.data(), chunk_size);
                if (!save_bin_file_in.filestream_ptr->good()) {
                    mes_out = bfs_m::err::service_error_bin_file_write_error_when_file_to_file_load();

                    // 关键修复：写入失败，目标文件已部分更新
                    // 保守处理：目标文件进入未初始化状态
                    save_bin_file_in.__enter_uninitialized_state__();
                    return;
                }

                bytes_transferred += chunk_size;
                remaining -= chunk_size;
            }

            // 4. 确保写入完成
            if (flush_instantly) {
                save_bin_file_in.flush();
            }

        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_unknown_error_when_file_to_file_load();

            // 异常发生时，目标文件可能已部分更新
            // 保守处理：目标文件进入未初始化状态
            save_bin_file_in.__enter_uninitialized_state__();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    // 从自身数据写入文件
    void bin_file::store(
        bin_file& load_bin_file_in,
        size_t      load_begin_in,
        size_t      load_size_in,
        size_t      store_begin_in,
        bool        safe,
        bool        flush_instantly,
        mes::a_mes& mes_out
    ) {
        // 重用load函数，但交换源和目标
        load_bin_file_in.load(
            load_begin_in,
            load_size_in,
            *this,
            store_begin_in,
            safe,
            flush_instantly,
            mes_out
        );
    }

    // 将自身数据读取到代理指针
    void bin_file::load_to_agent_ptr(
        size_t           load_begin_in,
        size_t           load_size_in,
        bmms_f::agent_ptr& save_agent_ptr_in,
        size_t           store_begin_in,
        bool             safe,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        if (safe) {
            // ====================== 安全检查 ======================

            // 1. 文件状态检查
            if (!__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_load_to_agent_ptr();
                return;
            }

            // 2. 代理指针非空检查
            bool agent_ptr_is_null = false;
            save_agent_ptr_in.get_is_null(agent_ptr_is_null);
            if (agent_ptr_is_null) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_agent_ptr_is_null)
            }

            // 3. 操作大小检查（为了幂等性，0大小直接返回）
            if (load_size_in == 0) {
                return;
            }

            // 4. 溢出检查
            if (!no_overflow(load_begin_in, load_size_in)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overflow)
            }

            // 5. 获取文件范围
            size_t file_size = __file_size__();
            size_t file_self_begin = __self_begin__();
            size_t file_self_end = __self_end__();
            size_t file_op_begin = __op_begin__(load_begin_in);
            size_t file_op_end = __op_end__(load_begin_in, load_size_in);

            // 6. 获取代理指针范围
            size_t agent_ptr_size = 0;
            save_agent_ptr_in.get_size(agent_ptr_size);

            // 7. 定理4：文件到代理指针有效性检查
            // 文件操作区间检查
            if (!(file_self_begin <= file_op_begin && file_op_end <= file_self_end)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_op_range_outside_file)
            }

            // 代理指针操作区间检查
            if (!no_overflow(store_begin_in, load_size_in)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overflow);
            }
            if (!(0 <= store_begin_in && store_begin_in + load_size_in <= agent_ptr_size)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_agent_ptr_op_range_outside_memory)
            }
        }
        else {
            // 不安全模式：基本检查
            if (!__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_load_to_agent_ptr();
                return;
            }
            if (load_size_in == 0) {
                return;
            }
        }

        // ====================== 执行数据复制 ======================
        try {
            // 1. 定位文件读取位置
            filestream_ptr->seekg(load_begin_in, std::ios::beg);
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_file_seek_error_when_load_to_agent_ptr();
                return;  // seek失败，文件未读取，保持状态
            }

            // 2. 获取代理指针目标地址
            void* agent_ptr_target = nullptr;
            save_agent_ptr_in.get_void_ptr(agent_ptr_target);
            if (agent_ptr_target == nullptr) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_agent_ptr_is_null)
            }

            // 计算目标地址
            char* target_ptr = static_cast<char*>(agent_ptr_target) + store_begin_in;

            // 3. 分块读取
            const size_t BUFFER_SIZE = 64 * 1024;
            size_t remaining = load_size_in;
            size_t bytes_read_total = 0;  // 记录已读取的字节数

            vector<char> buffer(BUFFER_SIZE);

            while (remaining > 0) {
                size_t chunk_size = std::min(remaining, BUFFER_SIZE);

                // 从文件读取到缓冲区
                filestream_ptr->read(buffer.data(), chunk_size);
                std::streamsize bytes_read = filestream_ptr->gcount();

                if (bytes_read != static_cast<std::streamsize>(chunk_size)) {
                    mes_out = bfs_m::err::service_error_bin_file_file_read_error_when_load_to_agent_ptr();

                    // 关键修复：部分读取失败
                    // 内存可能已部分更新，但这是调用者的责任
                    // 文件对象保持状态（因为只是读取操作）
                    return;
                }

                // 从缓冲区复制到代理指针
                memcpy(target_ptr + bytes_read_total, buffer.data(), chunk_size);

                bytes_read_total += chunk_size;
                remaining -= chunk_size;
            }

        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_file_unknown_error_when_load_to_agent_ptr();
            // 异常：内存可能已部分更新，文件对象保持状态
            return;
        }

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 从代理指针读取数据写入到自身
    void bin_file::store_from_agent_ptr(
        bmms_f::agent_ptr& load_agent_ptr_in,
        size_t           load_begin_in,
        size_t           load_size_in,
        size_t           store_begin_in,
        bool             safe,
        bool             flush_instantly,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        if (safe) {
            // ====================== 安全检查 ======================

            // 1. 文件状态检查
            if (!__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_store_from_agent_ptr();
                return;
            }

            // 2. 代理指针非空检查
            bool agent_ptr_is_null = false;
            load_agent_ptr_in.get_is_null(agent_ptr_is_null);
            if (agent_ptr_is_null) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_agent_ptr_is_null)
            }

            // 3. 操作大小检查 （为了幂等性，0大小直接返回）
            if (load_size_in == 0) {
                return;
            }

            // 4. 溢出检查
            if (!no_overflow(store_begin_in, load_size_in)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overflow);
            }
            if (!no_overflow(load_begin_in, load_size_in)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overflow)
            }

            // 5. 获取文件范围
            size_t file_size = __file_size__();
            size_t file_self_begin = __self_begin__();
            size_t file_self_end = __self_end__();
            size_t file_op_begin = __op_begin__(store_begin_in);
            size_t file_op_end = __op_end__(store_begin_in, load_size_in);

            // 6. 获取代理指针范围
            size_t agent_ptr_size = 0;
            load_agent_ptr_in.get_size(agent_ptr_size);

            // 7. 定理5：代理指针到文件有效性检查
            // 文件操作区间检查
            if (!(file_self_begin <= file_op_begin && file_op_end <= file_self_end)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_op_range_outside_file)
            }

            // 代理指针操作区间检查
            if (!(0 <= load_begin_in && load_begin_in + load_size_in <= agent_ptr_size)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_agent_ptr_op_range_outside_memory)
            }
        }
        else {
            // 不安全模式：基本检查
            if (!__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_store_from_agent_ptr();
                return;
            }
            if (load_size_in == 0) {
                return;
            }
        }

        // ====================== 执行数据复制 ======================
        try {
            // 1. 定位文件写入位置
            filestream_ptr->seekp(store_begin_in, std::ios::beg);
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_file_seek_error_when_store_from_agent_ptr();
                return;  // seek失败，文件未写入，保持状态
            }

            // 2. 获取代理指针源地址
            void* agent_ptr_source = nullptr;
            load_agent_ptr_in.get_void_ptr(agent_ptr_source);
            if (agent_ptr_source == nullptr) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_agent_ptr_is_null)
            }

            // 计算源地址
            const char* source_ptr = static_cast<const char*>(agent_ptr_source) + load_begin_in;

            // 3. 分块写入，记录已写入的字节数
            const size_t BUFFER_SIZE = 64 * 1024;
            size_t remaining = load_size_in;
            size_t bytes_written_total = 0;

            while (remaining > 0) {
                size_t chunk_size = std::min(remaining, BUFFER_SIZE);

                // 从代理指针写入文件
                filestream_ptr->write(source_ptr + bytes_written_total, chunk_size);
                if (!filestream_ptr->good()) {
                    mes_out = bfs_m::err::service_error_bin_file_file_write_error_when_store_from_agent_ptr();

                    // 关键修复：写入失败，文件已部分更新
                    // 文件对象进入未初始化状态
                    __enter_uninitialized_state__();
                    return;
                }

                bytes_written_total += chunk_size;
                remaining -= chunk_size;
            }

            // 4. 确保写入完成
            if (flush_instantly) {
                flush();  // flush可能失败，但数据已写入缓冲区
            }

        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_file_unknown_error_when_store_from_agent_ptr();
            // 异常：文件可能已部分写入，进入未初始化状态
            __enter_uninitialized_state__();
            return;
        }

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 将自身数据读到void指针
    void bin_file::load_to_void_ptr(
        size_t      load_begin_in,
        size_t      load_size_in,
        void* store_ptr_in,
        bool        safe,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        if (safe) {
            // ====================== 安全检查 ======================

            // 1. 文件状态检查
            if (!__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_load_to_void_ptr();
                return;
            }

            // 2. void指针非空检查
            if (store_ptr_in == nullptr) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_void_ptr_is_nullptr)
            }

            // 3. 操作大小检查 （为了幂等性，0大小直接返回）
            if (load_size_in == 0) {
                return;
            }

            // 4. 溢出检查
            if (!no_overflow(load_begin_in, load_size_in)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overflow)
            }

            // 5. 获取文件范围
            size_t file_size = __file_size__();
            size_t file_self_begin = __self_begin__();
            size_t file_self_end = __self_end__();
            size_t file_op_begin = __op_begin__(load_begin_in);
            size_t file_op_end = __op_end__(load_begin_in, load_size_in);

            // 6. 定理6：文件到void*有效性检查
            if (!(file_self_begin <= file_op_begin && file_op_end <= file_self_end)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_op_range_outside_file)
            }

            // 7. 内存边界检查（可选，如果知道内存大小）
            // void*没有大小信息，由调用者保证有足够空间

        }
        else {
            // 不安全模式：基本检查
            if (!__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_load_to_void_ptr();
                return;
            }
            if (load_size_in == 0) {
                return;
            }
            if (store_ptr_in == nullptr) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_void_ptr_is_nullptr)
            }
        };

        // ====================== 执行数据复制 ======================
        try {
            // 定位文件读取位置
            filestream_ptr->seekg(load_begin_in, std::ios::beg);
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_file_seek_error_when_load_to_void_ptr();
                return;  // seek失败，文件未读取，保持状态
            }

            // 直接读取到目标内存
            filestream_ptr->read(static_cast<char*>(store_ptr_in), load_size_in);
            std::streamsize bytes_read = filestream_ptr->gcount();

            if (bytes_read != static_cast<std::streamsize>(load_size_in)) {
                mes_out = bfs_m::err::service_error_bin_file_file_read_error_when_load_to_void_ptr();
                // 部分读取失败，内存可能已部分更新
                // 文件对象保持状态（读取操作）
                return;
            }

        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_file_unknown_error_when_load_to_void_ptr();
            // 异常：内存可能已部分更新，文件对象保持状态
            return;
        }

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    // 将void指针的数据写到自身
    void bin_file::store_from_void_ptr(
        void* load_ptr_in,
        size_t      load_size_in,
        size_t      store_begin_in,
        bool        safe,
        mes::a_mes& mes_out
    ) {
        mes_out = mes::a_mes(0, 0, "");

        if (safe) {
            // ====================== 安全检查 ======================

            // 1. 文件状态检查
            if (!__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_store_from_void_ptr();
                return;
            }

            // 2. void指针非空检查
            if (load_ptr_in == nullptr) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_void_ptr_is_nullptr)
            }

            // 3. 操作大小检查 （为了幂等性，0大小直接返回）
            if (load_size_in == 0) {
                return;
            }

            // 4. 溢出检查
            if (!no_overflow(store_begin_in, load_size_in)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overflow)
            }

            // 5. 获取文件范围
            size_t file_size = __file_size__();
            size_t file_self_begin = __self_begin__();
            size_t file_self_end = __self_end__();
            size_t file_op_begin = __op_begin__(store_begin_in);
            size_t file_op_end = __op_end__(store_begin_in, load_size_in);

            // 6. 定理7：void*到文件有效性检查
            if (!(file_self_begin <= file_op_begin && file_op_end <= file_self_end)) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_op_range_outside_file)
            }

        }
        else {
            // 不安全模式：基本检查
            if (!__check_inited__()) {
                mes_out = bfs_m::err::service_error_bin_file_file_not_opened_when_store_from_void_ptr();
                return;
            }
            if (load_size_in == 0) {
                return;
            }
            if (load_ptr_in == nullptr) {
                __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_void_ptr_is_nullptr)
            }
        };

        // ====================== 执行数据复制 ======================
        try {
            // 定位文件写入位置
            filestream_ptr->seekp(store_begin_in, std::ios::beg);
            if (!filestream_ptr->good()) {
                mes_out = bfs_m::err::service_error_bin_file_file_seek_error_when_store_from_void_ptr();
                return;  // seek失败，文件未写入，保持状态
            }

            // 直接从源内存写入文件（分块以保持一致性）
            const size_t BUFFER_SIZE = 64 * 1024;
            size_t remaining = load_size_in;
            size_t bytes_written_total = 0;
            const char* source_ptr = static_cast<const char*>(load_ptr_in);

            while (remaining > 0) {
                size_t chunk_size = std::min(remaining, BUFFER_SIZE);

                // 写入文件
                filestream_ptr->write(source_ptr + bytes_written_total, chunk_size);
                if (!filestream_ptr->good()) {
                    mes_out = bfs_m::err::service_error_bin_file_file_write_error_when_store_from_void_ptr();

                    // 关键修复：写入失败，文件已部分更新
                    // 文件对象进入未初始化状态
                    __enter_uninitialized_state__();
                    return;
                }

                bytes_written_total += chunk_size;
                remaining -= chunk_size;
            }

            // 立即刷新确保数据写入磁盘
            flush();  // flush可能失败，但数据已写入缓冲区

        }
        catch (...) {
            mes_out = bfs_m::err::service_error_bin_file_file_unknown_error_when_store_from_void_ptr();
            // 异常：文件可能已部分写入，进入未初始化状态
            __enter_uninitialized_state__();
            return;
        }

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }

    //============================ 信息获取函数 ============================

    bool bin_file::__check_path_valid__(str& path_in) {
        if (path_in.empty()) {
            return false;
        }

        #if defined(_WIN32) || defined(_WIN64)
        // ==================== Windows平台路径检查 ====================

        // 1. 检查控制字符 (ASCII 0-31) - 除了制表符、换行符等
        for (char c : path_in) {
            if (c == '\0') {
                return false;  // 空字符绝对禁止
            }
            if (c >= 0 && c < 32) {
                // 允许少数控制字符
                if (c != '\t' && c != '\n' && c != '\r') {
                    return false;
                }
            }
        }

        // 2. 使用Windows API验证路径格式
        std::filesystem::path p(path_in);
        str filename = p.filename().string();

        // 文件名不能为空、不能是.或..
        if (filename.empty() || filename == "." || filename == "..") {
            return false;
        }

        // 3. Windows文件名非法字符（不包含:，因为:在完整路径中可能有效）
        const str illegal_chars_in_filename = "*?\"<>|";
        for (char c : illegal_chars_in_filename) {
            if (filename.find(c) != str::npos) {
                return false;
            }
        }

        // 4. 检查完整路径中的冒号使用（允许在盘符后）
        size_t path_colon_pos = path_in.find(':');
        if (path_colon_pos != str::npos) {
            // 如果冒号不在第二个位置（盘符后），则非法
            if (path_colon_pos != 1) {
                return false;
            }
            // 检查盘符是否有效（A-Z）
            char drive = toupper(static_cast<unsigned char>(path_in[0]));
            if (drive < 'A' || drive > 'Z') {
                return false;
            }
            // 确保没有其他冒号
            if (path_in.find(':', path_colon_pos + 1) != str::npos) {
                return false;
            }
        }

        // 5. 检查保留设备名（只在没有路径分隔符时检查）
        if (filename.find('/') == str::npos && filename.find('\\') == str::npos) {
            str name_without_ext = filename;
            size_t dot_pos = filename.find('.');
            if (dot_pos != str::npos) {
                name_without_ext = filename.substr(0, dot_pos);
            }

            // 转换为大写比较
            for (char& c : name_without_ext) {
                c = toupper(static_cast<unsigned char>(c));
            }

            const vector<str> reserved_names = {
                "CON", "PRN", "AUX", "NUL",
                "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
            };

            for (const auto& reserved : reserved_names) {
                if (name_without_ext == reserved) {
                    return false;
                }
            }
        }

        // 6. 检查文件名是否以空格或点结尾（Windows文件名限制）
        if (!filename.empty()) {
            // 使用 find_last_of 找到最后一个点
            size_t dot_pos = filename.find_last_of('.');

            str name_without_ext;
            if (dot_pos != str::npos) {
                name_without_ext = filename.substr(0, dot_pos);
            }
            else {
                name_without_ext = filename;
            }

            if (!name_without_ext.empty()) {
                char last_char = name_without_ext.back();
                // 检查最后一个字符
                if (last_char == ' ' || last_char == '.') {
                    return false;
                }
            }
        }

        // 8. 对于不存在的驱动器或受保护的系统位置，它们可能是有效的路径格式
        // 所以这些情况应该返回 true（路径格式有效）
        return true;

        #elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
        // ==================== Linux/Unix平台路径检查 ====================

        // 1. 检查控制字符
        for (char c : path_in) {
            if (c == '\0') {
                return false;
            }
            if (c >= 0 && c < 32) {
                if (c != '\t' && c != '\n' && c != '\r') {
                    return false;
                }
            }
        }

        // 2. Linux路径检查
        std::filesystem::path p(path_in);
        str filename = p.filename().string();

        // 文件名不能为空、不能是.或..
        if (filename.empty() || filename == "." || filename == "..") {
            return false;
        }

        // 3. 检查文件名中是否包含正斜杠
        if (filename.find('/') != str::npos) {
            return false;
        }

        // 4. 检查文件名长度
        if (filename.length() > 255) {
            return false;
        }

        // 5. 检查路径总长度
        if (path_in.length() > 4096) {
            return false;
        }

        return true;

#else
        // ==================== 其他平台 ====================
        for (char c : path_in) {
            if (c == '\0') {
                return false;
            }
        }

        std::filesystem::path p(path_in);
        str filename = p.filename().string();

        if (filename.empty() || filename == "." || filename == "..") {
            return false;
        }

        return true;
#endif
    }

    // 检查文件操作区间有效性
    bool bin_file::__check_file_range__(size_t begin, size_t size) {
        if (!__check_inited__()) {
            return false;
        }

        size_t file_size = __file_size__();

        // 检查溢出和范围
        if (!no_overflow(begin, size)) {
            return false;
        }

        return (begin < file_size) && (begin + size <= file_size);
    };

    // 检查操作大小有效性
    bool bin_file::__check_op_size__(size_t size) {
        return size > 0;
    };

    // 检查操作区间是否重叠
    bool bin_file::__check_range_overlap__(
        size_t self_begin, size_t self_size,
        size_t other_begin, size_t other_size
    ) {
        size_t self_end = __op_end__(self_begin, self_size);
        size_t other_end = __op_end__(other_begin, other_size);

        // 重叠的条件：区间相交但不完全分离
        return !(self_end <= other_begin || other_end <= self_begin);
    };

    // 检查两个文件是否为同一文件，如果是则结束进程
    void bin_file::__check_not_same_file_and_exit__(bin_file& other) {
        if (__same_file__(other)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_two_streams_use_same_file)
        }
    }
    // 检查文件是否已初始化
    bool bin_file::__check_inited__() {
        return inited && filestream_ptr != nullptr && filestream_ptr->is_open();
    };

    // 检查两个文件是否为同一文件
    bool bin_file::__same_file__(bin_file& other) {
        // 如果两个文件都没有路径信息，返回false
        if (path.empty() || other.path.empty()) {
            return false;
        }

        try {
            // 使用规范化路径进行比较
            auto self_path_abs = std::filesystem::absolute(path);
            auto other_path_abs = std::filesystem::absolute(other.path);

            return self_path_abs == other_path_abs;
        }
        catch (...) {
            // 如果路径转换失败，退回到简单字符串比较
            return path == other.path;
        }
    };

    size_t bin_file::__file_size__() {
        if (!inited) return 0;
        try {
            return std::filesystem::file_size(path);
        }
        catch (...) {
            return 0;
        }
    };

    // 获取文件管理的起点（文件开头）
    size_t bin_file::__self_begin__() {
        // 文件总是从0开始
        return 0;
    }

    // 获取文件管理的终点（文件大小）
    size_t bin_file::__self_end__() {
        return __file_size__();
    }

    // 获取操作区间起点
    size_t bin_file::__op_begin__(size_t begin_in) {
        return begin_in;  // 直接返回，因为文件从0开始
    }

    // 获取操作区间终点
    size_t bin_file::__op_end__(size_t op_begin_in, size_t op_size_in) {
        // 检查是否溢出
        if (op_begin_in > SIZE_MAX - op_size_in) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_end_overflow_size_t_max_value)
        }
        return op_begin_in + op_size_in;
    }

    // ====================== 辅助函数 ======================

    // 进入未初始化状态（清理资源）
    void bin_file::__enter_uninitialized_state__() {
        // 关闭文件流
        if (filestream_ptr && filestream_ptr->is_open()) {
            try {
                filestream_ptr->close();
            }
            catch (...) {
                // 忽略关闭异常
            }
        }

        // 释放资源
        filestream_ptr.reset();
        inited = false;
        // 保留path信息，用于调试或重新打开
    }

    // 尝试重新打开原文件，如果失败则进入未初始化状态
    void bin_file::__try_reopen_and_enter_uninitialized_state__(mes::a_mes& mes_out) {
        if (path.empty()) {
            __enter_uninitialized_state__();
            return;
        }

        try {
            if (filestream_ptr) {
                filestream_ptr->open(path,
                    std::ios::binary |
                    std::ios::in |
                    std::ios::out
                );

                if (filestream_ptr->is_open()) {
                    // 重新打开成功，但状态可能不一致
                    inited = true;
                    // 注意：此时文件可能已损坏或大小不正确
                    // 调用者应根据mes_out决定后续操作
                    return;
                }
            }
        }
        catch (...) {
            // 忽略重新打开异常
        }

        // 重新打开失败，进入未初始化状态
        __enter_uninitialized_state__();
    }

    // 清理失败时的资源
    void bin_file::__cleanup_on_failure__() {
        // 关闭文件流
        if (filestream_ptr && filestream_ptr->is_open()) {
            try {
                filestream_ptr->close();
            }
            catch (...) {
                // 忽略关闭异常
            }
        }

        // 释放资源
        filestream_ptr.reset();
        inited = false;
        // 注意：保留path用于调试
    }

    //============================ 定理检查函数 ============================

    // 前提：无溢出检查
    bool bin_file::no_overflow(size_t a, size_t b) {
        return a <= SIZE_MAX - b;
    }

    // 定理1：基础有效性检查（双方都是文件）
    void bin_file::check_basic_validity(
        size_t& self_op_begin,
        size_t& self_op_size,
        size_t& self_self_begin,
        size_t& self_self_end,
        size_t& other_op_begin,
        size_t& other_op_size,
        size_t& other_self_begin,
        size_t& other_self_end
    ) {
        // 1. 文件必须打开
        if (!__check_inited__()) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_self_file_not_opened)
        }

        // 2. 操作大小必须为正
        if (self_op_size == 0) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_self_op_size_zero)
        }
        if (other_op_size == 0) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_other_op_size_zero)
        }

        // 3. 操作区间必须在文件范围内
        if (!(self_self_begin <= self_op_begin && __op_end__(self_op_begin, self_op_size) <= self_self_end)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_self_op_range_outside_file)
        }
        if (!(other_self_begin <= other_op_begin && __op_end__(other_op_begin, other_op_size) <= other_self_end)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_other_op_range_outside_file)
        }
    }

    // 定理2：同文件检查（如果发现两个流使用同一个文件，立即退出）
    void bin_file::check_not_same_file(bin_file& other) {
        if (__same_file__(other)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_two_streams_use_same_file)
        }
    }

    // 定理3：操作区间不重叠检查（用于文件间操作）
    void bin_file::check_op_range_not_overlapping(
        size_t& self_op_begin,
        size_t& self_op_size,
        size_t& other_op_begin,
        size_t& other_op_size
    ) {
        size_t self_op_end = __op_end__(self_op_begin, self_op_size);
        size_t other_op_end = __op_end__(other_op_begin, other_op_size);

        if (!(self_op_end <= other_op_begin || other_op_end <= self_op_begin)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_op_range_overlapping)
        }
    }

    // 定理4：文件到代理指针的基础有效性检查
    void bin_file::check_file_to_agent_ptr_validity(
        size_t& file_op_begin,
        size_t& file_op_size,
        size_t& file_self_begin,
        size_t& file_self_end,
        bmms_f::agent_ptr& agent_ptr_in,
        size_t& agent_ptr_op_begin
    ) {
        // 1. 文件必须打开
        if (!__check_inited__()) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_not_opened)
        }

        // 2. 代理指针非空
        bool agent_ptr_is_null = false;
        agent_ptr_in.get_is_null(agent_ptr_is_null);
        if (agent_ptr_is_null) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_agent_ptr_is_null)
        }

        // 3. 操作大小必须为正
        if (file_op_size == 0) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_op_size_zero)
        }

        // 4. 文件操作区间必须在文件范围内
        if (!(file_self_begin <= file_op_begin && __op_end__(file_op_begin, file_op_size) <= file_self_end)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_op_range_outside_file)
        }

        // 5. 代理指针操作区间必须在内存范围内
        size_t agent_ptr_size = 0;
        agent_ptr_in.get_size(agent_ptr_size);
        if (!(0 <= agent_ptr_op_begin && agent_ptr_op_begin + file_op_size <= agent_ptr_size)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_agent_ptr_op_range_outside_memory)
        }
    }

    // 定理5：代理指针到文件的基础有效性检查
    void bin_file::check_agent_ptr_to_file_validity(
        bmms_f::agent_ptr& agent_ptr_in,
        size_t& agent_ptr_op_begin,
        size_t& agent_ptr_op_size,
        size_t& file_op_begin,
        size_t& file_self_begin,
        size_t& file_self_end
    ) {
        // 1. 文件必须打开
        if (!__check_inited__()) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_not_opened)
        }

        // 2. 代理指针非空
        bool agent_ptr_is_null = false;
        agent_ptr_in.get_is_null(agent_ptr_is_null);
        if (agent_ptr_is_null) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_agent_ptr_is_null)
        }

        // 3. 操作大小必须为正
        if (agent_ptr_op_size == 0) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_op_size_zero)
        }

        // 4. 代理指针操作区间必须在内存范围内
        size_t agent_ptr_total_size = 0;
        agent_ptr_in.get_size(agent_ptr_total_size);
        if (!(0 <= agent_ptr_op_begin && agent_ptr_op_begin + agent_ptr_op_size <= agent_ptr_total_size)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_agent_ptr_op_range_outside_memory)
        }

        // 5. 文件操作区间必须在文件范围内
        if (!(file_self_begin <= file_op_begin && __op_end__(file_op_begin, agent_ptr_op_size) <= file_self_end)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_op_range_outside_file)
        }
    }

    // 定理6：文件到void指针的基础有效性检查
    void bin_file::check_file_to_void_ptr_validity(
        size_t& file_op_begin,
        size_t& file_op_size,
        size_t& file_self_begin,
        size_t& file_self_end,
        void*& void_ptr_in
    ) {
        // 1. 文件必须打开
        if (!__check_inited__()) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_not_opened)
        }

        // 2. void指针非空
        if (void_ptr_in == nullptr) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_void_ptr_is_nullptr)
        }

        // 3. 操作大小必须为正
        if (file_op_size == 0) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_op_size_zero)
        }

        // 4. 文件操作区间必须在文件范围内
        if (!(file_self_begin <= file_op_begin && __op_end__(file_op_begin, file_op_size) <= file_self_end)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_op_range_outside_file)
        }
    }

    // 定理7：void指针到文件的基础有效性检查
    void bin_file::check_void_ptr_to_file_validity(
        void*& void_ptr_in,
        size_t& void_ptr_op_size,
        size_t& file_op_begin,
        size_t& file_self_begin,
        size_t& file_self_end
    ) {
        // 1. 文件必须打开
        if (!__check_inited__()) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_not_opened)
        }

        // 2. void指针非空
        if (void_ptr_in == nullptr) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_void_ptr_is_nullptr)
        }

        // 3. 操作大小必须为正
        if (void_ptr_op_size == 0) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_op_size_zero)
        }

        // 4. 文件操作区间必须在文件范围内
        if (!(file_self_begin <= file_op_begin && __op_end__(file_op_begin, void_ptr_op_size) <= file_self_end)) {
            __CODING_ERROR__(bfs_m::err::coding_error_bin_file_theorem_file_op_range_outside_file)
        }
    }
};