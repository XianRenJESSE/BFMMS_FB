// BFS_file_control_test.cpp
module BFS;

#include "BFS_dp.h"

#if defined(BFS_INCLUDE_TEST)
#include "TEST_helper.h"
#endif

namespace bfs_f::dir_control {
#if defined(BFS_INCLUDE_TEST)

    namespace file_control_test_detail {

        inline std::string get_test_dir() {
            return (std::filesystem::temp_directory_path() / "bfs_test" / "dir_control").string() + "/";
        }

        inline bool create_test_file(const std::string& path, const std::string& content = "") {
            try {
                std::ofstream f(path);
                if (f.is_open()) {
                    if (!content.empty()) f << content;
                    return true;
                }
                return false;
            }
            catch (...) {
                return false;
            }
        }

        inline bool verify_dir_contents(const std::string& path, size_t expected_files = 0, size_t expected_dirs = 0) {
            try {
                if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
                    return false;

                size_t file_cnt = 0, dir_cnt = 0;
                for (const auto& entry : std::filesystem::directory_iterator(path)) {
                    if (std::filesystem::is_regular_file(entry.status())) file_cnt++;
                    else if (std::filesystem::is_directory(entry.status())) dir_cnt++;
                }
                return file_cnt == expected_files && dir_cnt == expected_dirs;
            }
            catch (...) {
                return false;
            }
        }

        inline bool compare_string_vectors(const vector<str>& v1, const vector<str>& v2) {
            if (v1.size() != v2.size()) return false;
            auto s1 = v1, s2 = v2;
            std::sort(s1.begin(), s1.end());
            std::sort(s2.begin(), s2.end());
            return s1 == s2;
        }

        inline void cleanup_test_path(const std::string& path) {
            try {
                if (std::filesystem::exists(path)) {
                    std::filesystem::remove_all(path);
                }
            }
            catch (...) {
                // 忽略清理错误，测试环境不要求100%清理成功
            }
        }

    } // namespace file_control_test_detail

    void test() {
        using namespace file_control_test_detail;

        TEST_MODULE_BEGIN("DIR_CONTROL");

        std::string test_dir = get_test_dir();
        try { std::filesystem::create_directories(test_dir); }
        catch (...) {}

        // =====================================================================
        // 1. add_dir 测试
        // =====================================================================
        TEST_CLASS_BEGIN("add_dir");
        TEST_FUNCTION_BEGIN("normal cases");
        {
            mes::a_mes local_mes;

            // 正例1：创建新目录
            {
                std::string dir = test_dir + "add_dir_normal/";
                cleanup_test_path(dir);
                TEST_POSITIVE("create new directory", local_mes, {
                    add_dir(dir, _m);
                    if (!std::filesystem::exists(dir)) throw std::runtime_error("dir not created");
                    if (!std::filesystem::is_directory(dir)) throw std::runtime_error("not a directory");
                    });
            }

            // 正例2：递归创建多级目录
            {
                std::string deep = test_dir + "level1/level2/level3/";
                cleanup_test_path(test_dir + "level1");
                TEST_POSITIVE("recursive create directories", local_mes, {
                    add_dir(deep, _m);
                    if (!std::filesystem::exists(deep)) throw std::runtime_error("deep dir not created");
                    });
            }

            // 正例3：创建已存在的目录（幂等性）
            {
                std::string dir = test_dir + "add_dir_existing/";
                std::filesystem::create_directories(dir);
                TEST_POSITIVE("create existing directory (idempotent)", local_mes, {
                    add_dir(dir, _m);
                // 应该静默成功，不报错
                    });
            }

            // 正例4：当前目录
            {
                std::string dir = test_dir + "./././";
                TEST_POSITIVE("create current directory", local_mes, {
                    add_dir(dir, _m);
                    });
            }
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("coding errors");
        {
            // 编码错误：空路径
            TEST_CONVERSE_CODING("empty path", {
                add_dir("", _m);
                });

            // 编码错误：只有空格的路径（会变成空路径）
            TEST_CONVERSE_CODING("path with only spaces", {
                add_dir("   ", _m);
                });
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("service errors");
        {
            mes::a_mes local_mes;

            // 服务错误：路径已存在但不是目录
            {
                std::string file = test_dir + "add_dir_conflict.txt";
                create_test_file(file);
                TEST_CONVERSE_SERVICE("path exists but is not directory",
                    bfs_m::err::service_error_dir_control_path_exists_but_not_dir().code,
                    local_mes, {
                    add_dir(file, _m);
                    });
                std::filesystem::remove(file);
            }

            // 服务错误：无效路径 std::filesystem::path下，错误字符串不抛出异常，该测试不可达，故删除

            // 注意：权限不足、磁盘满等需要手动测试，不在单元测试范围
        }
        TEST_FUNCTION_END();
        TEST_CLASS_END();

        // =====================================================================
        // 2. del 测试
        // =====================================================================
        TEST_CLASS_BEGIN("del");
        TEST_FUNCTION_BEGIN("normal cases");
        {
            mes::a_mes local_mes;

            // 正例1：删除文件
            {
                std::string file = test_dir + "del_file.txt";
                create_test_file(file);
                TEST_POSITIVE("delete file", local_mes, {
                    del(file, _m);
                    if (std::filesystem::exists(file)) throw std::runtime_error("file still exists");
                    });
            }

            // 正例2：删除空目录
            {
                std::string dir = test_dir + "del_empty_dir/";
                std::filesystem::create_directories(dir);
                TEST_POSITIVE("delete empty directory", local_mes, {
                    del(dir, _m);
                    if (std::filesystem::exists(dir)) throw std::runtime_error("dir still exists");
                    });
            }

            // 正例3：递归删除非空目录
            {
                std::string dir = test_dir + "del_nonempty_dir/";
                std::string sub = dir + "sub/";
                std::string file = dir + "file.txt";
                std::filesystem::create_directories(sub);
                create_test_file(file);
                TEST_POSITIVE("recursive delete non-empty directory", local_mes, {
                    del(dir, _m);
                    if (std::filesystem::exists(dir)) throw std::runtime_error("dir still exists");
                    });
            }

            // 正例4：删除不存在的路径（幂等性）
            {
                std::string non_existent = test_dir + "del_non_existent/";
                cleanup_test_path(non_existent);
                TEST_POSITIVE("delete non-existent path (idempotent)", local_mes, {
                    del(non_existent, _m);
                // 应该静默成功
                    });
            }

            // 正例5：删除符号链接（只删链接本身）
            {
                std::string target = test_dir + "symlink_target.txt";
                std::string link = test_dir + "symlink.lnk";
                create_test_file(target);
                try {
                    std::filesystem::create_symlink(target, link);
                    TEST_POSITIVE("delete symlink only", local_mes, {
                        del(link, _m);
                        if (std::filesystem::exists(link)) throw std::runtime_error("link still exists");
                        if (!std::filesystem::exists(target)) throw std::runtime_error("target was deleted");
                        });
                }
                catch (...) {
                    // 平台可能不支持符号链接
                }
                std::filesystem::remove(target);
            }
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("coding errors");
        {
            TEST_CONVERSE_CODING("empty path", {
                del("", _m);
                });
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("service errors");
        {
            mes::a_mes local_mes;

            // 注意：根目录保护测试
            {
                std::string root = std::filesystem::absolute("/").string();
                TEST_CONVERSE_SERVICE("cannot delete root directory",
                    bfs_m::err::service_error_dir_control_cannot_delete_root().code,
                    local_mes, {
                    del(root, _m);
                    });
            }

            // 权限不足测试需要特殊环境，这里只验证错误码存在
        }
        TEST_FUNCTION_END();
        TEST_CLASS_END();

        // =====================================================================
        // 3. get_dir_list / get_list_file 测试
        // =====================================================================
        TEST_CLASS_BEGIN("list operations");
        TEST_FUNCTION_BEGIN("get_dir_list");
        {
            mes::a_mes local_mes;
            std::string parent = test_dir + "list_parent/";
            std::filesystem::create_directories(parent);

            // 准备测试数据
            std::string sub1 = parent + "subdir1/";
            std::string sub2 = parent + "subdir2/";
            std::string file1 = parent + "file1.txt";
            std::string subsub = sub1 + "subsub/";

            std::filesystem::create_directories(sub1);
            std::filesystem::create_directories(sub2);
            std::filesystem::create_directories(subsub);
            create_test_file(file1);

            // 正例1：获取目录列表（只返回直接子目录）
            {
                vector<str> dir_list;
                TEST_POSITIVE("list subdirectories only", local_mes, {
                    get_dir_list(parent, dir_list, _m);
                    if (dir_list.size() != 2) throw std::runtime_error("expected 2 dirs");
                    });

                // 验证路径（可能返回绝对路径）
                bool found_sub1 = false, found_sub2 = false;
                for (const auto& p : dir_list) {
                    if (p.find("subdir1") != std::string::npos) found_sub1 = true;
                    if (p.find("subdir2") != std::string::npos) found_sub2 = true;
                }
                if (!found_sub1 || !found_sub2) {
                    TEST_REPORT_ERROR("directory list content mismatch");
                }
            }

            // 正例2：空目录
            {
                std::string empty = test_dir + "list_empty/";
                std::filesystem::create_directories(empty);
                vector<str> dir_list;
                TEST_POSITIVE("list empty directory", local_mes, {
                    get_dir_list(empty, dir_list, _m);
                    if (!dir_list.empty()) throw std::runtime_error("expected empty list");
                    });
            }
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("get_list_file");
        {
            mes::a_mes local_mes;
            std::string parent = test_dir + "file_list_parent/";
            std::filesystem::create_directories(parent);

            std::string file1 = parent + "a_file.txt";
            std::string file2 = parent + "b_file.txt";
            std::string subdir = parent + "subdir/";
            create_test_file(file1, "data1");
            create_test_file(file2, "data2");
            std::filesystem::create_directories(subdir);

            // 正例：只返回文件，不返回目录
            {
                vector<str> file_list;
                TEST_POSITIVE("list files only", local_mes, {
                    get_list_file(parent, file_list, _m);
                    if (file_list.size() != 2) throw std::runtime_error("expected 2 files");
                    });

                // 验证排序
                bool sorted = std::is_sorted(file_list.begin(), file_list.end());
                if (!sorted) TEST_REPORT_ERROR("file list not sorted");
            }
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("service errors for list operations");
        {
            mes::a_mes local_mes;
            vector<str> dummy;

            // 目录不存在
            {
                std::string non_existent = test_dir + "list_non_existent/";
                cleanup_test_path(non_existent);
                TEST_CONVERSE_SERVICE("directory does not exist",
                    bfs_m::err::service_error_dir_control_path_not_exist().code,
                    local_mes, {
                    get_dir_list(non_existent, dummy, _m);
                    });
            }

            // 路径是文件，不是目录
            {
                std::string file = test_dir + "list_not_dir.txt";
                create_test_file(file);
                TEST_CONVERSE_SERVICE("path is not a directory",
                    bfs_m::err::service_error_dir_control_path_is_not_dir().code,
                    local_mes, {
                    get_dir_list(file, dummy, _m);
                    });
                std::filesystem::remove(file);
            }
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("coding errors for list operations");
        {
            vector<str> dummy;
            TEST_CONVERSE_CODING("empty path for get_dir_list", {
                get_dir_list("", dummy, _m);
                });
            TEST_CONVERSE_CODING("empty path for get_list_file", {
                get_list_file("", dummy, _m);
                });
        }
        TEST_FUNCTION_END();
        TEST_CLASS_END();

        // =====================================================================
        // 4. 路径检查函数测试 (get_path_is_excited / get_is_dir / get_is_file / get_accessible)
        // =====================================================================
        TEST_CLASS_BEGIN("path query functions");
        TEST_FUNCTION_BEGIN("get_path_is_excited");
        {
            mes::a_mes local_mes;
            std::string dir = test_dir + "query_dir/";
            std::string file = test_dir + "query_file.txt";
            std::filesystem::create_directories(dir);
            create_test_file(file);

            bool result = false;

            TEST_POSITIVE("existing directory", local_mes, {
                get_path_is_excited(dir, result, _m);
                if (!result) throw std::runtime_error("directory should exist");
                });

            TEST_POSITIVE("existing file", local_mes, {
                get_path_is_excited(file, result, _m);
                if (!result) throw std::runtime_error("file should exist");
                });

            TEST_POSITIVE("non-existent path", local_mes, {
                get_path_is_excited(dir + "non_existent", result, _m);
                if (result) throw std::runtime_error("should not exist");
                });

            TEST_CONVERSE_CODING("empty path", {
                get_path_is_excited("", result, _m);
                });
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("get_is_dir / get_is_file");
        {
            mes::a_mes local_mes;
            std::string dir = test_dir + "is_dir_test/";
            std::string file = test_dir + "is_file_test.txt";
            std::filesystem::create_directories(dir);
            create_test_file(file);

            bool result = true;

            TEST_POSITIVE("is_dir on directory", local_mes, {
                get_is_dir(dir, result, _m);
                if (!result) throw std::runtime_error("should be directory");
                });

            TEST_POSITIVE("is_dir on file returns false", local_mes, {
                get_is_dir(file, result, _m);
                if (result) throw std::runtime_error("file should not be directory");
                });

            TEST_POSITIVE("is_file on file", local_mes, {
                get_is_file(file, result, _m);
                if (!result) throw std::runtime_error("should be file");
                });

            TEST_POSITIVE("is_file on directory returns false", local_mes, {
                get_is_file(dir, result, _m);
                if (result) throw std::runtime_error("directory should not be file");
                });

            TEST_POSITIVE("non-existent path returns false", local_mes, {
                get_is_dir(dir + "non_existent", result, _m);
                if (result) throw std::runtime_error("non-existent should not be dir");
                });
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("get_accessible");
        {
            mes::a_mes local_mes;
            std::string dir = test_dir + "accessible_dir/";
            std::filesystem::create_directories(dir);

            bool accessible = false;
            TEST_POSITIVE("accessible path", local_mes, {
                get_accessible(dir, accessible, _m);
                if (!accessible) throw std::runtime_error("should be accessible");
                });

            TEST_POSITIVE("non-existent path not accessible", local_mes, {
                get_accessible(dir + "non_existent", accessible, _m);
                if (accessible) throw std::runtime_error("should not be accessible");
                });
        }
        TEST_FUNCTION_END();
        TEST_CLASS_END();

        // =====================================================================
        // 5. rename_file 测试
        // =====================================================================
        TEST_CLASS_BEGIN("rename_file");
        TEST_FUNCTION_BEGIN("normal cases");
        {
            mes::a_mes local_mes;
            std::string test_rename_dir = test_dir + "rename/";
            std::filesystem::create_directories(test_rename_dir);

            std::string old_path = test_rename_dir + "old.txt";
            std::string new_path = test_rename_dir + "new.txt";
            create_test_file(old_path, "test content");

            TEST_POSITIVE("rename file without overwrite", local_mes, {
                rename_file(old_path, new_path, _m, false);
                if (std::filesystem::exists(old_path)) throw std::runtime_error("old file still exists");
                if (!std::filesystem::exists(new_path)) throw std::runtime_error("new file not created");
                });
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("service errors");
        {
            mes::a_mes local_mes;
            std::string test_rename_dir = test_dir + "rename_error/";
            std::filesystem::create_directories(test_rename_dir);

            // 源文件不存在
            {
                std::string non_existent = test_rename_dir + "non_existent.txt";
                std::string target = test_rename_dir + "target.txt";
                cleanup_test_path(non_existent);
                TEST_CONVERSE_SERVICE("source file does not exist",
                    bfs_m::err::service_error_dir_control_source_not_exist().code,
                    local_mes, {
                    rename_file(non_existent, target, _m, false);
                    });
            }

            // 目标已存在且不覆盖
            {
                std::string src = test_rename_dir + "src.txt";
                std::string dst = test_rename_dir + "dst.txt";
                create_test_file(src, "src");
                create_test_file(dst, "dst");
                TEST_CONVERSE_SERVICE("target exists and overwrite=false",
                    bfs_m::err::service_error_dir_control_target_already_exists().code,
                    local_mes, {
                    rename_file(src, dst, _m, false);
                    });
                std::filesystem::remove(src);
                std::filesystem::remove(dst);
            }

            // 相同文件
            {
                std::string file = test_rename_dir + "same.txt";
                create_test_file(file);
                TEST_CONVERSE_SERVICE("source and target are same file",
                    bfs_m::err::service_error_dir_control_rename_same_file().code,
                    local_mes, {
                    rename_file(file, file, _m, true);
                    });
                std::filesystem::remove(file);
            }
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("coding errors");
        {
            mes::a_mes local_mes;
            TEST_CONVERSE_CODING("empty old path", {
                rename_file("", "target.txt", _m, false);
                });
            TEST_CONVERSE_CODING("empty new path", {
                rename_file("source.txt", "", _m, false);
                });
        }
        TEST_FUNCTION_END();
        TEST_CLASS_END();

        // =====================================================================
        // 6. 综合场景测试
        // =====================================================================
        TEST_CLASS_BEGIN("integrated scenarios");
        TEST_FUNCTION_BEGIN("full directory lifecycle");
        {
            mes::a_mes local_mes;
            std::string root = test_dir + "lifecycle/";
            cleanup_test_path(root);

            // 1. 创建目录结构
            add_dir(root + "sub1/subsub/", local_mes);
            add_dir(root + "sub2/", local_mes);
            create_test_file(root + "file1.txt", "data1");
            create_test_file(root + "sub1/file2.txt", "data2");
            create_test_file(root + "sub1/subsub/file3.txt", "data3");

            // 2. 验证
            bool exists = false;
            get_path_is_excited(root, exists, local_mes);
            if (!exists) TEST_REPORT_ERROR("root not created");

            vector<str> dirs;
            get_dir_list(root, dirs, local_mes);
            if (dirs.size() < 2) TEST_REPORT_ERROR("subdir count mismatch");

            vector<str> files;
            get_list_file(root, files, local_mes);
            if (files.size() < 1) TEST_REPORT_ERROR("file count mismatch");

            // 3. 类型检查
            bool is_dir = false, is_file = false;
            get_is_dir(root, is_dir, local_mes);
            get_is_file(root + "file1.txt", is_file, local_mes);
            if (!is_dir || !is_file) TEST_REPORT_ERROR("type check failed");

            // 4. 清理
            del(root, local_mes);
            get_path_is_excited(root, exists, local_mes);
            if (exists) TEST_REPORT_ERROR("cleanup failed");

            TEST_POSITIVE("full lifecycle completed", local_mes, {});
        }
        TEST_FUNCTION_END();

        TEST_FUNCTION_BEGIN("batch operations");
        {
            mes::a_mes local_mes;
            std::string batch = test_dir + "batch/";
            cleanup_test_path(batch);
            std::filesystem::create_directories(batch);

            // 批量创建10个目录
            for (int i = 0; i < 10; ++i) {
                add_dir(batch + "dir" + to_string(i) + "/", local_mes);
                if (local_mes.code != 0) {
                    TEST_REPORT_ERROR("batch dir creation failed at " + to_string(i));
                    break;
                }
            }

            // 批量创建10个文件
            for (int i = 0; i < 10; ++i) {
                create_test_file(batch + "file" + to_string(i) + ".txt", "data");
            }

            // 验证
            vector<str> dirs, files;
            get_dir_list(batch, dirs, local_mes);
            get_list_file(batch, files, local_mes);

            if (dirs.size() == 10 && files.size() == 10) {
                TEST_POSITIVE("batch operations successful", local_mes, {});
            }
            else {
                TEST_REPORT_ERROR("batch verification failed");
            }

            del(batch, local_mes);
        }
        TEST_FUNCTION_END();
        TEST_CLASS_END();

        // 清理
        try {
            std::filesystem::remove_all(test_dir);
        }
        catch (...) {}

        TEST_MODULE_END();
    }
#endif // BFS_INCLUDE_TEST

} // namespace bfs_f::dir_control
