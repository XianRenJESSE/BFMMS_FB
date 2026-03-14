// file_file_control.cpp

module BFS;

#include "BFS_dp.h"

namespace bfs_f::dir_control {

    // 内部宏用于处理编码错误
    #if defined(BFS_INCLUDE_TEST)
    #define __CODING_ERROR__(mes)\
                mes().out(); \
                throw mes();
    #else
    #define __CODING_ERROR__(mes)\
                mes().out();std::exit(EXIT_FAILURE);
    #endif

    // 创建目录（递归创建）
    void add_dir(
        str path,
        mes::a_mes& mes_out
    ) {
        // 重置消息
        mes_out = mes::a_mes(0, 0, "");

        // ====================== 定理1：基础有效性检查 ======================
        if (path.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        if (!__check_path_valid__(path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }

        // 检查是否只包含空白字符
        bool all_whitespace = true;
        for (char c : path) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                all_whitespace = false;
                break;
            }
        }
        if (all_whitespace) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        // ====================== 步骤1：规范化路径 ======================
        f_s::path fs_path;
        try {
            fs_path = f_s::path(path);

            // 检查路径是否有效（不是空路径且不包含非法字符）
            if (fs_path.empty()) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_empty();
                return;
            }
        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_invalid_path();
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤2：检查路径是否已存在 ======================
        try {
            if (f_s::exists(fs_path)) {
                // 如果已存在，检查是否为目录
                if (f_s::is_directory(fs_path)) {
                    // 已经是目录，返回成功（幂等操作）
                    mes_out = mes::a_mes(0, 0, "");
                    return;
                }
                else {
                    // 存在但不是目录
                    mes_out = bfs_m::err::service_error_dir_control_path_exists_but_not_dir();
                    return;
                }
            }
        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_check_existence_failed();
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤3：递归创建目录 ======================
        try {
            // 使用create_directories，它会递归创建所有不存在的父目录
            bool created = f_s::create_directories(fs_path);

            if (!created) {
                // 理论上不应该发生，因为我们已经检查过不存在
                // 但可能由于竞争条件或其他进程创建了目录
                if (!f_s::exists(fs_path)) {
                    mes_out = bfs_m::err::service_error_service_error_dir_control_create_failed_but_no_error();
                    return;
                }
                // 如果现在存在了，再次检查是否为目录
                if (!f_s::is_directory(fs_path)) {
                    mes_out = bfs_m::err::service_error_dir_control_path_exists_but_not_dir();
                    return;
                }
                // 如果现在存在且是目录，返回成功
            }

            // ====================== 步骤4：验证创建结果 ======================
            if (!f_s::exists(fs_path)) {
                // 创建了但立即不存在（极不可能）
                mes_out = bfs_m::err::service_error_service_error_dir_control_create_failed_but_no_error();
                return;
            }

            if (!f_s::is_directory(fs_path)) {
                // 创建的不是目录（不应该发生）
                mes_out = bfs_m::err::service_error_dir_control_path_exists_but_not_dir();
                return;
            }

            // 检查是否有权限访问
            try {
                auto status = f_s::status(fs_path);
                if ((status.permissions() & f_s::perms::owner_read) == f_s::perms::none) {
                    // 创建成功但可能无法读取
                    mes_out = bfs_m::err::service_error_dir_control_created_but_no_read_permission();
                    return;
                }
            }
            catch (...) {
                // 忽略权限检查异常，目录已创建成功
            }

        }
        catch (const f_s::filesystem_error& e) {
            // 分析错误类型
            if (e.code().value() == EACCES || e.code().value() == EPERM) {
                mes_out = bfs_m::err::service_error_dir_control_permission_denied();
            }
            else if (e.code().value() == ENOSPC) {
                mes_out = bfs_m::err::service_error_dir_control_no_space();
            }
            else if (e.code().value() == ENAMETOOLONG) {
                mes_out = bfs_m::err::service_error_dir_control_name_too_long();
            }
            else if (e.code().value() == ENOENT) {
                // 父目录不存在（不应该发生，因为create_directories会创建父目录）
                mes_out = bfs_m::err::service_error_dir_control_parent_not_exist();
            }
            else {
                mes_out = bfs_m::err::service_error_dir_control_create_failed();
            }
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    // 删除文件或目录（递归删除目录）
    void del(
        str path,
        mes::a_mes& mes_out
    ) {
        // 重置消息
        mes_out = mes::a_mes(0, 0, "");

        // ====================== 定理1：基础有效性检查 ======================
        if (path.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        if (!__check_path_valid__(path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }

        // 检查是否只包含空白字符
        bool all_whitespace = true;
        for (char c : path) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                all_whitespace = false;
                break;
            }
        }
        if (all_whitespace) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        // ====================== 步骤1：规范化路径 ======================
        f_s::path fs_path;
        try {
            fs_path = f_s::path(path);

            if (fs_path.empty()) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_empty();
                return;
            }

            // 检查是否是根目录（禁止删除根目录）
            if (fs_path.root_name().empty() && fs_path.root_directory().empty()) {
                // 这是相对路径，需要转换为绝对路径检查
                fs_path = f_s::absolute(fs_path);
            }

            if (fs_path.root_path() == fs_path) {
                mes_out = bfs_m::err::service_error_dir_control_cannot_delete_root();
                return;
            }

        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_invalid_path();
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤2：检查路径是否存在 ======================
        bool exists = false;
        try {
            exists = f_s::exists(fs_path);
        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_check_existence_failed();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        if (!exists) {
            // 路径不存在，返回成功（幂等操作）
            mes_out = mes::a_mes(0, 0, "");
            return;
        }

        // ====================== 步骤3：安全检查 ======================
        try {
            // 检查是否是符号链接
            if (f_s::is_symlink(fs_path)) {
                // 删除符号链接本身，不跟随链接
                f_s::remove(fs_path);
                mes_out = mes::a_mes(0, 0, "");
                return;
            }

            // 检查是否是目录
            bool is_dir = f_s::is_directory(fs_path);

            // 检查是否有写权限
            auto perms = f_s::status(fs_path).permissions();
            if ((perms & f_s::perms::owner_write) == f_s::perms::none) {
                mes_out = bfs_m::err::service_error_dir_control_no_write_permission();
                return;
            }

            // 对于目录，检查是否为空（可选，但提供更安全的操作）
            if (is_dir) {
                try {
                    bool is_empty = f_s::is_empty(fs_path);
                    // 我们不强制要求目录为空，remove_all会递归删除
                }
                catch (...) {
                    // 忽略检查目录是否为空时的错误
                }
            }

        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_access_denied();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤4：执行删除 ======================
        try {
            // 使用remove_all递归删除目录及其内容
            std::uintmax_t count = 0;

            try {
                count = f_s::remove_all(fs_path);
            }
            catch (const f_s::filesystem_error& e) {
                if (e.code().value() == EBUSY) {
                    mes_out = bfs_m::err::service_error_dir_control_resource_busy();
                    return;
                }
                throw; // 重新抛出其他异常
            }

            if (count == 0) {
                // 没有删除任何东西（不应该发生，因为我们检查了存在性）
                mes_out = bfs_m::err::service_error_dir_control_delete_failed_but_no_error();
                return;
            }

        }
        catch (const f_s::filesystem_error& e) {
            // 分析错误类型
            if (e.code().value() == EACCES || e.code().value() == EPERM) {
                mes_out = bfs_m::err::service_error_dir_control_permission_denied();
            }
            else if (e.code().value() == EBUSY) {
                mes_out = bfs_m::err::service_error_dir_control_resource_busy();
            }
            else if (e.code().value() == ENOENT) {
                // 在删除过程中被其他进程删除（竞争条件）
                mes_out = mes::a_mes(0, 0, ""); // 视为成功
                return;
            }
            else {
                mes_out = bfs_m::err::service_error_dir_control_delete_failed();
            }
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤5：验证删除结果 ======================
        try {
            // 短暂延迟后检查
            // 注意：这不能完全保证，因为其他进程可能立即创建同名文件
            // 但可以检测明显的删除失败
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            if (f_s::exists(fs_path)) {
                mes_out = bfs_m::err::service_error_dir_control_delete_failed_but_no_error();
                return;
            }
        }
        catch (...) {
            // 忽略验证异常，假设删除成功
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    // 获取目录列表（只获取直接子目录）
    void get_dir_list(
        str path,
        vector<str>& dir_list_path_out,
        mes::a_mes& mes_out
    ) {
        // 重置消息和输出
        mes_out = mes::a_mes(0, 0, "");
        dir_list_path_out.clear();

        // ====================== 定理1：基础有效性检查 ======================
        if (path.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        if (!__check_path_valid__(path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }

        // 检查是否只包含空白字符
        bool all_whitespace = true;
        for (char c : path) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                all_whitespace = false;
                break;
            }
        }
        if (all_whitespace) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        // ====================== 步骤1：规范化路径 ======================
        f_s::path fs_path;
        try {
            fs_path = f_s::path(path);

            if (fs_path.empty()) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_empty();
                return;
            }
        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_invalid_path();
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤2：检查路径是否存在且是目录 ======================
        try {
            if (!f_s::exists(fs_path)) {
                mes_out = bfs_m::err::service_error_dir_control_path_not_exist();
                return;
            }

            if (!f_s::is_directory(fs_path)) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_not_dir();
                return;
            }

            // 检查读取权限
            auto perms = f_s::status(fs_path).permissions();
            if ((perms & f_s::perms::owner_read) == f_s::perms::none) {
                mes_out = bfs_m::err::service_error_dir_control_no_read_permission();
                return;
            }

        }
        catch (const f_s::filesystem_error& e) {
            if (e.code().value() == EACCES || e.code().value() == EPERM) {
                mes_out = bfs_m::err::service_error_dir_control_no_read_permission();
            }
            else {
                mes_out = bfs_m::err::service_error_dir_control_access_failed();
            }
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤3：遍历目录 ======================
        try {
            // 使用directory_iterator（不递归）
            for (const auto& entry : f_s::directory_iterator(fs_path)) {
                try {
                    // 只添加目录，忽略文件和其他类型
                    if (f_s::is_directory(entry.status())) {
                        // 获取规范化的绝对路径
                        f_s::path canonical_path;
                        try {
                            canonical_path = f_s::canonical(entry.path());
                            dir_list_path_out.push_back(canonical_path.string());
                        }
                        catch (...) {
                            // 如果无法获取规范路径，使用相对路径
                            dir_list_path_out.push_back(entry.path().string());
                        }
                    }
                }
                catch (f_s::filesystem_error) {
                    // 忽略无法访问的条目，继续处理其他条目
                    continue;
                }
                catch (...) {
                    // 忽略其他异常，继续处理
                    continue;
                }
            }

            // 按字母顺序排序（可选）
            std::sort(dir_list_path_out.begin(), dir_list_path_out.end());

        }
        catch (const f_s::filesystem_error& e) {
            if (e.code().value() == EACCES || e.code().value() == EPERM) {
                mes_out = bfs_m::err::service_error_dir_control_no_read_permission();
            }
            else if (e.code().value() == ENOTDIR) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_not_dir();
            }
            else {
                mes_out = bfs_m::err::service_error_dir_control_list_failed();
            }
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    // 获取文件列表（只获取直接子文件，不包括目录）
    void get_list_file(
        str path,
        vector<str>& file_list_path_out,
        mes::a_mes& mes_out
    ) {
        // 重置消息和输出
        mes_out = mes::a_mes(0, 0, "");
        file_list_path_out.clear();

        // ====================== 定理1：基础有效性检查 ======================
        if (path.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        if (!__check_path_valid__(path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }

        if (!__check_path_valid__(path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }

        // 检查是否只包含空白字符
        bool all_whitespace = true;
        for (char c : path) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                all_whitespace = false;
                break;
            }
        }
        if (all_whitespace) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        // ====================== 步骤1：规范化路径 ======================
        f_s::path fs_path;
        try {
            fs_path = f_s::path(path);

            if (fs_path.empty()) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_empty();
                return;
            }
        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_invalid_path();
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤2：检查路径是否存在且是目录 ======================
        try {
            if (!f_s::exists(fs_path)) {
                mes_out = bfs_m::err::service_error_dir_control_path_not_exist();
                return;
            }

            if (!f_s::is_directory(fs_path)) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_not_dir();
                return;
            }

            // 检查读取权限
            auto perms = f_s::status(fs_path).permissions();
            if ((perms & f_s::perms::owner_read) == f_s::perms::none) {
                mes_out = bfs_m::err::service_error_dir_control_no_read_permission();
                return;
            }

        }
        catch (const f_s::filesystem_error& e) {
            if (e.code().value() == EACCES || e.code().value() == EPERM) {
                mes_out = bfs_m::err::service_error_dir_control_no_read_permission();
            }
            else {
                mes_out = bfs_m::err::service_error_dir_control_access_failed();
            }
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤3：遍历目录 ======================
        try {
            // 使用directory_iterator（不递归）
            for (const auto& entry : f_s::directory_iterator(fs_path)) {
                try {
                    // 只添加普通文件，忽略目录和其他类型
                    if (f_s::is_regular_file(entry.status())) {
                        // 获取规范化的绝对路径
                        f_s::path canonical_path;
                        try {
                            canonical_path = f_s::canonical(entry.path());
                            file_list_path_out.push_back(canonical_path.string());
                        }
                        catch (...) {
                            // 如果无法获取规范路径，使用相对路径
                            file_list_path_out.push_back(entry.path().string());
                        }
                    }
                }
                catch (f_s::filesystem_error) {
                    // 忽略无法访问的条目，继续处理其他条目
                    continue;
                }
                catch (...) {
                    // 忽略其他异常，继续处理
                    continue;
                }
            }

            // 按字母顺序排序（可选）
            std::sort(file_list_path_out.begin(), file_list_path_out.end());

        }
        catch (const f_s::filesystem_error& e) {
            if (e.code().value() == EACCES || e.code().value() == EPERM) {
                mes_out = bfs_m::err::service_error_dir_control_no_read_permission();
            }
            else if (e.code().value() == ENOTDIR) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_not_dir();
            }
            else {
                mes_out = bfs_m::err::service_error_dir_control_list_failed();
            }
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    // 检查路径是否存在
    void get_path_is_excited(
        str path,
        bool& is_excited_out,
        mes::a_mes& mes_out
    ) {
        // 重置消息和输出
        mes_out = mes::a_mes(0, 0, "");
        is_excited_out = false;

        // ====================== 定理1：基础有效性检查 ======================
        if (path.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        if (!__check_path_valid__(path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }

        // 检查是否只包含空白字符
        bool all_whitespace = true;
        for (char c : path) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                all_whitespace = false;
                break;
            }
        }
        if (all_whitespace) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        // ====================== 步骤1：规范化路径 ======================
        f_s::path fs_path;
        try {
            fs_path = f_s::path(path);

            if (fs_path.empty()) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_empty();
                return;
            }
        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_invalid_path();
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤2：检查存在性 ======================
        try {
            is_excited_out = f_s::exists(fs_path);
        }
        catch (const f_s::filesystem_error& e) {
            // 检查过程中出错，通常意味着路径不可访问
            if (e.code().value() == EACCES || e.code().value() == EPERM) {
                // 没有权限检查，但我们不将其视为错误
                // 路径视为不存在（从调用者角度）
                is_excited_out = false;
                mes_out = mes::a_mes(0, 0, "");
                return;
            }
            else {
                mes_out = bfs_m::err::service_error_dir_control_check_existence_failed();
                return;
            }
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    // 检查路径是否是目录
    void get_is_dir(
        str path,
        bool& is_dir_out,
        mes::a_mes& mes_out
    ) {
        // 重置消息和输出
        mes_out = mes::a_mes(0, 0, "");
        is_dir_out = false;

        // ====================== 定理1：基础有效性检查 ======================
        if (path.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        if (!__check_path_valid__(path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }

        // 检查是否只包含空白字符
        bool all_whitespace = true;
        for (char c : path) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                all_whitespace = false;
                break;
            }
        }
        if (all_whitespace) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        // ====================== 步骤1：规范化路径 ======================
        f_s::path fs_path;
        try {
            fs_path = f_s::path(path);

            if (fs_path.empty()) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_empty();
                return;
            }
        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_invalid_path();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤2：检查存在性和类型 ======================
        try {
            // 先检查是否存在
            if (!f_s::exists(fs_path)) {
                // 路径不存在，不是目录
                is_dir_out = false;
                mes_out = mes::a_mes(0, 0, "");
                return;
            }

            // 检查是否是目录
            is_dir_out = f_s::is_directory(fs_path);

        }
        catch (const f_s::filesystem_error& e) {
            if (e.code().value() == EACCES || e.code().value() == EPERM) {
                // 没有权限访问，无法确定是否是目录
                mes_out = bfs_m::err::service_error_dir_control_no_read_permission();
                return;
            }
            else {
                mes_out = bfs_m::err::service_error_dir_control_check_type_failed();
                return;
            }
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    // 检查路径是否是普通文件
    void get_is_file(
        str path,
        bool& is_file_out,
        mes::a_mes& mes_out
    ) {
        // 重置消息和输出
        mes_out = mes::a_mes(0, 0, "");
        is_file_out = false;

        // ====================== 定理1：基础有效性检查 ======================
        if (path.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        if (!__check_path_valid__(path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }

        // 检查是否只包含空白字符
        bool all_whitespace = true;
        for (char c : path) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                all_whitespace = false;
                break;
            }
        }
        if (all_whitespace) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        // ====================== 步骤1：规范化路径 ======================
        f_s::path fs_path;
        try {
            fs_path = f_s::path(path);

            if (fs_path.empty()) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_empty();
                return;
            }
        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_invalid_path();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤2：检查存在性和类型 ======================
        try {
            // 先检查是否存在
            if (!f_s::exists(fs_path)) {
                // 路径不存在，不是文件
                is_file_out = false;
                mes_out = mes::a_mes(0, 0, "");
                return;
            }

            // 检查是否是普通文件
            is_file_out = f_s::is_regular_file(fs_path);

        }
        catch (const f_s::filesystem_error& e) {
            if (e.code().value() == EACCES || e.code().value() == EPERM) {
                // 没有权限访问，无法确定是否是文件
                mes_out = bfs_m::err::service_error_dir_control_no_read_permission();
                return;
            }
            else {
                mes_out = bfs_m::err::service_error_dir_control_check_type_failed();
                return;
            }
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    // 检查路径是否可访问（有读取权限）
    void get_accessible(
        str path,
        bool& accessible_out,
        mes::a_mes& mes_out
    ) {
        // 重置消息和输出
        mes_out = mes::a_mes(0, 0, "");
        accessible_out = false;

        // ====================== 定理1：基础有效性检查 ======================
        if (path.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        if (!__check_path_valid__(path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }

        // 检查是否只包含空白字符
        bool all_whitespace = true;
        for (char c : path) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                all_whitespace = false;
                break;
            }
        }
        if (all_whitespace) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_try_create_dir_with_empty_path)
        }

        // ====================== 步骤1：规范化路径 ======================
        f_s::path fs_path;
        try {
            fs_path = f_s::path(path);

            if (fs_path.empty()) {
                mes_out = bfs_m::err::service_error_dir_control_path_is_empty();
                return;
            }
        }
        catch (f_s::filesystem_error) {
            mes_out = bfs_m::err::service_error_dir_control_invalid_path();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 步骤2：检查可访问性 ======================
        try {
            // 尝试多种操作来检查可访问性

            // 1. 检查是否存在（基本可访问性）
            if (!f_s::exists(fs_path)) {
                // 路径不存在，不可访问
                accessible_out = false;
                mes_out = mes::a_mes(0, 0, "");
                return;
            }

            // 2. 检查权限
            auto perms = f_s::status(fs_path).permissions();

            // 检查是否有所有者读取权限
            bool has_read_permission = (perms & f_s::perms::owner_read) != f_s::perms::none;

            // 如果是目录，还需要执行权限来遍历
            bool has_dir_permission = true;
            if (f_s::is_directory(fs_path)) {
                has_dir_permission = (perms & f_s::perms::owner_exec) != f_s::perms::none;
            }

            accessible_out = has_read_permission && has_dir_permission;

            // 3. 尝试一个轻量级操作来验证（可选）
            if (accessible_out) {
                try {
                    // 尝试打开目录迭代器（不实际迭代）
                    auto it = f_s::directory_iterator(fs_path);
                    // 如果能创建迭代器，说明可访问
                }
                catch (...) {
                    // 创建迭代器失败，可能实际不可访问
                    accessible_out = false;
                }
            }

        }
        catch (f_s::filesystem_error) {
            // 检查过程中出错，视为不可访问
            accessible_out = false;
            mes_out = mes::a_mes(0, 0, "");
            return;
        }
        catch (std::exception) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_unknown_error();
            return;
        }

        // ====================== 成功 ======================
        mes_out = mes::a_mes(0, 0, "");
    }

    void rename_file(
        str old_path,
        str new_path,
        mes::a_mes& mes_out,
        bool overwrite
    ) {
        mes_out = mes::a_mes(0, 0, "");

        // 1. 参数检查
        if (old_path.empty() || new_path.empty()) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_rename_empty_path)
        }

        if (!__check_path_valid__(old_path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }
        if (!__check_path_valid__(new_path)) {
            __CODING_ERROR__(bfs_m::err::coding_error_dir_control_invalid_path_of_your_system)
        }

        // 2. 路径规范化
        f_s::path old_fs_path, new_fs_path;
        try {
            old_fs_path = f_s::absolute(old_path);
            new_fs_path = f_s::absolute(new_path);

            // 检查是否相同文件
            if (old_fs_path == new_fs_path) {
                mes_out = bfs_m::err::service_error_dir_control_rename_same_file();
                return;
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_invalid_path();
            return;
        }

        // 3. 检查源文件
        try {
            if (!f_s::exists(old_fs_path)) {
                mes_out = bfs_m::err::service_error_dir_control_source_not_exist();
                return;
            }

            // 如果是目录，需要特殊处理
            if (f_s::is_directory(old_fs_path)) {
                // 可以单独实现rename_dir，或这里统一处理
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_check_source_failed();
            return;
        }

        // 4. 检查目标文件
        try {
            bool target_exists = f_s::exists(new_fs_path);

            if (target_exists) {
                if (!overwrite) {
                    mes_out = bfs_m::err::service_error_dir_control_target_already_exists();
                    return;
                }

                // 检查是否可以覆盖
                if (!f_s::is_regular_file(new_fs_path) &&
                    !f_s::is_directory(new_fs_path)) {
                    mes_out = bfs_m::err::service_error_dir_control_target_not_regular();
                    return;
                }
            }
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_check_target_failed();
            return;
        }

        // 5. 执行重命名
        try {
            if (overwrite && f_s::exists(new_fs_path)) {
                // 先删除目标文件（如果需要）
                f_s::remove_all(new_fs_path);
            }

            f_s::rename(old_fs_path, new_fs_path);
        }
        catch (const f_s::filesystem_error& e) {
            // 根据错误码分类
            switch (e.code().value()) {
            case EACCES:
            case EPERM:
                mes_out = bfs_m::err::service_error_dir_control_permission_denied();
                break;
            case ENOSPC:
                mes_out = bfs_m::err::service_error_dir_control_no_space();
                break;
            case EXDEV:  // 跨设备/卷
                mes_out = bfs_m::err::service_error_dir_control_cross_device();
                break;
            case EBUSY:
                mes_out = bfs_m::err::service_error_dir_control_resource_busy();
                break;
            default:
                mes_out = bfs_m::err::service_error_dir_control_rename_failed();
            }
            return;
        }
        catch (...) {
            mes_out = bfs_m::err::service_error_dir_control_rename_failed();
            return;
        }

        // 6. 验证
        try {
            if (f_s::exists(old_fs_path) || !f_s::exists(new_fs_path)) {
                mes_out = bfs_m::err::service_error_dir_control_rename_verification_failed();
                return;
            }
        }
        catch (...) {
            // 忽略验证异常，至少重命名操作完成了
        }

        // 成功
        mes_out = mes::a_mes(0, 0, "");
    }
};