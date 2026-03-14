// BMMS_free_part_test.cpp
module BMMS;

#include "BMMS_dp.h"
import MES;

#if defined(BMMS_INCLUDE_TEST)
#include "TEST_helper.h"
#endif
namespace bmms_f {
#if defined(BMMS_INCLUDE_TEST)
    // ========== free_part 单元测试 - 核心功能 ==========
    void free_part::test() {
        using namespace test_helper;

        TEST_MODULE_BEGIN("FREE_PART");
        TEST_CLASS_BEGIN("free_part");

        // ------------------------------------------------------------
        // 1. 构造函数测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Constructor");
        {
            mes::a_mes local_mes;

            // ----- 正例：正常构造 -----
            TEST_POSITIVE("construct with valid parameters (size=64, count=10, no_init)", local_mes, {
                free_part fp(64, 10, free_part::init_type::no_init_all_size, _m);
                if (!fp.is_init) throw int();
                if (fp.get_block_count_max() != 10) throw int();
                if (fp.get_block_size() != 64) throw int();
                if (fp.get_block_count_used() != 0) throw int();
                if (fp.get_total_size() != 640) throw int();
                });

            TEST_POSITIVE("construct with init_all_size (all blocks pre-allocated)", local_mes, {
                free_part fp(64, 5, free_part::init_type::init_all_size, _m);
                if (!fp.is_init) throw int();
                if (fp.get_block_count_used() != 5) throw int();  // 所有块已分配

                // 验证每个块都可访问
                for (size_t i = 0; i < 5; i++) {
                    bmms_f::agent_ptr block_ap;
                    bool is_null = true;
                    fp.get_block_ptr(i, block_ap, is_null);
                    if (is_null) throw int();
                    if (block_ap.__self_begin__() == 0) throw int();
                }
                });

            // ----- 反例：编码错误 -----
            TEST_CONVERSE_CODING("block size = 0", {
                free_part fp(0, 10, free_part::init_type::no_init_all_size, _m);
                });

            TEST_CONVERSE_CODING("block count = 0", {
                free_part fp(64, 0, free_part::init_type::no_init_all_size, _m);
                });

            TEST_CONVERSE_CODING("block count > bmms_f::index_max", {
                free_part fp(64, bmms_f::index_max + 1, free_part::init_type::no_init_all_size, _m);
                });

            TEST_CONVERSE_CODING("invalid init_type (2)", {
                free_part fp(64, 10, static_cast<free_part::init_type>(2), _m);
                });

            // ----- 服务错误：内存分配失败（在init_all_size时）-----
            {
                // 难以模拟，留空
            }
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 2. 移动构造函数
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Move Constructor");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("move construct, source becomes invalid, target works", local_mes, {
                free_part src(64, 5, free_part::init_type::no_init_all_size, _m);
                free_part dst(move(src));

                if (!dst.is_init) throw int();
                if (src.is_init) throw int();  // 源对象应无效
                if (dst.get_block_count_max() != 5) throw int();
                if (dst.get_block_size() != 64) throw int();
                if (dst.get_block_count_used() != 0) throw int();
                });

            TEST_POSITIVE("move construct with allocated blocks", local_mes, {
                free_part src(64, 5, free_part::init_type::init_all_size, _m);
                size_t used_before = src.get_block_count_used();

                free_part dst(move(src));

                if (!dst.is_init) throw int();
                if (dst.get_block_count_used() != used_before) throw int();  // 使用计数应转移
                });

            TEST_CONVERSE_CODING("call get_block_ptr on moved-from object", {
                free_part src(64, 5, free_part::init_type::no_init_all_size, _m);
                free_part dst(move(src));
                bmms_f::agent_ptr ap;
                bool is_null;
                size_t block_index = 0;
                src.get_block_ptr(block_index, ap, is_null);  // 源对象未初始化 → 编码错误
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 3. 移动赋值运算符
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Move Assignment");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("move assign, target gets resources, source becomes invalid", local_mes, {
                free_part src(64, 5, free_part::init_type::init_all_size, _m);
                free_part dst(128, 3, free_part::init_type::no_init_all_size, _m);

                size_t src_used = src.get_block_count_used();
                dst = move(src);

                if (!dst.is_init) throw int();
                if (src.is_init) throw int();
                if (dst.get_block_size() != 64) throw int();
                if (dst.get_block_count_max() != 5) throw int();
                if (dst.get_block_count_used() != src_used) throw int();  // 使用计数应转移
                });

            TEST_CONVERSE_CODING("call free_block on moved-from object", {
                free_part src(64, 5, free_part::init_type::no_init_all_size, _m);
                free_part dst(128, 3, free_part::init_type::no_init_all_size, _m);
                dst = move(src);
                src.free_block(0);  // 源对象未初始化 → 编码错误
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 4. Getter 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Getters");
        {
            mes::a_mes local_mes;
            free_part fp(64, 10, free_part::init_type::no_init_all_size, local_mes);

            TEST_POSITIVE("get_block_count_max returns constructor argument", local_mes, {
                if (fp.get_block_count_max() != 10) throw int();
                });

            TEST_POSITIVE("get_block_size returns constructor argument", local_mes, {
                if (fp.get_block_size() != 64) throw int();
                });

            TEST_POSITIVE("get_total_size returns block_size * block_count_max", local_mes, {
                if (fp.get_total_size() != 640) throw int();
                });

            TEST_POSITIVE("get_block_count_used initially 0 for no_init", local_mes, {
                if (fp.get_block_count_used() != 0) throw int();
                });

            TEST_CONVERSE_CODING("getters on uninitialized volume", {
                free_part src(64, 5, free_part::init_type::no_init_all_size, _m);
                free_part dst(move(src));
                src.get_block_count_max();  // 源对象未初始化 → 编码错误
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 5. get_block_ptr 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_block_ptr");
        {
            mes::a_mes local_mes;
            free_part fp(64, 10, free_part::init_type::init_all_size, local_mes);

            TEST_POSITIVE("get valid block pointer", local_mes, {
                bmms_f::agent_ptr ap;
                bool is_null = true;
                size_t block_index = 0;
                fp.get_block_ptr(block_index, ap, is_null);
                if (is_null) throw int();
                if (ap.__self_begin__() == 0) throw int();
                if (ap.size != 64) throw int();
                });

            TEST_POSITIVE("get null pointer for unallocated block", local_mes, {
                free_part fp2(64, 10, free_part::init_type::no_init_all_size, _m);
                bmms_f::agent_ptr ap;
                bool is_null = false;
                size_t block_index = 5;
                fp2.get_block_ptr(block_index, ap, is_null);
                if (!is_null) throw int();  // 应为空
                if (ap.__self_begin__() != 0) throw int();  // 空指针
                });

            TEST_CONVERSE_CODING("block index out of range", {
                free_part fp(64, 10, free_part::init_type::no_init_all_size, _m);
                bmms_f::agent_ptr ap;
                bool is_null;
                size_t bad_index = fp.get_block_count_max();
                fp.get_block_ptr(bad_index, ap, is_null);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 6. store 触发自动分配测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("store - auto allocation");
        {
            mes::a_mes local_mes;
            free_part fp(64, 5, free_part::init_type::no_init_all_size, local_mes);

            TEST_POSITIVE("store to unallocated block triggers allocation", local_mes, {
                byte_1 src_data[64] = {1, 2, 3};  // 测试数据
                bmms_f::agent_ptr src_ap(src_data, 64);

                // 初始使用计数为0
                if (fp.get_block_count_used() != 0) throw int();

                // 写入块0
                fp.store(src_ap, 0, 64, 0, true);

                // 应自动分配块，使用计数+1
                if (fp.get_block_count_used() != 1) throw int();

                // 验证块已分配并可读回
                byte_1 dst_data[64] = {0};
                bmms_f::agent_ptr dst_ap(dst_data, 64);
                fp.load(0, 64, dst_ap, 0, true);

                // 验证数据一致
                if (dst_data[0] != 1 || dst_data[1] != 2 || dst_data[2] != 3) throw int();
                });

            TEST_POSITIVE("store to allocated block does not increase count", local_mes, {
                byte_1 src_data[64] = {0};
                bmms_f::agent_ptr src_ap(src_data, 64);

                // 第一次写入 → 分配
                fp.store(src_ap, 0, 64, 0, true);
                size_t used_after_first = fp.get_block_count_used();

                // 第二次写入 → 不分配
                fp.store(src_ap, 0, 64, 0, true);
                size_t used_after_second = fp.get_block_count_used();

                if (used_after_second != used_after_first) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 7. free_block 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("free_block");
        {
            mes::a_mes local_mes;
            free_part fp(64, 5, free_part::init_type::init_all_size, local_mes);

            TEST_POSITIVE("free an allocated block", local_mes, {
                size_t used_before = fp.get_block_count_used();  // 应为5
                fp.free_block(0);
                size_t used_after = fp.get_block_count_used();   // 应为4

                if (used_after != used_before - 1) throw int();

                // 验证块已释放
                bmms_f::agent_ptr ap;
                bool is_null = false;
                size_t block_index = 0;
                fp.get_block_ptr(block_index, ap, is_null);
                if (!is_null) throw int();  // 应为空
                });

            TEST_POSITIVE("free already freed block (no effect)", local_mes, {
                fp.free_block(0);  // 第一次释放
                size_t used_before = fp.get_block_count_used();
                fp.free_block(0);  // 第二次释放
                size_t used_after = fp.get_block_count_used();

                if (used_after != used_before) throw int();  // 不应变化
                });

            TEST_CONVERSE_CODING("free with invalid index", {
                free_part fp2(64, 5, free_part::init_type::init_all_size, _m);
                size_t bad_index = fp2.get_block_count_max();
                fp2.free_block(bad_index);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 8. free_range 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("free_range");
        {
            mes::a_mes local_mes;
            free_part fp(64, 10, free_part::init_type::init_all_size, local_mes);

            TEST_POSITIVE("free range of complete blocks", local_mes, {
                size_t used_before = fp.get_block_count_used();  // 10

            // 释放块2-4（共3个块）
            fp.free_range(128, 192);  // 块2: 128-191, 块3: 192-255, 块4: 256-319

            size_t used_after = fp.get_block_count_used();  // 应为7

            if (used_after != used_before - 3) throw int();

            // 验证块2-4已释放
            for (size_t i = 2; i <= 4; i++) {
                bmms_f::agent_ptr ap;
                bool is_null = false;
                fp.get_block_ptr(i, ap, is_null);
                if (!is_null) throw int();
            }
                });

            TEST_POSITIVE("free range with partial blocks (should not free)", local_mes, {
                free_part fp2(64, 5, free_part::init_type::init_all_size, _m);
                size_t used_before = fp2.get_block_count_used();

                // 范围只覆盖块0的一部分
                fp2.free_range(32, 64);  // 块0的32-95，跨越到块1开始

                size_t used_after = fp2.get_block_count_used();
                if (used_after != used_before) throw int();  // 不应释放任何块
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 9. resize 测试 - 缩小
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("resize - shrink");
        {
            mes::a_mes local_mes;
            free_part fp(64, 10, free_part::init_type::init_all_size, local_mes);

            TEST_POSITIVE("shrink to smaller size, extra blocks freed", local_mes, {
                size_t used_before = fp.get_block_count_used();  // 10

                fp.resize(5, _m);

                if (fp.get_block_count_max() != 5) throw int();
                size_t used_after = fp.get_block_count_used();  // 5（前5个块保留）
                if (used_after != 5) throw int();

                // 验证前5个块仍存在
                for (size_t i = 0; i < 5; i++) {
                    bmms_f::agent_ptr ap;
                    bool is_null = true;
                    fp.get_block_ptr(i, ap, is_null);
                    if (is_null) throw int();
                }

                // 验证后5个块已不可访问（索引越界）
                TEST_CONVERSE_CODING("access freed block after shrink", {
                    bmms_f::agent_ptr ap;
                    bool is_null;
                    size_t block_index = 7;
                    fp.get_block_ptr(block_index, ap, is_null);  // 索引7已超出新大小
                });
                });

            size_t expected_code = bmms_m::err::service_err_free_part_resize_block_count_zero().code;
            TEST_CONVERSE_SERVICE("resize to zero", expected_code, local_mes, {
                free_part fp2(64, 5, free_part::init_type::init_all_size, _m);
                fp2.resize(0, _m);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 10. resize 测试 - 扩大
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("resize - expand");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("expand with init_all_size, new blocks allocated", local_mes, {
                free_part fp(64, 5, free_part::init_type::init_all_size, _m);
                size_t used_before = fp.get_block_count_used();  // 5

                fp.resize(8, _m);

                if (fp.get_block_count_max() != 8) throw int();
                size_t used_after = fp.get_block_count_used();  // 8（所有块都已分配）
                if (used_after != 8) throw int();

                // 验证新块可访问
                for (size_t i = 5; i < 8; i++) {
                    bmms_f::agent_ptr ap;
                    bool is_null = true;
                    fp.get_block_ptr(i, ap, is_null);
                    if (is_null) throw int();
                }
                });

            TEST_POSITIVE("expand with no_init_all_size, new blocks unallocated", local_mes, {
                free_part fp(64, 5, free_part::init_type::no_init_all_size, _m);
                size_t used_before = fp.get_block_count_used();  // 0

                fp.resize(8, _m);

                if (fp.get_block_count_max() != 8) throw int();
                size_t used_after = fp.get_block_count_used();  // 0（未分配）
                if (used_after != 0) throw int();

                // 验证新块为空
                for (size_t i = 5; i < 8; i++) {
                    bmms_f::agent_ptr ap;
                    bool is_null = false;
                    fp.get_block_ptr(i, ap, is_null);
                    if (!is_null) throw int();  // 应为空
                }
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 11. load/store 跨块读写测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("cross-block load/store");
        {
            mes::a_mes local_mes;
            free_part fp(64, 5, free_part::init_type::init_all_size, local_mes);

            TEST_POSITIVE("write across block boundary", local_mes, {
                // 准备150字节数据
                byte_1 src_data[150];
                for (int i = 0; i < 150; i++) src_data[i] = (byte_1)(i % 256);
                bmms_f::agent_ptr src_ap(src_data, 150);

                // 从块0偏移50开始写入150字节
                // 应跨越块0(50-63)、块1(0-63)、块2(0-85)
                fp.store(src_ap, 0, 150, 50, true);

                // 读回验证
                byte_1 dst_data[150] = {0};
                bmms_f::agent_ptr dst_ap(dst_data, 150);
                fp.load(50, 150, dst_ap, 0, true);

                // 验证数据一致
                for (int i = 0; i < 150; i++) {
                    if (dst_data[i] != (byte_1)(i % 256)) throw int();
                }
                });

            TEST_POSITIVE("read from unallocated blocks returns zeros", local_mes, {
                free_part fp2(64, 5, free_part::init_type::no_init_all_size, _m);

                byte_1 dst_data[128] = {0xFF};  // 初始化为非0
                bmms_f::agent_ptr dst_ap(dst_data, 128);

                // 从未分配的块0-1读取128字节
                fp2.load(0, 128, dst_ap, 0, true);

                // 所有字节应为0
                for (int i = 0; i < 128; i++) {
                    if (dst_data[i] != 0) throw int();
                }
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 12. clear / clear_range 测试
        // ------------------------------------------------------------
        mes::a_mes local_mes = mes::a_mes();
        TEST_POSITIVE("clear range", local_mes, {
            free_part fp2(64, 5, free_part::init_type::init_all_size, _m);

        // 1. 准备测试数据：全写0xAA
        byte_1 test_data[64];
        memset(test_data, 0xAA, 64);
        bmms_f::agent_ptr test_ap(test_data, 64);

        // 2. 写入块0和块1
        fp2.store(test_ap, 0, 64, 0, true);   // 块0偏移0
        fp2.store(test_ap, 0, 64, 64, true);  // 块1偏移0

        // 3. 清除块0的[32,64)范围
        fp2.clear_range(32, 32);

        // 4. 验证块0前半部分[0,32)未被清除（应为0xAA）
        byte_1 read_head[32];
        bmms_f::agent_ptr read_head_ap(read_head, 32);
        fp2.load(0, 32, read_head_ap, 0, true);
        for (int i = 0; i < 32; i++) {
            if (read_head[i] != 0xAA) {
                std::cout << "        [DEBUG] block0前半部分[" << i << "] = "
                          << (int)read_head[i] << ", expected 0xAA\n";
                throw int();
            }
        }

        // 5. 验证块0后半部分[32,64)已被清除（应为0）
        byte_1 read_tail[32];
        bmms_f::agent_ptr read_tail_ap(read_tail, 32);
        fp2.load(32, 32, read_tail_ap, 0, true);
        for (int i = 0; i < 32; i++) {
            if (read_tail[i] != 0) {
                std::cout << "        [DEBUG] block0后半部分[" << i << "] = "
                          << (int)read_tail[i] << ", expected 0\n";
                throw int();
            }
        }

        // 6. 验证块1未被影响（仍为0xAA）
        byte_1 read_block1[64];
        bmms_f::agent_ptr read_block1_ap(read_block1, 64);
        fp2.load(64, 64, read_block1_ap, 0, true);
        for (int i = 0; i < 64; i++) {
            if (read_block1[i] != 0xAA) {
                std::cout << "        [DEBUG] block1[" << i << "] = "
                          << (int)read_block1[i] << ", expected 0xAA\n";
                throw int();
            }
        }
            });

        TEST_CLASS_END();
        TEST_MODULE_END();
    }

#endif // BMMS_INCLUDE_TEST

} // namespace bmms_f
