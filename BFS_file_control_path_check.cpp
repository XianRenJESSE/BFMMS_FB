//BFS_file_control_path_check.cpp


//以下用于消除路径UB
#if defined(_WIN32) || defined(_WIN64)
// 禁止 windows.h 定义 min/max 宏
#ifndef NOMINMAX
#define NOMINMAX
#endif

// 最小化 windows.h 的内容
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

// 清理宏，防止影响其他代码
#undef WIN32_LEAN_AND_MEAN
// 注意：不取消 NOMINMAX，让它继续生效

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#endif
//以上用于消除路径UB

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

    #if defined(_WIN32) || defined(_WIN64)
    // UTF-8 到 UTF-16 转换辅助函数
    std::wstring utf8_to_utf16(const std::string& utf8) {
        if (utf8.empty()) return std::wstring();

        int size_needed = MultiByteToWideChar(CP_UTF8, 0,
            utf8.c_str(), static_cast<int>(utf8.size()), NULL, 0);

        if (size_needed <= 0) return std::wstring();

        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0,
            utf8.c_str(), static_cast<int>(utf8.size()), &wstr[0], size_needed);

        return wstr;
    }
    #endif

    // BFS_file_control.cpp 中的路径校验函数

    bool __check_path_valid__(str& path_in) {
        // ====================== 基础检查 ======================
        if (path_in.empty()) {
            return false;
        }

        // 检查空字符（所有平台都不允许）
        for (char c : path_in) {
            if (c == '\0') {
                return false;
            }
        }

        // ====================== 平台特定检查 ======================

#if defined(_WIN32) || defined(_WIN64)
// -------------------- Windows平台：使用WinAPI --------------------

// 将UTF-8转换为UTF-16
        int wide_len = MultiByteToWideChar(CP_UTF8, 0,
            path_in.c_str(), -1, NULL, 0);

        if (wide_len == 0) {
            return false;  // UTF-8转换失败，路径无效
        }

        vector<wchar_t> wide_path(wide_len);
        MultiByteToWideChar(CP_UTF8, 0,
            path_in.c_str(), -1, wide_path.data(), wide_len);

        // 使用GetFileAttributesW检查路径格式
        DWORD attrs = GetFileAttributesW(wide_path.data());

        if (attrs != INVALID_FILE_ATTRIBUTES) {
            // 路径存在且可访问，绝对有效
            return true;
        }

        // 获取错误码
        DWORD err = GetLastError();

        // 这些错误码表示路径格式无效
        if (err == ERROR_INVALID_NAME ||
            err == ERROR_BAD_PATHNAME ||
            err == ERROR_BAD_NET_NAME ||
            err == ERROR_BAD_NETPATH ||
            err == ERROR_ILLEGAL_CHARACTER ||
            err == ERROR_FILENAME_EXCED_RANGE) {
            return false;
        }

        // 其他所有错误（包括ERROR_FILE_NOT_FOUND、ERROR_PATH_NOT_FOUND等）
        // 都表示路径格式有效，只是文件/目录不存在
        // 因为文件控制提供递归操作，路径不存在是允许的
        return true;

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
// -------------------- Linux/Unix平台：使用POSIX API --------------------

// 使用stat检查路径格式
        struct stat st;
        int ret = stat(path_in.c_str(), &st);

        if (ret == 0) {
            // 路径存在且可访问，绝对有效
            return true;
        }

        // 检查错误码
        switch (errno) {
        case ENOENT:
        case ENOTDIR:
            // 路径不存在，但路径名格式有效
            // 因为文件控制提供递归操作，路径不存在是允许的
            return true;

        case EACCES:
        case ELOOP:
        case EOVERFLOW:
            // 权限问题、符号链接循环等，但路径格式有效
            return true;

        case ENAMETOOLONG:
        case EINVAL:
        case EFAULT:
            // 这些错误码表示路径格式无效
            return false;

        default:
            // 其他未知错误，保守认为路径格式有效
            return true;
        }

#else
// -------------------- 未知平台：使用filesystem --------------------

        try {
            std::filesystem::path p(path_in);

            // 尝试创建path对象，如果抛出异常说明路径无效
            // 不检查父目录是否存在，因为文件控制提供递归操作

            // 检查是否包含空字符（已经检查过）
            // 检查路径是否为空（已经检查过）

            return true;
        }
        catch (const std::filesystem::filesystem_error&) {
            // filesystem_error通常表示路径无效
            return false;
        }
        catch (const std::exception&) {
            // 其他异常，保守返回false
            return false;
        }
#endif
    }
};