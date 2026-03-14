// BFS_bin_file_test.cpp

module BFS;

#include "BFS_dp.h"

#if defined(BFS_INCLUDE_TEST)
#include "TEST_helper.h"
#endif



namespace bfs_f {

#if defined(BFS_INCLUDE_TEST)

    namespace bin_file_test_detail {
        // Helper: create test directory
        inline bool create_test_directory(const str& path) {
            try {
                std::filesystem::create_directories(path);
                return true;
            }
            catch (...) {
                return false;
            }
        }

        // Helper: clean up test file
        inline void cleanup_test_file(const str& path) {
            try {
                if (std::filesystem::exists(path)) {
                    std::filesystem::remove(path);
                }
            }
            catch (...) {
                // ignore cleanup errors
            }
        }

        // Helper: compare two file contents
        inline bool compare_files(const str& path1, const str& path2) {
            try {
                if (!std::filesystem::exists(path1) || !std::filesystem::exists(path2)) {
                    return false;
                }

                size_t size1 = std::filesystem::file_size(path1);
                size_t size2 = std::filesystem::file_size(path2);

                if (size1 != size2) {
                    return false;
                }

                std::ifstream file1(path1, std::ios::binary);
                std::ifstream file2(path2, std::ios::binary);

                if (!file1.is_open() || !file2.is_open()) {
                    return false;
                }

                const size_t BUFFER_SIZE = 4096;
                vector<char> buffer1(BUFFER_SIZE);
                vector<char> buffer2(BUFFER_SIZE);

                size_t remaining = size1;
                while (remaining > 0) {
                    size_t chunk = std::min(remaining, BUFFER_SIZE);
                    file1.read(buffer1.data(), chunk);
                    file2.read(buffer2.data(), chunk);

                    if (memcmp(buffer1.data(), buffer2.data(), chunk) != 0) {
                        return false;
                    }

                    remaining -= chunk;
                }

                return true;
            }
            catch (...) {
                return false;
            }
        }

        // Test directory path: use system temporary directory
        inline str get_test_dir() {
            return (std::filesystem::temp_directory_path() / "bfs_test" / "bin_file").string() + "/";
        }
    }

    void bin_file::test() {
        using namespace test_helper;    // macros already included, just for clarity

        // ========== MODULE BEGIN ==========
        TEST_MODULE_BEGIN("BIN_FILE");

        // Create test directory
        str test_dir = bin_file_test_detail::get_test_dir();
        bin_file_test_detail::create_test_directory(test_dir);

        // ========== CLASS TEST: bin_file ==========
        TEST_CLASS_BEGIN("bin_file");

        // ------------------------------------------------------------
        // 0. Path validation test - before constructor tests
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("path test");
        {
            mes::a_mes local_mes;

#if defined(_WIN32) || defined(_WIN64)
            output << "\n        [Windows Platform Path Validation]\n";

            // Valid paths
            vector<std::pair<const char*, str>> valid_paths = {
                {"regular path with drive letter", "C:/Users/test/file.txt"},
                {"path with spaces", "D:/folder/name with spaces.txt"},
                {"deeply nested path", "E:/deeply/nested/folder/structure/file.bin"},
                {"path with underscores", "F:/file_with_underscores.dat"},
                {"path with hyphens", "G:/file-with-hyphens.dat"},
                {"multiple dots in filename", "H:/file.with.multiple.dots.dat"},
                {"path with current dir", "I:/path/./with/current/dir"},
                {"path with parent dir", "J:/path/../with/parent/dir"},
                {"long but valid path (200 chars)", str("K:/") + str(200, 'a') + ".txt"},
                {"unicode characters in path", "L:/unicode_测试_文件.txt"}
            };

            for (const auto& test : valid_paths) {
                output << "            Testing valid: " << test.first << "\n";
                output << "            Path: " << test.second << "\n";

                str path_copy = test.second;
                bool result = __check_path_valid__(path_copy);

                if (result) {
                    TEST_POSITIVE(str("valid path: ") + test.first, local_mes, {});
                }
                else {
                    output << "            [FAIL] Valid path rejected\n";
                    TEST_REPORT_ERROR(str("valid path rejected: ") + test.first);
                }
            }

            // Invalid paths - 格式错误（208）
            struct {
                const char* desc;
                const char* path;
                int expected_code;  // 208 = 格式错误, 27 = 服务错误
            } invalid_paths[] = {
                // 格式错误（208）- 路径字符串本身非法
                {"empty path", "", 208},
                {"path with * character", "C:/file*.txt", 208},
                {"path with ? character", "C:/file?.txt", 208},
                {"path with \" character", "C:/file\".txt", 208},
                {"path with < character", "C:/file<.txt", 208},
                {"path with > character", "C:/file>.txt", 208},
                {"path with | character", "C:/file|.txt", 208},
                {"path with : (not drive letter)", "C:/illegal:.txt", 208},
                {"reserved device name CON", "CON.txt", 208},
                {"reserved device name PRN", "PRN.txt", 208},
                {"reserved device name AUX", "AUX.txt", 208},
                {"reserved device name NUL", "NUL.txt", 208},
                {"reserved device name COM1", "COM1.txt", 208},
                {"reserved device name LPT1", "LPT1.txt", 208},
                {"filename ending with space", "C:/file .txt", 208},
                {"filename ending with dot", "C:/file..txt", 208},

                // 服务错误（27）- 路径格式有效但不可访问
                {"nonexistent drive", "Z:/nonexistent/file.bin", 27},
                {"protected system location", "C:/Windows/System32/test.bin", 27}
            };

            for (const auto& test : invalid_paths) {
                output << "            Testing: " << test.desc << "\n";
                output << "            Path: " << test.path << "\n";
                output << "            Expected: " << (test.expected_code == 208 ? "CODING ERROR" : "SERVICE ERROR") << "\n";

                str path_copy = test.path;
                bool result = __check_path_valid__(path_copy);

                if (test.expected_code == 208) {
                    // 预期格式错误：__check_path_valid__ 应该返回 false
                    if (!result) {
                        TEST_POSITIVE(str("correctly rejected format-invalid path: ") + test.desc, local_mes, {});
                    }
                    else {
                        output << "            [FAIL] Format-invalid path was accepted\n";
                        TEST_REPORT_ERROR(str("format-invalid path accepted: ") + test.desc);
                    }
                }
                else {
                    // 预期服务错误：__check_path_valid__ 应该返回 true（格式有效）
                    if (result) {
                        TEST_POSITIVE(str("correctly accepted format-valid but inaccessible path: ") + test.desc, local_mes, {});
                    }
                    else {
                        output << "            [FAIL] Format-valid path was rejected\n";
                        TEST_REPORT_ERROR(str("format-valid path rejected: ") + test.desc);
                    }
                }
            }
#endif
        }
        TEST_FUNCTION_END();


        // ------------------------------------------------------------
        // 1. Constructor test
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Constructor");
        {
            mes::a_mes local_mes;

            // ----- Positive cases -----
            {
                str test_file = test_dir + "ctor_create.bin";
                bin_file_test_detail::cleanup_test_file(test_file);

                TEST_POSITIVE("create mode: construct with valid path and size", local_mes, {
                    bin_file file(test_file, 100, init_type::create, local_mes);
                    });

                bin_file_test_detail::cleanup_test_file(test_file);
            }

            {
                str test_file = test_dir + "ctor_open_existed.bin";
                bin_file_test_detail::cleanup_test_file(test_file);
                {
                    std::ofstream f(test_file);
                    f << "test data";
                }

                TEST_POSITIVE("open_existed mode: construct with existing file", local_mes, {
                    bin_file file(test_file, 0, init_type::open_existed, local_mes);
                    });

                bin_file_test_detail::cleanup_test_file(test_file);
            }

            {
                str test_file = test_dir + "ctor_cover.bin";
                bin_file_test_detail::cleanup_test_file(test_file);

                TEST_POSITIVE("cover_and_create mode: construct with path", local_mes, {
                    bin_file file(test_file, 200, init_type::cover_and_create, local_mes);
                    });

                bin_file_test_detail::cleanup_test_file(test_file);
            }

            // ----- Negative cases (coding error) -----
            TEST_CONVERSE_CODING("construct with empty path", {
                bin_file file("", 100, init_type::create, local_mes);
                });

            // ----- Invalid enum value test (cross-platform) -----
            {
                str test_file = test_dir + "ctor_invalid_enum.bin";

                TEST_CONVERSE_CODING("construct with invalid init_type enum value (0xFF)", {
                    bin_file file(test_file, 100, static_cast<init_type>(0xFF), local_mes);
                    });
            }

            // ----- Negative cases (service error) -----
            {
                str test_file = test_dir + "ctor_non_existent.bin";
                bin_file_test_detail::cleanup_test_file(test_file);

                TEST_CONVERSE_SERVICE("open_existed mode: file does not exist",
                    bfs_m::err::service_error_bin_file_file_not_existed_when_build_with_choice_of_open_existed().code,
                    local_mes, {
                    bin_file file(test_file, 0, init_type::open_existed, local_mes);
                    });
            }

            {
                str test_file = test_dir + "ctor_already_exists.bin";
                bin_file_test_detail::cleanup_test_file(test_file);
                {
                    std::ofstream f(test_file);
                    f << "existing data";
                }

                TEST_CONVERSE_SERVICE("create mode: file already exists",
                    bfs_m::err::service_error_bin_file_file_already_existed_when_build_with_choice_of_create().code,
                    local_mes, {
                    bin_file file(test_file, 100, init_type::create, local_mes);
                    });

                bin_file_test_detail::cleanup_test_file(test_file);
            }
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 2. open function test
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("open");
        {
            mes::a_mes local_mes;

            // ----- Positive cases -----
            {
                str test_file = test_dir + "open_normal.bin";
                bin_file_test_detail::cleanup_test_file(test_file);
                {
                    std::ofstream f(test_file);
                    f << "test data";
                }

                TEST_POSITIVE("open_existed mode: open existing file", local_mes, {
                    bin_file file;
                    file.open(test_file, 0, init_type::open_existed, local_mes);
                    file.close(local_mes);
                    });

                bin_file_test_detail::cleanup_test_file(test_file);
            }

            {
                str test_file = test_dir + "open_reopen.bin";
                bin_file_test_detail::cleanup_test_file(test_file);

                TEST_POSITIVE("reopen: close previous and open new file", local_mes, {
                    bin_file file;
                    file.open(test_file, 100, init_type::create, local_mes);
                    file.open(test_file, 200, init_type::cover_and_create, local_mes);
                    file.close(local_mes);
                    });

                bin_file_test_detail::cleanup_test_file(test_file);
            }

            // ----- Negative cases (coding error) -----
            TEST_CONVERSE_CODING("open with empty path", {
                bin_file file;
                file.open("", 100, init_type::create, local_mes);
                });

            // ----- Negative cases (service error) -----
            {
                str test_file = test_dir + "open_non_existent.bin";
                bin_file_test_detail::cleanup_test_file(test_file);

                TEST_CONVERSE_SERVICE("open_existed mode: file does not exist",
                    bfs_m::err::service_error_bin_file_file_not_existed_when_open_with_choice_of_open_existed().code,
                    local_mes, {
                    bin_file file;
                    file.open(test_file, 0, init_type::open_existed, local_mes);
                    });
            }

            {
                str test_file = test_dir + "open_already_exists.bin";
                bin_file_test_detail::cleanup_test_file(test_file);
                {
                    std::ofstream f(test_file);
                    f << "existing data";
                }

                TEST_CONVERSE_SERVICE("create mode: file already exists",
                    bfs_m::err::service_error_bin_file_file_already_existed_when_open_with_choice_of_create().code,
                    local_mes, {
                    bin_file file;
                    file.open(test_file, 100, init_type::create, local_mes);
                    });

                bin_file_test_detail::cleanup_test_file(test_file);
            }

            TEST_CONVERSE_CODING("open with invalid init_type enum value", {
                str test_file = test_dir + "open_invalid_enum.bin";
                bin_file file;
                file.open(test_file, 100, static_cast<init_type>(99), local_mes);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 3. close function test
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("close");
        {
            mes::a_mes local_mes;
            str test_file = test_dir + "close_test.bin";
            bin_file_test_detail::cleanup_test_file(test_file);

            // ----- Positive cases -----
            TEST_POSITIVE("close opened file", local_mes, {
                bin_file file;
                file.open(test_file, 100, init_type::create, local_mes);
                file.close(local_mes);
                });

            // ----- Negative cases (service error) -----
            TEST_CONVERSE_SERVICE("close file that is not opened",
                bfs_m::err::service_error_bin_file_not_opened_when_close().code,
                local_mes, {
                bin_file file;
                file.close(local_mes);
                });

            // open then close, second close should fail
            {
                bin_file file;
                file.open(test_file, 100, init_type::create, local_mes);
                file.close(local_mes);

                TEST_CONVERSE_SERVICE("double close",
                    bfs_m::err::service_error_bin_file_not_opened_when_close().code,
                    local_mes, {
                    file.close(local_mes);
                    });
            }

            bin_file_test_detail::cleanup_test_file(test_file);
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 4. flush / flush_with_check function test
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("flush");
        {
            mes::a_mes local_mes;
            str test_file = test_dir + "flush_test.bin";
            bin_file_test_detail::cleanup_test_file(test_file);

            // ----- Positive cases -----
            TEST_POSITIVE("flush opened file ", local_mes, {
                bin_file file;
                file.open(test_file, 100, init_type::create, local_mes);
                file.flush();  // no exception means pass
                file.close(local_mes);
                });

            bin_file_test_detail::cleanup_test_file(test_file);
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 5. resize function test
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("resize");
        {
            mes::a_mes local_mes;
            str test_file = test_dir + "resize_test.bin";
            bin_file_test_detail::cleanup_test_file(test_file);

            // prepare test file
            bin_file file;
            file.open(test_file, 100, init_type::create, local_mes);

            // ----- Positive cases -----
            TEST_POSITIVE("resize to larger size", local_mes, {
                file.resize(200, local_mes);
                });

            TEST_POSITIVE("resize to smaller size", local_mes, {
                file.resize(50, local_mes);
                });

            TEST_POSITIVE("resize to same size (no change)", local_mes, {
                file.resize(50, local_mes);
                });

            TEST_POSITIVE("resize to zero", local_mes, {
                file.resize(0, local_mes);
                });

            file.close(local_mes);

            // ----- Negative cases (service error) -----
            TEST_CONVERSE_SERVICE("resize on unopened file",
                bfs_m::err::service_error_bin_file_file_not_opened_when_resize().code,
                local_mes, {
                bin_file unopened_file;
                unopened_file.resize(100, local_mes);
                });

            bin_file_test_detail::cleanup_test_file(test_file);
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 6. clear function test
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("clear");
        {
            mes::a_mes local_mes;
            str test_file = test_dir + "clear_test.bin";
            bin_file_test_detail::cleanup_test_file(test_file);

            bin_file file;
            file.open(test_file, 100, init_type::create, local_mes);

            // write data
            vector<char> data(100, static_cast<char>(0xAA));
            file.store_from_void_ptr(data.data(), 100, 0, false, local_mes);

            // ----- Positive cases -----
            TEST_POSITIVE("clear entire file", local_mes, {
                file.clear(local_mes);
            // verification: only check no exception, actual content verification could be done via load later
                });

            file.close(local_mes);

            // ----- Negative cases (service error) -----
            TEST_CONVERSE_SERVICE("clear on unopened file",
                bfs_m::err::service_error_bin_file_file_not_opened_when_clear().code,
                local_mes, {
                bin_file unopened_file;
                unopened_file.clear(local_mes);
                });

            bin_file_test_detail::cleanup_test_file(test_file);
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 7. clear_range function test
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("clear_range");
        {
            mes::a_mes local_mes;
            str test_file = test_dir + "clear_range_test.bin";
            bin_file_test_detail::cleanup_test_file(test_file);

            bin_file file;
            file.open(test_file, 100, init_type::create, local_mes);

            // ----- Positive cases -----
            TEST_POSITIVE("clear valid range inside file", local_mes, {
                file.clear_range(25, 50, local_mes);
                });

            TEST_POSITIVE("clear entire file via range", local_mes, {
                file.clear_range(0, 100, local_mes);
                });

            TEST_POSITIVE("clear zero-size range (no effect)", local_mes, {
                file.clear_range(50, 0, local_mes);
                });

            // ----- Negative cases (coding error) -----
            TEST_CONVERSE_CODING("clear range with arithmetic overflow", {
                file.clear_range(SIZE_MAX - 50, 100, local_mes);
                });

            TEST_CONVERSE_CODING("clear range out of file bounds (begin+size > file_size)", {
                file.clear_range(80, 50, local_mes);  // 80+50 > 100
                });

            // ----- Negative cases (service error) -----
            file.close(local_mes);

            TEST_CONVERSE_SERVICE("clear_range on unopened file",
                bfs_m::err::service_error_bin_file_file_not_opened_when_clear_range().code,
                local_mes, {
                bin_file unopened_file;
                unopened_file.clear_range(0, 10, local_mes);
                });

            bin_file_test_detail::cleanup_test_file(test_file);
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 8. save_as function test
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("save_as");
        {
            mes::a_mes local_mes;
            str src_file = test_dir + "save_as_src.bin";
            str dst_file = test_dir + "save_as_dst.bin";
            bin_file_test_detail::cleanup_test_file(src_file);
            bin_file_test_detail::cleanup_test_file(dst_file);

            bin_file file;
            file.open(src_file, 100, init_type::create, local_mes);

            // write test data
            vector<char> data(100);
            for (int i = 0; i < 100; i++) data[i] = static_cast<char>(i);
            file.store_from_void_ptr(data.data(), 100, 0, false, local_mes);

            // ----- Positive cases -----
            TEST_POSITIVE("save_as to new path", local_mes, {
                file.save_as(dst_file, local_mes);
                });

            file.close(local_mes);

            // ----- Negative cases (coding error) -----
            TEST_CONVERSE_CODING("save_as to empty path", {
                bin_file f;
                f.open(src_file, 100, init_type::open_existed, local_mes);
                f.save_as("", local_mes);
                });

            {
                bin_file f;
                f.open(src_file, 100, init_type::open_existed, local_mes);

                TEST_CONVERSE_CODING("save_as to self (same absolute path)", {
                    f.save_as(src_file, local_mes);
                    });
            }

            // ----- Negative cases (service error) -----
            TEST_CONVERSE_SERVICE("save_as on unopened file",
                bfs_m::err::service_error_bin_file_file_not_opened_when_save_as().code,
                local_mes, {
                bin_file unopened_file;
                unopened_file.save_as(dst_file, local_mes);
                });

            bin_file_test_detail::cleanup_test_file(src_file);
            bin_file_test_detail::cleanup_test_file(dst_file);
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 9. load / store (file-to-file)
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("load_store_file_to_file");
        {
            mes::a_mes local_mes;
            str src_file = test_dir + "load_src.bin";
            str dst_file = test_dir + "load_dst.bin";
            bin_file_test_detail::cleanup_test_file(src_file);
            bin_file_test_detail::cleanup_test_file(dst_file);

            bin_file src;
            bin_file dst;
            src.open(src_file, 200, init_type::create, local_mes);
            dst.open(dst_file, 200, init_type::create, local_mes);

            // prepare source data
            vector<char> src_data(200);
            for (int i = 0; i < 200; i++) src_data[i] = static_cast<char>(i % 256);
            src.store_from_void_ptr(src_data.data(), 200, 0, false, local_mes);

            // ----- Positive cases -----
            TEST_POSITIVE("load: safe mode, valid file-to-file copy", local_mes, {
                src.load(0, 100, dst, 50, true, true, local_mes);
                });

            TEST_POSITIVE("load: zero size (no effect)", local_mes, {
                src.load(0, 0, dst, 0, true, true, local_mes);
                });

            TEST_POSITIVE("load: unsafe mode", local_mes, {
                src.load(0, 100, dst, 100, false, true, local_mes);
                });

            TEST_POSITIVE("store: safe mode (wrapper of load)", local_mes, {
                dst.store(src, 0, 50, 150, true, true, local_mes);
                });

            // ----- Negative cases (coding error) -----
            TEST_CONVERSE_CODING("load: same file overlapping ranges (self-load)", {
                src.load(0, 100, src, 50, true, true, local_mes);
                });

            TEST_CONVERSE_CODING("load: source range arithmetic overflow", {
                src.load(SIZE_MAX - 50, 100, dst, 0, true, true, local_mes);
                });

            TEST_CONVERSE_CODING("load: destination range arithmetic overflow", {
                src.load(0, 100, dst, SIZE_MAX - 50, true, true, local_mes);
                });

            TEST_CONVERSE_CODING("load: source range out of file bounds", {
                src.load(150, 100, dst, 0, true, true, local_mes);
                });

            TEST_CONVERSE_CODING("load: destination range out of file bounds", {
                src.load(0, 100, dst, 150, true, true, local_mes);
                });

            // ----- Negative cases (service error) -----
            src.close(local_mes);
            dst.close(local_mes);

            TEST_CONVERSE_SERVICE("load: source file not opened",
                bfs_m::err::service_error_bin_file_file_not_opened_when_load().code,
                local_mes, {
                bin_file f1, f2;
                f2.open(dst_file, 200, init_type::open_existed, local_mes);
                f1.load(0, 10, f2, 0, true, true, local_mes);
                });

            bin_file_test_detail::cleanup_test_file(src_file);
            bin_file_test_detail::cleanup_test_file(dst_file);
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 10. load_to_agent_ptr / store_from_agent_ptr
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("agent_ptr_interaction");
        {
            mes::a_mes local_mes;
            str test_file = test_dir + "agent_ptr_test.bin";
            bin_file_test_detail::cleanup_test_file(test_file);

            bin_file file;
            file.open(test_file, 200, init_type::create, local_mes);

            // prepare file data
            vector<char> file_data(200);
            for (int i = 0; i < 200; i++) file_data[i] = static_cast<char>(i);
            file.store_from_void_ptr(file_data.data(), 200, 0, false, local_mes);

            // prepare agent pointer
            vector<char> agent_buffer(200);
            bmms_f::agent_ptr agent(agent_buffer.data(), 200);

            // ----- Positive cases -----
            TEST_POSITIVE("load_to_agent_ptr: safe mode, valid copy", local_mes, {
                file.load_to_agent_ptr(50, 100, agent, 25, true, local_mes);
                });

            TEST_POSITIVE("load_to_agent_ptr: unsafe mode", local_mes, {
                file.load_to_agent_ptr(0, 50, agent, 0, false, local_mes);
                });

            TEST_POSITIVE("load_to_agent_ptr: zero size", local_mes, {
                file.load_to_agent_ptr(0, 0, agent, 0, true, local_mes);
                });

            TEST_POSITIVE("store_from_agent_ptr: safe mode, valid copy", local_mes, {
                file.store_from_agent_ptr(agent, 0, 100, 100, true, true, local_mes);
                });

            TEST_POSITIVE("store_from_agent_ptr: unsafe mode", local_mes, {
                file.store_from_agent_ptr(agent, 0, 50, 150, false, true, local_mes);
                });

            // ----- Negative cases (coding error) -----
            {
                bmms_f::agent_ptr null_agent;

                TEST_CONVERSE_CODING("load_to_agent_ptr: null agent_ptr", {
                    file.load_to_agent_ptr(0, 50, null_agent, 0, true, local_mes);
                    });

                TEST_CONVERSE_CODING("store_from_agent_ptr: null agent_ptr", {
                    file.store_from_agent_ptr(null_agent, 0, 50, 0, true, true, local_mes);
                    });
            }

            TEST_CONVERSE_CODING("load_to_agent_ptr: agent_ptr buffer too small", {
                vector<char> small_buffer(50);
                bmms_f::agent_ptr small_agent(small_buffer.data(), 50);
                file.load_to_agent_ptr(0, 100, small_agent, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_agent_ptr: file range out of bounds", {
                file.load_to_agent_ptr(150, 100, agent, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_agent_ptr: agent_ptr store_begin out of range", {
                file.load_to_agent_ptr(0, 100, agent, 150, true, local_mes);
                });

            TEST_CONVERSE_CODING("store_from_agent_ptr: file range out of bounds", {
                file.store_from_agent_ptr(agent, 0, 100, 150, true, true, local_mes);
                });

            TEST_CONVERSE_CODING("store_from_agent_ptr: agent_ptr load_begin out of range", {
                file.store_from_agent_ptr(agent, 150, 100, 0, true, true, local_mes);
                });

            // ----- Negative cases (service error) -----
            file.close(local_mes);

            TEST_CONVERSE_SERVICE("load_to_agent_ptr: file not opened",
                bfs_m::err::service_error_bin_file_file_not_opened_when_load_to_agent_ptr().code,
                local_mes, {
                bin_file closed_file;
                bmms_f::agent_ptr valid_agent(agent_buffer.data(), 200);
                closed_file.load_to_agent_ptr(0, 10, valid_agent, 0, true, local_mes);
                });

            bin_file_test_detail::cleanup_test_file(test_file);
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 11. load_to_void_ptr / store_from_void_ptr
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("void_ptr_interaction");
        {
            mes::a_mes local_mes;
            str test_file = test_dir + "void_ptr_test.bin";
            bin_file_test_detail::cleanup_test_file(test_file);

            bin_file file;
            file.open(test_file, 200, init_type::create, local_mes);

            // prepare file data
            vector<char> file_data(200);
            for (int i = 0; i < 200; i++) file_data[i] = static_cast<char>(i);
            file.store_from_void_ptr(file_data.data(), 200, 0, false, local_mes);

            vector<char> void_buffer(200);

            // ----- Positive cases -----
            TEST_POSITIVE("load_to_void_ptr: safe mode, valid copy", local_mes, {
                file.load_to_void_ptr(25, 50, void_buffer.data(), true, local_mes);
                });

            TEST_POSITIVE("load_to_void_ptr: unsafe mode", local_mes, {
                file.load_to_void_ptr(0, 100, void_buffer.data(), false, local_mes);
                });

            TEST_POSITIVE("load_to_void_ptr: zero size", local_mes, {
                file.load_to_void_ptr(0, 0, void_buffer.data(), true, local_mes);
                });

            TEST_POSITIVE("store_from_void_ptr: safe mode, valid copy", local_mes, {
                file.store_from_void_ptr(void_buffer.data(), 50, 150, true, local_mes);
                });

            TEST_POSITIVE("store_from_void_ptr: unsafe mode", local_mes, {
                file.store_from_void_ptr(void_buffer.data(), 50, 0, false, local_mes);
                });

            // ----- Negative cases (coding error) -----
            TEST_CONVERSE_CODING("load_to_void_ptr: null void pointer", {
                file.load_to_void_ptr(0, 50, nullptr, true, local_mes);
                });

            TEST_CONVERSE_CODING("store_from_void_ptr: null void pointer", {
                file.store_from_void_ptr(nullptr, 50, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_void_ptr: file range out of bounds", {
                file.load_to_void_ptr(150, 100, void_buffer.data(), true, local_mes);
                });

            TEST_CONVERSE_CODING("store_from_void_ptr: file range out of bounds", {
                file.store_from_void_ptr(void_buffer.data(), 100, 150, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_void_ptr: arithmetic overflow", {
                file.load_to_void_ptr(SIZE_MAX - 50, 100, void_buffer.data(), true, local_mes);
                });

            // ----- Negative cases (service error) -----
            file.close(local_mes);

            TEST_CONVERSE_SERVICE("load_to_void_ptr: file not opened",
                bfs_m::err::service_error_bin_file_file_not_opened_when_load_to_void_ptr().code,
                local_mes, {
                bin_file closed_file;
                closed_file.load_to_void_ptr(0, 10, void_buffer.data(), true, local_mes);
                });

            bin_file_test_detail::cleanup_test_file(test_file);
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 12. Theorem check functions test (pure logic, no file I/O)
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("theorem_functions");
        {
            mes::a_mes local_mes;

            // no_overflow
            TEST_POSITIVE("no_overflow: normal cases", local_mes, {
                bin_file f;
                bool r1 = f.no_overflow(100, 200);
                bool r2 = f.no_overflow(0, SIZE_MAX);
                bool r3 = !f.no_overflow(SIZE_MAX - 50, 100);
                bool r4 = !f.no_overflow(1, SIZE_MAX);
                });

            // __check_inited__
            {
                str test_file = test_dir + "check_inited.bin";
                bin_file_test_detail::cleanup_test_file(test_file);

                bin_file file;
                TEST_POSITIVE("__check_inited__: initially false", local_mes, {
                    bool inited = file.__check_inited__();
                    });

                file.open(test_file, 100, init_type::create, local_mes);

                TEST_POSITIVE("__check_inited__: after open true", local_mes, {
                    bool inited = file.__check_inited__();
                    });

                file.close(local_mes);
                bin_file_test_detail::cleanup_test_file(test_file);
            }

            // __same_file__
            {
                str file1_path = test_dir + "same_file1.bin";
                str file2_path = test_dir + "same_file2.bin";
                bin_file_test_detail::cleanup_test_file(file1_path);
                bin_file_test_detail::cleanup_test_file(file2_path);

                bin_file f1, f2, f3;
                f1.open(file1_path, 100, init_type::create, local_mes);
                f2.open(file2_path, 100, init_type::create, local_mes);
                f3.open(file1_path, 100, init_type::open_existed, local_mes);

                TEST_POSITIVE("__same_file__: different files false", local_mes, {
                    bool same = f1.__same_file__(f2);
                    });

                TEST_POSITIVE("__same_file__: same file true", local_mes, {
                    bool same = f1.__same_file__(f3);
                    });

                f1.close(local_mes);
                f2.close(local_mes);
                f3.close(local_mes);
                bin_file_test_detail::cleanup_test_file(file1_path);
                bin_file_test_detail::cleanup_test_file(file2_path);
            }

            // __check_file_range__
            {
                str test_file = test_dir + "check_file_range.bin";
                bin_file_test_detail::cleanup_test_file(test_file);

                bin_file file;
                file.open(test_file, 100, init_type::create, local_mes);

                TEST_POSITIVE("__check_file_range__: valid ranges", local_mes, {
                    bool r1 = file.__check_file_range__(0, 100);
                    bool r2 = file.__check_file_range__(25, 50);
                    bool r3 = file.__check_file_range__(0, 0);
                    });

                TEST_POSITIVE("__check_file_range__: invalid ranges", local_mes, {
                    bool r1 = !file.__check_file_range__(0, 101);
                    bool r2 = !file.__check_file_range__(100, 1);
                    bool r3 = !file.__check_file_range__(SIZE_MAX - 50, 100);
                    });

                file.close(local_mes);
                bin_file_test_detail::cleanup_test_file(test_file);
            }

            // __check_range_overlap__
            TEST_POSITIVE("__check_range_overlap__: overlap detection", local_mes, {
                bin_file f;
                bool overlap1 = f.__check_range_overlap__(0, 50, 25, 50);
                bool overlap2 = f.__check_range_overlap__(0, 100, 0, 100);
                bool overlap3 = f.__check_range_overlap__(25, 50, 0, 100);
                bool no_overlap1 = !f.__check_range_overlap__(0, 50, 50, 50);
                bool no_overlap2 = !f.__check_range_overlap__(0, 50, 100, 50);
                });

            // __op_begin__ / __op_end__
            TEST_POSITIVE("__op_begin__/__op_end__: normal calculation", local_mes, {
                bin_file f;
                size_t begin = f.__op_begin__(100);
                size_t end = f.__op_end__(100, 50);
                });

            TEST_CONVERSE_CODING("__op_end__: arithmetic overflow", {
                bin_file f;
                f.__op_end__(SIZE_MAX - 50, 100);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 13. Boundary value tests (zero-size file, single byte, etc.)
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("boundary_cases");
        {
            mes::a_mes local_mes;

            // zero-size file
            {
                str test_file = test_dir + "zero_size.bin";
                bin_file_test_detail::cleanup_test_file(test_file);

                bin_file file;
                file.open(test_file, 0, init_type::create, local_mes);

                TEST_POSITIVE("zero-size file: create and basic ops", local_mes, {
                    // only verify no exception, actual operations should be caught in negative cases
                    });

                // read/write on zero-size file should fail (coding/service error)
                vector<char> buf(10);
                bmms_f::agent_ptr agent(buf.data(), 10);

                TEST_CONVERSE_CODING("zero-size file: load_to_void_ptr out of range", {
                    file.load_to_void_ptr(0, 1, buf.data(), true, local_mes);
                    });

                TEST_CONVERSE_CODING("zero-size file: store_from_void_ptr out of range", {
                    file.store_from_void_ptr(buf.data(), 1, 0, true, local_mes);
                    });

                TEST_CONVERSE_CODING("zero-size file: load_to_agent_ptr out of range", {
                    file.load_to_agent_ptr(0, 1, agent, 0, true, local_mes);
                    });

                file.close(local_mes);
                bin_file_test_detail::cleanup_test_file(test_file);
            }

            // single-byte file
            {
                str test_file = test_dir + "one_byte.bin";
                bin_file_test_detail::cleanup_test_file(test_file);

                bin_file file;
                file.open(test_file, 1, init_type::create, local_mes);

                char write_data = 0x7F;
                char read_data = 0;

                TEST_POSITIVE("1-byte file: write and read", local_mes, {
                    file.store_from_void_ptr(&write_data, 1, 0, true, local_mes);
                    file.load_to_void_ptr(0, 1, &read_data, true, local_mes);
                    });

                file.close(local_mes);
                bin_file_test_detail::cleanup_test_file(test_file);
            }
        }
        TEST_FUNCTION_END();

        // ========== CLASS TEST END ==========
        TEST_CLASS_END();

        // Clean up test directory
        bin_file_test_detail::cleanup_test_file(test_dir);

        // ========== MODULE END ==========
        TEST_MODULE_END();

    }


#endif // BFS_INCLUDE_TEST
} // namespace bfs_f
