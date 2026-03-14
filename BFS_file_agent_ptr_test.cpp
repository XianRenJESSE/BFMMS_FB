// file_agent_ptr_test.cpp (完全重构版)
module BFS;

#include "BFS_dp.h"
#include "BMMS_dp.h"

#if defined(BFS_INCLUDE_TEST)
#include "TEST_helper.h"
#endif

namespace bfs_f {
#if defined(BFS_INCLUDE_TEST)

    // 显式命名空间，仅用于测试辅助函数
    namespace file_agent_ptr_test_detail {

        inline std::string get_test_dir() {
            return (std::filesystem::temp_directory_path() / "bfs_test" / "file_agent_ptr").string() + "/";
        }

        inline bool create_test_directory(const std::string& path) {
            try { std::filesystem::create_directories(path); return true; }
            catch (...) { return false; }
        }

        inline void cleanup_test_file(const std::string& path) {
            try { if (std::filesystem::exists(path)) std::filesystem::remove(path); }
            catch (...) {}
        }

        inline bool memory_equal(const void* p1, const void* p2, size_t size) {
            return memcmp(p1, p2, size) == 0;
        }

    } // namespace file_agent_ptr_test_detail

    void file_agent_ptr::test() {
        using namespace file_agent_ptr_test_detail;

        TEST_MODULE_BEGIN("BFS_AGENT_PTR");

        std::string test_dir = get_test_dir();
        create_test_directory(test_dir);

        TEST_CLASS_BEGIN("file_agent_ptr");

        // -----------------------------------------------------------------
        // 1. 构造函数测试
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Constructor");
        {
            mes::a_mes local_mes;

            // ----- 正例：正常构造 -----
            {
                std::string path = test_dir + "ctor_ok.bin";
                cleanup_test_file(path);
                auto bin = make_shared<bfs_f::bin_file>(
                    path, 1024, bfs_f::bin_file::init_type::create, local_mes
                );
                vector<byte_1> cache(1024);
                bmms_f::agent_ptr agent(cache.data(), 1024);

                TEST_POSITIVE("construct with valid parameters", local_mes, {
                    file_agent_ptr fap(bin, 0, 1024, agent, local_mes);
                    });

                bin->close(local_mes);
                cleanup_test_file(path);
            }

            // ----- 反例：编码错误（直接抛出异常）-----
            TEST_CONVERSE_CODING("construct with null bin_file shared_ptr", {
                shared_ptr<bfs_f::bin_file> null_bin;
                vector<byte_1> buf(1024);
                bmms_f::agent_ptr agent(buf.data(), 1024);
                file_agent_ptr fap(null_bin, 0, 1024, agent, _m);
                });

            TEST_CONVERSE_CODING("construct with zero size", {
                std::string p = test_dir + "ctor_zero.bin";
                cleanup_test_file(p);
                auto bin = make_shared<bfs_f::bin_file>(
                    p, 1024, bfs_f::bin_file::init_type::create, _m 
                );
                vector<byte_1> buf(1024);
                bmms_f::agent_ptr agent(buf.data(), 1024);
                file_agent_ptr fap(bin, 0, 0, agent, _m); 
                bin->close(_m);
                });

            // ----- 反例：服务错误（通过 mes_out 返回）-----
            {
                std::string path = test_dir + "ctor_not_opened.bin";
                cleanup_test_file(path);
                auto bin = make_shared<bfs_f::bin_file>(); // 未打开

                vector<byte_1> cache(1024);
                bmms_f::agent_ptr agent(cache.data(), 1024);

                TEST_CONVERSE_SERVICE("construct with unopened bin_file",
                    bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_construct().code,
                    local_mes, {
                    file_agent_ptr fap(bin, 0, 1024, agent, local_mes);
                    });
            }

            {
                std::string path = test_dir + "ctor_range_outside.bin";
                cleanup_test_file(path);
                auto bin = make_shared<bfs_f::bin_file>(
                    path, 1024, bfs_f::bin_file::init_type::create, local_mes
                );
                vector<byte_1> cache(1024);
                bmms_f::agent_ptr agent(cache.data(), 1024);

                TEST_CONVERSE_SERVICE("construct with file range out of bounds",
                    bfs_m::err::service_error_file_agent_ptr_file_range_outside_file_when_construct().code,
                    local_mes, {
                    file_agent_ptr fap(bin, 512, 1024, agent, local_mes); // 512+1024 > 1024
                    });

                bin->close(local_mes);
                cleanup_test_file(path);
            }

            {
                std::string path = test_dir + "ctor_null_agent.bin";
                cleanup_test_file(path);
                auto bin = make_shared<bfs_f::bin_file>(
                    path, 1024, bfs_f::bin_file::init_type::create, local_mes
                );
                bmms_f::agent_ptr null_agent;

                TEST_CONVERSE_SERVICE("construct with null agent_ptr",
                    bfs_m::err::service_error_file_agent_ptr_cache_agent_ptr_is_null_when_construct().code,
                    local_mes, {
                    file_agent_ptr fap(bin, 0, 1024, null_agent, local_mes);
                    });

                bin->close(local_mes);
                cleanup_test_file(path);
            }

            {
                std::string path = test_dir + "ctor_agent_too_small.bin";
                cleanup_test_file(path);
                auto bin = make_shared<bfs_f::bin_file>(
                    path, 1024, bfs_f::bin_file::init_type::create, local_mes
                );
                vector<byte_1> small(512);
                bmms_f::agent_ptr small_agent(small.data(), 512);

                TEST_CONVERSE_SERVICE("construct with agent too small",
                    bfs_m::err::service_error_file_agent_ptr_cache_agent_ptr_size_too_small_when_construct().code,
                    local_mes, {
                    file_agent_ptr fap(bin, 0, 1024, small_agent, local_mes);
                    });

                bin->close(local_mes);
                cleanup_test_file(path);
            }
        }
        TEST_FUNCTION_END();

        // -----------------------------------------------------------------
        // 2. 移动操作测试
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Move operations");
        {
            mes::a_mes local_mes;
            std::string path = test_dir + "move.bin";
            cleanup_test_file(path);

            auto bin = make_shared<bfs_f::bin_file>(
                path, 1024, bfs_f::bin_file::init_type::create, local_mes
            );
            vector<byte_1> cache(1024);
            bmms_f::agent_ptr agent(cache.data(), 1024);
            file_agent_ptr fap1(bin, 0, 1024, agent, local_mes);

            // 写入数据使其脏（使用外部数据源）
            vector<byte_1> external(1024, 0xAB);
            fap1.store_from_void_ptr(external.data(), 1024, 0, true, local_mes);

            TEST_POSITIVE("move constructor transfers resources and dirty flag", local_mes, {
                file_agent_ptr fap2 = move(fap1);
                bool inited = false, dirty = false;
                fap2.get_is_inited(inited);
                fap2.get_dirty_flag(dirty);
                if (!inited || !dirty) throw std::runtime_error("move constructor failed");
                });

            // 移动赋值
            std::string path2 = test_dir + "move_assign.bin";
            cleanup_test_file(path2);
            auto bin2 = make_shared<bfs_f::bin_file>(
                path2, 2048, bfs_f::bin_file::init_type::create, local_mes
            );
            vector<byte_1> cache2(2048);
            bmms_f::agent_ptr agent2(cache2.data(), 2048);
            file_agent_ptr fap3(bin2, 0, 2048, agent2, local_mes);

            vector<byte_1> external2(2048, 0xCD);
            fap3.store_from_void_ptr(external2.data(), 2048, 0, true, local_mes);

            TEST_POSITIVE("move assignment transfers resources", local_mes, {
                file_agent_ptr dest;
                dest = move(fap3);
                bool inited = false;
                dest.get_is_inited(inited);
                if (!inited) throw std::runtime_error("move assignment failed");
                });

            bin->close(local_mes);
            bin2->close(local_mes);
            cleanup_test_file(path);
            cleanup_test_file(path2);
        }
        TEST_FUNCTION_END();

        // -----------------------------------------------------------------
        // 3. clear / clear_range 测试
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("clear / clear_range");
        {
            mes::a_mes local_mes;
            std::string path = test_dir + "clear.bin";
            cleanup_test_file(path);

            auto bin = make_shared<bfs_f::bin_file>(
                path, 1024, bfs_f::bin_file::init_type::create, local_mes
            );
            vector<byte_1> cache(1024, 0xFF);
            bmms_f::agent_ptr agent(cache.data(), 1024);
            file_agent_ptr fap(bin, 0, 1024, agent, local_mes);

            TEST_POSITIVE("clear entire cache", local_mes, {
                fap.clear(local_mes);
                for (size_t i = 0; i < 1024; ++i)
                    if (cache[i] != 0) throw std::runtime_error("clear failed");
                });

            // 重新填充（使用外部数据源）
            vector<byte_1> external(1024, 0xAA);
            fap.store_from_void_ptr(external.data(), 1024, 0, true, local_mes);

            TEST_POSITIVE("clear_range valid range", local_mes, {
                fap.clear_range(256, 512, local_mes);
                for (size_t i = 256; i < 768; ++i)
                    if (cache[i] != 0) throw std::runtime_error("clear_range failed");
                for (size_t i = 0; i < 256; ++i)
                    if (cache[i] != 0xAA) throw std::runtime_error("clear_range corrupted before");
                for (size_t i = 768; i < 1024; ++i)
                    if (cache[i] != 0xAA) throw std::runtime_error("clear_range corrupted after");
                });

            // ----- 反例：编码错误（越界）-----
            TEST_CONVERSE_CODING("clear_range with overflow", {
                fap.clear_range(SIZE_MAX - 50, 100, local_mes);
                });

            TEST_CONVERSE_CODING("clear_range out of cache bounds", {
                fap.clear_range(512, 1024, local_mes); // 512+1024 > 1024
                });

            // ----- 反例：服务错误（未初始化）-----
            file_agent_ptr uninit;
            TEST_CONVERSE_SERVICE("clear on uninitialized object",
                bfs_m::err::coding_error_file_agent_ptr_not_inited_when_checking().code,
                local_mes, {
                uninit.clear(local_mes);
                });

            bin->close(local_mes);
            cleanup_test_file(path);
        }
        TEST_FUNCTION_END();

        // -----------------------------------------------------------------
        // 4. push / pull 缓存同步测试
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("push / pull cache synchronization");
        {
            mes::a_mes local_mes;
            std::string path = test_dir + "sync.bin";
            cleanup_test_file(path);

            auto bin = make_shared<bfs_f::bin_file>(
                path, 1024, bfs_f::bin_file::init_type::create, local_mes
            );
            if (local_mes.code != 0) {
                TEST_REPORT_ERROR("Failed to create bin_file");
                return;
            }

            vector<byte_1> cache(1024);
            bmms_f::agent_ptr agent(cache.data(), 1024);
            file_agent_ptr fap(bin, 0, 1024, agent, local_mes);

            // 准备文件数据
            vector<byte_1> file_data(1024);
            for (int i = 0; i < 1024; ++i) file_data[i] = static_cast<byte_1>(i);
            bin->store_from_void_ptr(file_data.data(), 1024, 0, true, local_mes);

            // 修改缓存（外部数据源）
            vector<byte_1> modified(1024, 0x55);
            fap.store_from_void_ptr(modified.data(), 1024, 0, true, local_mes);

            TEST_POSITIVE("push_cache_to_bin_file writes back dirty cache", local_mes, {
                fap.push_cache_to_bin_file(local_mes);
                vector<byte_1> verify(1024);
                bin->load_to_void_ptr(0, 1024, verify.data(), true, local_mes);
                if (!memory_equal(modified.data(), verify.data(), 1024))
                    throw std::runtime_error("push failed");
                bool dirty = true;
                fap.get_dirty_flag(dirty);
                if (dirty) throw std::runtime_error("dirty flag not cleared after push");
                });

            // 外部修改文件
            bin->store_from_void_ptr(file_data.data(), 1024, 0, true, local_mes);

            TEST_POSITIVE("pull_cache_from_bin_file updates cache", local_mes, {
                fap.pull_cache_from_bin_file(local_mes);
                if (!memory_equal(cache.data(), file_data.data(), 1024))
                    throw std::runtime_error("pull failed");
                bool dirty = true;
                fap.get_dirty_flag(dirty);
                if (dirty) throw std::runtime_error("dirty flag not cleared after pull");
                });

            // 缩小文件 - 检查是否成功
            bin->resize(512, local_mes);
            if (local_mes.code != 0) {
                TEST_REPORT_ERROR("resize failed");
            }
            else {
                // 调试输出 - 验证文件已缩小
                size_t fs = bin->__file_size__();
                size_t begin = 0, csize = 0;
                fap.get_cache_begin_about_bin_file(begin);
                fap.get_cache_size(csize);
                std::cout << "        [DEBUG] after resize - file_size: " << fs
                    << ", cache_begin: " << begin << ", cache_size: " << csize << "\n";

                // 反例：文件缩小后 push 应该返回服务错误
                TEST_CONVERSE_SERVICE("push after file shrunk",
                    bfs_m::err::service_error_file_agent_ptr_file_range_outside_file_when_push().code,
                    local_mes, {
                    fap.push_cache_to_bin_file(_m);
                    });
            }

            bin->close(local_mes);
            cleanup_test_file(path);
        }
        TEST_FUNCTION_END();

        // -----------------------------------------------------------------
        // 5. FAP <-> FAP 数据交换 (load / store)
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("FAP <-> FAP load/store");
        {
            mes::a_mes local_mes;
            std::string src_path = test_dir + "fap_src.bin";
            std::string dst_path = test_dir + "fap_dst.bin";
            cleanup_test_file(src_path);
            cleanup_test_file(dst_path);

            auto src_bin = make_shared<bfs_f::bin_file>(
                src_path, 2048, bfs_f::bin_file::init_type::create, local_mes
            );
            auto dst_bin = make_shared<bfs_f::bin_file>(
                dst_path, 2048, bfs_f::bin_file::init_type::create, local_mes
            );

            vector<byte_1> src_cache(2048);
            vector<byte_1> dst_cache(2048);
            bmms_f::agent_ptr src_agent(src_cache.data(), 2048);
            bmms_f::agent_ptr dst_agent(dst_cache.data(), 2048);

            file_agent_ptr fap_src(src_bin, 0, 2048, src_agent, local_mes);
            file_agent_ptr fap_dst(dst_bin, 0, 2048, dst_agent, local_mes);

            // 准备源数据
            vector<byte_1> source_data(2048);
            for (int i = 0; i < 2048; ++i) source_data[i] = static_cast<byte_1>(i % 256);
            fap_src.store_from_void_ptr(source_data.data(), 2048, 0, true, local_mes);

            TEST_POSITIVE("load: safe mode, non-overlapping copy", local_mes, {
                fap_src.load(0, 1024, fap_dst, 512, true, local_mes);
                for (int i = 0; i < 1024; ++i)
                    if (dst_cache[512 + i] != src_cache[i])
                        throw std::runtime_error("load data mismatch");
                bool dirty = false;
                fap_dst.get_dirty_flag(dirty);
                if (!dirty) throw std::runtime_error("dst dirty flag not set");
                });

            TEST_POSITIVE("store: wrapper of load", local_mes, {
                fap_dst.store(fap_src, 256, 512, 1024, true, local_mes);
                for (int i = 0; i < 512; ++i)
                    if (dst_cache[1024 + i] != src_cache[256 + i])
                        throw std::runtime_error("store data mismatch");
                });

            // ----- 反例：编码错误（越界、重叠、零大小）-----
            TEST_CONVERSE_CODING("load: zero size", {
                fap_src.load(0, 0, fap_dst, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load: source range overflow", {
                fap_src.load(SIZE_MAX - 50, 100, fap_dst, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load: destination range overflow", {
                fap_src.load(0, 100, fap_dst, SIZE_MAX - 50, true, local_mes);
                });

            TEST_CONVERSE_CODING("load: source out of cache bounds", {
                fap_src.load(1500, 1000, fap_dst, 0, true, local_mes); // 1500+1000 > 2048
                });

            TEST_CONVERSE_CODING("load: destination out of cache bounds", {
                fap_src.load(0, 100, fap_dst, 2000, true, local_mes); // 2000+100 > 2048
                });

            TEST_CONVERSE_CODING("load: same cache overlapping (safe mode)", {
                fap_src.load(0, 500, fap_src, 256, true, local_mes);
                });

            src_bin->close(local_mes);
            dst_bin->close(local_mes);
            cleanup_test_file(src_path);
            cleanup_test_file(dst_path);
        }
        TEST_FUNCTION_END();

        // -----------------------------------------------------------------
        // 6. FAP <-> agent_ptr 数据交换
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("FAP <-> agent_ptr");
        {
            mes::a_mes local_mes;
            std::string path = test_dir + "fap_agent.bin";
            cleanup_test_file(path);

            auto bin = make_shared<bfs_f::bin_file>(
                path, 1024, bfs_f::bin_file::init_type::create, local_mes
            );
            vector<byte_1> fap_cache(1024);
            bmms_f::agent_ptr fap_agent(fap_cache.data(), 1024);
            file_agent_ptr fap(bin, 0, 1024, fap_agent, local_mes);

            // 准备 FAP 数据
            vector<byte_1> source_data(1024);
            for (int i = 0; i < 1024; ++i) source_data[i] = static_cast<byte_1>(i % 2);
            fap.store_from_void_ptr(source_data.data(), 1024, 0, true, local_mes);

            

            // 目标 agent_ptr
            vector<byte_1> target_buf(2048);
            bmms_f::agent_ptr target_agent(target_buf.data(), 2048);

            TEST_POSITIVE("load_to_agent_ptr: safe mode", local_mes, {
                fap.load_to_agent_ptr(128, 256, target_agent, 1024, true, local_mes);
                for (int i = 0; i < 256; ++i)
                    if (target_buf[1024 + i] != fap_cache[128 + i])
                        throw std::runtime_error("load_to_agent_ptr failed");
                });

            // 源 agent_ptr
            vector<byte_1> src_buf(512, 0x77);
            bmms_f::agent_ptr src_agent(src_buf.data(), 512);

            TEST_POSITIVE("store_from_agent_ptr: safe mode", local_mes, {
                fap.store_from_agent_ptr(src_agent, 0, 256, 512, true, local_mes);
                for (int i = 0; i < 256; ++i)
                    if (fap_cache[512 + i] != 0x77)
                        throw std::runtime_error("store_from_agent_ptr failed");
                bool dirty = false;
                fap.get_dirty_flag(dirty);
                if (!dirty) throw std::runtime_error("dirty flag not set");
                });

            // ----- 反例：编码错误（越界、重叠、空指针）-----
            bmms_f::agent_ptr null_agent;
            TEST_CONVERSE_CODING("load_to_agent_ptr: null agent_ptr", {
                fap.load_to_agent_ptr(0, 50, null_agent, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_agent_ptr: agent buffer too small", {
                vector<byte_1> small(100);
                bmms_f::agent_ptr small_agent(small.data(), 100);
                fap.load_to_agent_ptr(0, 200, small_agent, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_agent_ptr: source out of cache bounds", {
                fap.load_to_agent_ptr(800, 500, target_agent, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_agent_ptr: destination out of agent bounds", {
                fap.load_to_agent_ptr(0, 500, target_agent, 1800, true, local_mes); // 1800+500 > 2048
                });

            TEST_CONVERSE_CODING("store_from_agent_ptr: destination out of cache bounds", {
                fap.store_from_agent_ptr(src_agent, 0, 256, 900, true, local_mes); // 900+256 > 1024
                });

            TEST_CONVERSE_CODING("store_from_agent_ptr: source out of agent bounds", {
                fap.store_from_agent_ptr(src_agent, 400, 200, 0, true, local_mes); // 400+200 > 512
                });

            TEST_CONVERSE_CODING("store_from_agent_ptr: null agent_ptr", {
                fap.store_from_agent_ptr(null_agent, 0, 100, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_agent_ptr: same memory overlapping (safe mode)", {
                // fap_cache 和 target_buf 不同，这里构造一个与自身缓存重叠的场景
                bmms_f::agent_ptr self_agent(fap_cache.data(), 1024);
                fap.load_to_agent_ptr(0, 512, self_agent, 256, true, local_mes);
                });

            bin->close(local_mes);
            cleanup_test_file(path);
        }
        TEST_FUNCTION_END();

        // -----------------------------------------------------------------
        // 7. FAP <-> void* 数据交换
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("FAP <-> void*");
        {
            mes::a_mes local_mes;
            std::string path = test_dir + "fap_void.bin";
            cleanup_test_file(path);

            auto bin = make_shared<bfs_f::bin_file>(
                path, 1024, bfs_f::bin_file::init_type::create, local_mes
            );
            vector<byte_1> fap_cache(1024);
            bmms_f::agent_ptr fap_agent(fap_cache.data(), 1024);
            file_agent_ptr fap(bin, 0, 1024, fap_agent, local_mes);

            // 准备数据
            vector<byte_1> source_data(1024);
            for (int i = 0; i < 1024; ++i) source_data[i] = static_cast<byte_1>(i);
            fap.store_from_void_ptr(source_data.data(), 1024, 0, true, local_mes);

            vector<byte_1> buffer(512);
            TEST_POSITIVE("load_to_void_ptr: safe mode", local_mes, {
                fap.load_to_void_ptr(256, 128, buffer.data(), true, local_mes);
                if (!memory_equal(buffer.data(), fap_cache.data() + 256, 128))
                    throw std::runtime_error("load_to_void_ptr failed");
                });

            vector<byte_1> src_data(256, 0x33);
            TEST_POSITIVE("store_from_void_ptr: safe mode", local_mes, {
                fap.store_from_void_ptr(src_data.data(), 256, 512, true, local_mes);
                for (int i = 0; i < 256; ++i)
                    if (fap_cache[512 + i] != 0x33)
                        throw std::runtime_error("store_from_void_ptr failed");
                bool dirty = false;
                fap.get_dirty_flag(dirty);
                if (!dirty) throw std::runtime_error("dirty flag not set");
                });

            // ----- 反例：编码错误（越界、重叠、空指针）-----
            TEST_CONVERSE_CODING("load_to_void_ptr: null pointer", {
                fap.load_to_void_ptr(0, 10, nullptr, true, local_mes);
                });

            TEST_CONVERSE_CODING("store_from_void_ptr: null pointer", {
                fap.store_from_void_ptr(nullptr, 10, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_void_ptr: zero size", {
                fap.load_to_void_ptr(0, 0, buffer.data(), true, local_mes);
                });

            TEST_CONVERSE_CODING("store_from_void_ptr: zero size", {
                fap.store_from_void_ptr(src_data.data(), 0, 0, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_void_ptr: overflow", {
                fap.load_to_void_ptr(SIZE_MAX - 50, 100, buffer.data(), true, local_mes);
                });

            TEST_CONVERSE_CODING("store_from_void_ptr: overflow", {
                fap.store_from_void_ptr(src_data.data(), 100, SIZE_MAX - 50, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_void_ptr: out of cache bounds", {
                fap.load_to_void_ptr(800, 500, buffer.data(), true, local_mes);
                });

            TEST_CONVERSE_CODING("store_from_void_ptr: out of cache bounds", {
                fap.store_from_void_ptr(src_data.data(), 256, 900, true, local_mes);
                });

            TEST_CONVERSE_CODING("load_to_void_ptr: overlapping (safe mode)", {
                fap.load_to_void_ptr(0, 512, fap_cache.data() + 256, true, local_mes);
                });

            TEST_CONVERSE_CODING("store_from_void_ptr: overlapping (safe mode)", {
                fap.store_from_void_ptr(fap_cache.data() + 256, 512, 0, true, local_mes);
                });

            bin->close(local_mes);
            cleanup_test_file(path);
        }
        TEST_FUNCTION_END();

        // -----------------------------------------------------------------
        // 8. 信息获取函数测试
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Information getters");
        {
            mes::a_mes local_mes;
            std::string path = test_dir + "getters.bin";
            cleanup_test_file(path);

            auto bin = make_shared<bfs_f::bin_file>(
                path, 2048, bfs_f::bin_file::init_type::create, local_mes
            );
            vector<byte_1> cache(1024);
            bmms_f::agent_ptr agent(cache.data(), 1024);
            file_agent_ptr fap(bin, 256, 1024, agent, local_mes);

            TEST_POSITIVE("get_is_inited returns true", local_mes, {
                bool inited = false;
                fap.get_is_inited(inited);
                if (!inited) throw std::runtime_error("get_is_inited false");
                });

            TEST_POSITIVE("get_cache_begin_about_bin_file", local_mes, {
                size_t begin = 0;
                fap.get_cache_begin_about_bin_file(begin);
                if (begin != 256) throw std::runtime_error("wrong begin");
                });

            TEST_POSITIVE("get_cache_agent_ptr", local_mes, {
                bmms_f::agent_ptr out;
                fap.get_cache_agent_ptr(out);
                bool is_null = true;
                out.get_is_null(is_null);
                if (is_null) throw std::runtime_error("got null agent");
                });

            TEST_POSITIVE("get_cache_raw_ptr", local_mes, {
                void* raw = nullptr;
                fap.get_cache_raw_ptr(raw);
                if (raw != cache.data()) throw std::runtime_error("raw ptr mismatch");
                });

            TEST_POSITIVE("get_cache_size", local_mes, {
                size_t sz = 0;
                fap.get_cache_size(sz);
                if (sz != 1024) throw std::runtime_error("cache size mismatch");
                });

            TEST_POSITIVE("get_dirty_flag", local_mes, {
                bool dirty = true;
                fap.get_dirty_flag(dirty);
                if (dirty) throw std::runtime_error("dirty should be false initially");
                });

            TEST_POSITIVE("get_associated_bin_file_shared_ptr", local_mes, {
                shared_ptr<bfs_f::bin_file> ptr;
                fap.get_associated_bin_file_shared_ptr(ptr);
                if (ptr != bin) throw std::runtime_error("associated file mismatch");
                });

            TEST_POSITIVE("check_associated_with_bin_file_shared_ptr", local_mes, {
                bool assoc = false;
                fap.check_associated_with_bin_file_shared_ptr(bin, assoc, local_mes);
                if (!assoc) throw std::runtime_error("association check failed");
                });

            TEST_POSITIVE("get_managed_file_size", local_mes, {
                size_t size = 0;
                fap.get_managed_file_size(size, local_mes);
                if (size != 2048 - 256) throw std::runtime_error("managed size wrong");
                });

            TEST_POSITIVE("get_cache_used_size", local_mes, {
                size_t used = 0;
                fap.get_cache_used_size(used, local_mes);
                if (used != 1024) throw std::runtime_error("used size wrong");
                });

            TEST_POSITIVE("check_file_still_valid", local_mes, {
                bool valid = false;
                fap.check_file_still_valid(valid, local_mes);
                if (!valid) throw std::runtime_error("file should be valid");
                });

            // 位置映射
            TEST_POSITIVE("calculate_cache_position", local_mes, {
                size_t cache_pos = 0;
                bool mappable = false;
                fap.calculate_cache_position(512, cache_pos, mappable, local_mes);
                if (!mappable || cache_pos != 256) throw std::runtime_error("cache position mapping failed");
                });

            TEST_POSITIVE("calculate_file_position", local_mes, {
                size_t file_pos = 0;
                bool mappable = false;
                fap.calculate_file_position(128, file_pos, mappable, local_mes);
                if (!mappable || file_pos != 384) throw std::runtime_error("file position mapping failed");
                });

            // ----- 反例：编码错误（未初始化）-----
            file_agent_ptr uninit;
            TEST_CONVERSE_CODING("check_associated_with_bin_file_shared_ptr on uninitialized", {
                bool assoc;
                uninit.check_associated_with_bin_file_shared_ptr(bin, assoc, local_mes);
                });

            // ----- 反例：服务错误（文件已关闭）-----
            bin->close(local_mes);

            TEST_CONVERSE_SERVICE("get_managed_file_size after file closed",
                bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_get_managed_size().code,
                local_mes, {
                size_t sz;
                fap.get_managed_file_size(sz, local_mes);
                });

            TEST_CONVERSE_SERVICE("check_file_still_valid after file closed",
                bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_check_validity().code,
                local_mes, {
                bool valid;
                fap.check_file_still_valid(valid, local_mes);
                });

            cleanup_test_file(path);
        }
        TEST_FUNCTION_END();

        // -----------------------------------------------------------------
        // 9. 定理检查函数测试（纯逻辑）
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Theorem check functions");
        {
            mes::a_mes local_mes;
            std::string path = test_dir + "theorem.bin";
            cleanup_test_file(path);

            auto bin = make_shared<bfs_f::bin_file>(
                path, 1024, bfs_f::bin_file::init_type::create, local_mes
            );
            vector<byte_1> cache(1024);
            bmms_f::agent_ptr agent(cache.data(), 1024);
            file_agent_ptr fap(bin, 0, 1024, agent, local_mes);

            TEST_POSITIVE("__check_self_valid__ on valid object", local_mes, {
                bool ok = fap.__check_self_valid__(local_mes);
                if (!ok) throw std::runtime_error("self valid check failed");
                });

            // 编码错误：未初始化
            file_agent_ptr uninit;
            TEST_CONVERSE_CODING("__check_self_valid__ on uninitialized", {
                uninit.__check_self_valid__(local_mes);
                });

            // 文件范围有效性（服务错误 + 编码错误）
            size_t valid_begin = 128, valid_size = 256;
            TEST_POSITIVE("__check_file_range_valid__ valid range", local_mes, {
                bool ok = fap.__check_file_range_valid__(*bin, valid_begin, valid_size, local_mes);
                if (!ok) throw std::runtime_error("file range valid failed");
                });

            // 文件未打开（服务错误）
            bin->close(local_mes);
            TEST_CONVERSE_SERVICE("__check_file_range_valid__ file not opened",
                bfs_m::err::service_error_file_agent_ptr_file_not_opened_when_check_file_range().code,
                local_mes, {
                fap.__check_file_range_valid__(*bin, valid_begin, valid_size, local_mes);
                });

            // 重新打开
            bin->open(path, 1024, bfs_f::bin_file::init_type::open_existed, local_mes);

            // 越界（编码错误）
            size_t invalid_begin = 800, invalid_size = 300;
            TEST_CONVERSE_CODING("__check_file_range_valid__ out of bounds", {
                fap.__check_file_range_valid__(*bin, invalid_begin, invalid_size, local_mes);
                });

            // 零大小（编码错误）
            size_t zero = 0;
            TEST_CONVERSE_CODING("__check_file_range_valid__ zero size", {
                fap.__check_file_range_valid__(*bin, valid_begin, zero, local_mes);
                });

            // 缓存范围有效性
            TEST_POSITIVE("__check_cache_range_valid__ valid range", local_mes, {
                bool ok = fap.__check_cache_range_valid__(valid_begin, valid_size, local_mes);
                if (!ok) throw std::runtime_error("cache range valid failed");
                });

            // 缓存越界（服务错误）
            size_t cache_out = 512, cache_size_out = 1024;
            TEST_CONVERSE_SERVICE("__check_cache_range_valid__ out of bounds",
                bfs_m::err::coding_error_file_agent_ptr_cache_range_outside_memory_when_operate().code,
                local_mes, {
                fap.__check_cache_range_valid__(cache_out, cache_size_out, local_mes);
                });

            // 两个FAP兼容性
            std::string path2 = test_dir + "theorem2.bin";
            cleanup_test_file(path2);
            auto bin2 = make_shared<bfs_f::bin_file>(
                path2, 1024, bfs_f::bin_file::init_type::create, local_mes
            );
            vector<byte_1> cache2(1024);
            bmms_f::agent_ptr agent2(cache2.data(), 1024);
            file_agent_ptr fap2(bin2, 0, 1024, agent2, local_mes);

            TEST_POSITIVE("__check_two_fap_compatible__", local_mes, {
                size_t self = 0, other = 0, sz = 100;
                bool ok = fap.__check_two_fap_compatible__(fap2, self, other, sz, local_mes);
                if (!ok) throw std::runtime_error("compatible check failed");
                });

            bin->close(local_mes);
            bin2->close(local_mes);
            cleanup_test_file(path);
            cleanup_test_file(path2);
        }
        TEST_FUNCTION_END();

        // -----------------------------------------------------------------
        // 10. 边界条件测试
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Boundary cases");
        {
            mes::a_mes local_mes;

            // 单字节 FAP
            {
                std::string path = test_dir + "boundary_1byte.bin";
                cleanup_test_file(path);
                auto bin = make_shared<bfs_f::bin_file>(
                    path, 1, bfs_f::bin_file::init_type::create, local_mes
                );
                vector<byte_1> cache(1);
                bmms_f::agent_ptr agent(cache.data(), 1);
                file_agent_ptr fap(bin, 0, 1, agent, local_mes);

                byte_1 data = 0x7F;
                TEST_POSITIVE("single byte store/load", local_mes, {
                    fap.store_from_void_ptr(&data, 1, 0, true, local_mes);
                    byte_1 read = 0;
                    fap.load_to_void_ptr(0, 1, &read, true, local_mes);
                    if (read != data) throw std::runtime_error("single byte mismatch");
                    });

                bin->close(local_mes);
                cleanup_test_file(path);
            }

            // 文件末尾开始的 FAP
            {
                std::string path = test_dir + "boundary_end.bin";
                cleanup_test_file(path);
                auto bin = make_shared<bfs_f::bin_file>(
                    path, 1024, bfs_f::bin_file::init_type::create, local_mes
                );
                vector<byte_1> cache(512);
                bmms_f::agent_ptr agent(cache.data(), 512);

                TEST_POSITIVE("construct from file end", local_mes, {
                    file_agent_ptr fap(bin, 512, 512, agent, local_mes);
                    });

                bin->close(local_mes);
                cleanup_test_file(path);
            }

            // 精确边界操作
            {
                std::string path = test_dir + "boundary_edge.bin";
                cleanup_test_file(path);
                auto bin = make_shared<bfs_f::bin_file>(
                    path, 1024, bfs_f::bin_file::init_type::create, local_mes
                );
                vector<byte_1> cache(1024);
                bmms_f::agent_ptr agent(cache.data(), 1024);
                file_agent_ptr fap(bin, 0, 1024, agent, local_mes);

                vector<byte_1> data(1024, 0xAA);

                TEST_POSITIVE("store at exact boundary", local_mes, {
                    fap.store_from_void_ptr(data.data(), 1024, 0, true, local_mes);
                    });

                TEST_POSITIVE("load at exact boundary", local_mes, {
                    vector<byte_1> out(1024);
                    fap.load_to_void_ptr(0, 1024, out.data(), true, local_mes);
                    if (!memory_equal(data.data(), out.data(), 1024))
                        throw std::runtime_error("boundary load failed");
                    });

                bin->close(local_mes);
                cleanup_test_file(path);
            }
        }
        TEST_FUNCTION_END();

        // -----------------------------------------------------------------
        // 11. 综合工作流测试
        // -----------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Integrated workflow");
        {
            mes::a_mes local_mes;
            std::string path = test_dir + "workflow.bin";
            cleanup_test_file(path);

            auto bin = make_shared<bfs_f::bin_file>(
                path, 4096, bfs_f::bin_file::init_type::create, local_mes
            );
            vector<byte_1> cache(2048);
            bmms_f::agent_ptr agent(cache.data(), 2048);
            file_agent_ptr fap(bin, 1024, 2048, agent, local_mes);

            // 1. 写入数据
            vector<byte_1> src1(2048);
            for (int i = 0; i < 2048; ++i) src1[i] = static_cast<byte_1>(i);
            fap.store_from_void_ptr(src1.data(), 2048, 0, true, local_mes);

            // 2. 推送
            fap.push_cache_to_bin_file(local_mes);
            if (local_mes.code != 0) throw std::runtime_error("push failed");

            // 3. 修改缓存并拉取恢复
            vector<byte_1> src2(2048, 0xFF);
            fap.store_from_void_ptr(src2.data(), 2048, 0, true, local_mes);
            fap.pull_cache_from_bin_file(local_mes);

            TEST_POSITIVE("cache restored after pull", local_mes, {
                if (!memory_equal(cache.data(), src1.data(), 2048))
                    throw std::runtime_error("cache not restored");
                bool dirty = true;
                fap.get_dirty_flag(dirty);
                if (dirty) throw std::runtime_error("dirty flag not cleared");
                });

            // 4. 清空
            fap.clear(local_mes);
            TEST_POSITIVE("clear succeeded", local_mes, {
                for (size_t i = 0; i < 2048; ++i)
                    if (cache[i] != 0) throw std::runtime_error("clear failed");
                });

            bin->close(local_mes);
            cleanup_test_file(path);
        }
        TEST_FUNCTION_END();

        TEST_CLASS_END();

        // 清理测试目录
        try { std::filesystem::remove_all(test_dir); }
        catch (...) {}

        TEST_MODULE_END();
    }

#endif // BFS_INCLUDE_TEST

} // namespace bfs_f
