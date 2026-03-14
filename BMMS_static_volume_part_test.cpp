// BMMS_static_volume_part_test.cpp
module BMMS;

#include "BMMS_dp.h"
import MES;

#if defined(BMMS_INCLUDE_TEST)
#include "TEST_helper.h"
#endif

namespace bmms_f {
#if defined(BMMS_INCLUDE_TEST)

    void static_volume_part::test() {
        using namespace test_helper;

        TEST_MODULE_BEGIN("STATIC_VOLUME_PART");
        TEST_CLASS_BEGIN("static_volume_part");

        // ------------------------------------------------------------
        // 1. 构造函数测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Constructor");
        {
            mes::a_mes local_mes;

            // ----- 准备一个静态卷 -----
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );
            if (local_mes.code != 0) throw int();

            // ----- 正例1: no_init, not_allow_over_use -----
            TEST_POSITIVE("construct with no_init, not_allow_over_use", local_mes, {
                static_volume_part part(volume, 10,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                if (!part.is_init) throw int();
                if (part.block_count_max != 10) throw int();
                if (part.using_block_count != 0) throw int();
                if (part.init_type_ != init_type::no_init_all_size) throw int();
                if (part.control_type_ != control_type::not_allow_over_use) throw int();
                });

            // ----- 正例2: init_all, not_allow_over_use (空闲块足够) -----
            TEST_POSITIVE("construct with init_all, not_allow_over_use (enough free blocks)", local_mes, {
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                if (!part.is_init) throw int();
                if (part.using_block_count != 5) throw int();  // 所有块已分配
                // 验证块指针非空
                for (size_t i = 0; i < 5; ++i) {
                    void* ptr = nullptr;
                    part.block_ptr_ram_space_agent_ptr.load_to_void_ptr(i * 8, 8, &ptr, true);
                    if (ptr == nullptr) throw int();
                }
                });

            // ----- 正例3: allow_over_use (允许超过卷最大块数) -----
            TEST_POSITIVE("construct with allow_over_use (exceed volume max)", local_mes, {
                // 卷最大块数20，这里请求30，应成功
                static_volume_part part(volume, 30,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::allow_over_use,
                    _m);
                if (!part.is_init) throw int();
                if (part.block_count_max != 30) throw int();
                });

            // ----- 反例1: 空 shared_ptr -----
            TEST_CONVERSE_CODING("null shared_ptr", {
                shared_ptr<static_volume> null_ptr;
                static_volume_part part(null_ptr, 10,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                });

            // ----- 反例2: 卷未初始化 -----
            
            TEST_CONVERSE_CODING("volume not initialized (moved-from)", {
                mes::a_mes tmp;
                // 创建原始卷
                static_volume o_vol(64, 20,
                    static_volume::init_type::no_init_all_size,
                    static_volume::alignment_type::without_alignment, tmp);

                // 移动原始卷，并尝试使用它
                static_volume moved_vol = move(o_vol);

                //制造原始卷的指针
                shared_ptr<static_volume> o_vol_ptr = make_shared<static_volume>(move(o_vol));

                static_volume_part part(o_vol_ptr, 10,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use, tmp);
                tmp.out();
                output<<"moved then: " << o_vol_ptr->get_is_init() << endl;
                });
            
            // ----- 反例3: 无效 init_type -----
            TEST_CONVERSE_CODING("invalid init_type", {
                static_volume_part part(volume, 10,
                    static_cast<init_type>(99),
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                });

            // ----- 反例4: 无效 control_type -----
            TEST_CONVERSE_CODING("invalid control_type", {
                static_volume_part part(volume, 10,
                    static_volume_part::init_type::no_init_all_size,
                    static_cast<control_type>(99),
                    _m);
                });

            // ----- 反例5: block_count_max = 0 -----
            TEST_CONVERSE_CODING("block count zero", {
                static_volume_part part(volume, 0,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                });

            // ----- 反例6: block_count_max > index_max -----
            TEST_CONVERSE_CODING("block count > index_max", {
                static_volume_part part(volume, bmms_f::index_max + 1,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                });

            // ----- 反例7: init_all_size 且 allow_over_use (编码错误) -----
            TEST_CONVERSE_CODING("init_all with allow_over_use (coding error)", {
                static_volume_part part(volume, 10,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::allow_over_use,
                    _m);
                });

            // ----- 服务错误1: init_all_size + not_allow_over_use 且卷空闲块不足 -----
            {
                // 先分配一些块，使卷的空闲块少于请求数
                mes::a_mes tmp_mes;
                bmms_f::agent_ptr blocks[15];
                for (int i = 0; i < 15; ++i) {
                    volume->new_block(blocks[i], tmp_mes);
                    if (tmp_mes.code != 0) throw int();
                }

                // 卷最大20，已分配15，空闲5
                size_t expected_code = bmms_m::err::service_err_static_volume_part_build_not_enough_free_blocks().code;
                TEST_CONVERSE_SERVICE("init_all with not_allow_over_use but not enough free blocks",
                    expected_code, local_mes, {
                    static_volume_part part(volume, 10,  // 需要10个空闲块，只有5个
                        static_volume_part::init_type::init_all_size,
                        static_volume_part::control_type::not_allow_over_use,
                        _m);
                    });

                // 正确释放块
                for (int i = 0; i < 15; ++i) {
                    volume->delete_block(blocks[i]);
                    if (tmp_mes.code != 0) throw int();
                }
            }
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 2. 移动构造函数
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Move Constructor");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );

            TEST_POSITIVE("move construct, source becomes invalid, target works", local_mes, {
                static_volume_part src(volume, 10,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                static_volume_part dst(move(src));

                if (!dst.is_init) throw int();
                if (src.is_init) throw int();      // 源对象应无效
                if (dst.block_count_max != 10) throw int();
                if (dst.using_block_count != 0) throw int();
                if (src.block_count_max != 0) throw int();
                });

            TEST_POSITIVE("move construct with allocated blocks", local_mes, {
                static_volume_part src(volume, 5,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                size_t used_before = src.using_block_count; // 5
                static_volume_part dst(move(src));

                if (!dst.is_init) throw int();
                if (dst.using_block_count != used_before) throw int();
                if (src.using_block_count != 0) throw int();
                });

            TEST_CONVERSE_CODING("call get_block_ptr on moved-from object", {
                static_volume_part src(volume, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                static_volume_part dst(move(src));
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
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );

            TEST_POSITIVE("move assign, target gets resources, source becomes invalid", local_mes, {
                static_volume_part src(volume, 5,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                static_volume_part dst(volume, 3,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);

                size_t src_used = src.using_block_count;
                dst = move(src);

                if (!dst.is_init) throw int();
                if (src.is_init) throw int();
                if (dst.using_block_count != src_used) throw int();
                if (dst.block_count_max != 5) throw int();
                });

            TEST_CONVERSE_CODING("call free_block on moved-from object", {
                static_volume_part src(volume, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                static_volume_part dst(volume, 3,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                dst = move(src);
                src.free_block(0);  // 源对象未初始化 → 编码错误
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 4. Getters 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Getters");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );
            static_volume_part part(volume, 10,
                static_volume_part::init_type::no_init_all_size,
                static_volume_part::control_type::not_allow_over_use,
                local_mes);

            TEST_POSITIVE("get_total_size returns block_size * block_count_max", local_mes, {
                if (part.get_total_size() != 64 * 10) throw int();
                });

            TEST_POSITIVE("get_block_count_max returns constructor argument", local_mes, {
                if (part.get_block_count_max() != 10) throw int();
                });

            TEST_POSITIVE("get_block_size returns volume's block size", local_mes, {
                if (part.get_block_size() != 64) throw int();
                });

            TEST_POSITIVE("get_block_count_used initially 0 for no_init", local_mes, {
                if (part.get_block_count_used() != 0) throw int();
                });

            TEST_CONVERSE_CODING("getters on uninitialized object", {
                mes::a_mes tmp;
                shared_ptr<static_volume> vol = make_shared<static_volume>(64, 5,
                    static_volume::init_type::no_init_all_size,
                    static_volume::alignment_type::without_alignment,
                    tmp);
                static_volume_part src(vol, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                static_volume_part dst(move(src));
                src.get_total_size();  // 源对象未初始化 → 编码错误
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 5. get_block_ptr 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_block_ptr");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );
            static_volume_part part(volume, 10,
                static_volume_part::init_type::init_all_size,  // 预分配所有块
                static_volume_part::control_type::not_allow_over_use,
                local_mes);

            TEST_POSITIVE("get valid block pointer", local_mes, {
                bmms_f::agent_ptr ap;
                bool is_null = true;
                size_t idx = 3;
                part.get_block_ptr(idx, ap, is_null);
                if (is_null) throw int();
                if (ap.__self_begin__() == 0) throw int();
                if (ap.size != 64) throw int();
                });

            TEST_POSITIVE("get null pointer for freed block", local_mes, {
                part.free_block(3);  // 释放块3
                bmms_f::agent_ptr ap;
                bool is_null = false;
                size_t idx = 3;
                part.get_block_ptr(idx, ap, is_null);
                if (!is_null) throw int();   // 应为空
                if (ap.__self_begin__() != 0) throw int();
                });

            TEST_CONVERSE_CODING("block index out of range", {
                size_t bad_idx = part.get_block_count_max();
                bmms_f::agent_ptr ap;
                bool is_null;
                part.get_block_ptr(bad_idx, ap, is_null);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 6. get_range_info 测试（内部函数，通过公有行为间接验证）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_range_info (via clear_range)");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );
            static_volume_part part(volume, 10,
                static_volume_part::init_type::init_all_size,
                static_volume_part::control_type::not_allow_over_use,
                local_mes);

            // 我们通过 clear_range 的行为来验证范围计算是否正确
            // 先向块0-1写入测试数据
            byte_1 test_data[128];
            for (int i = 0; i < 128; ++i) test_data[i] = (byte_1)(i % 256);
            bmms_f::agent_ptr src_ap(test_data, 128);
            part.store(src_ap, 0, 128, 0, true,local_mes);

            // 清除跨块范围（块0的后32字节 + 块1的全部64字节 + 块2的前32字节）
            part.clear_range(32, 128);  // 32..159

            // 验证块0的前32字节未被清除
            byte_1 read_buf0[32];
            bmms_f::agent_ptr dst_ap0(read_buf0, 32);
            part.load(0, 32, dst_ap0, 0, true,local_mes);
            for (int i = 0; i < 32; ++i) {
                if (read_buf0[i] != (byte_1)(i % 256)) throw int();
            }

            // 验证块0的32-63被清除
            byte_1 read_buf0_2[32];
            bmms_f::agent_ptr dst_ap0_2(read_buf0_2, 32);
            part.load(32, 32, dst_ap0_2, 0, true,local_mes);
            for (int i = 0; i < 32; ++i) {
                if (read_buf0_2[i] != 0) throw int();
            }

            // 验证块1全部被清除
            byte_1 read_buf1[64];
            bmms_f::agent_ptr dst_ap1(read_buf1, 64);
            part.load(64, 64, dst_ap1, 0, true,local_mes);
            for (int i = 0; i < 64; ++i) {
                if (read_buf1[i] != 0) throw int();
            }

            // 验证块2的前32字节被清除
            byte_1 read_buf2[32];
            bmms_f::agent_ptr dst_ap2(read_buf2, 32);
            part.load(128, 32, dst_ap2, 0, true,local_mes);
            for (int i = 0; i < 32; ++i) {
                if (read_buf2[i] != 0) throw int();
            }

            // 验证块2的32-63未被影响（原本未写入，但也应为0）
            byte_1 read_buf2_2[32];
            bmms_f::agent_ptr dst_ap2_2(read_buf2_2, 32);
            part.load(160, 32, dst_ap2_2, 0, true,local_mes);
            for (int i = 0; i < 32; ++i) {
                if (read_buf2_2[i] != 0) throw int();
            }
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 7. clear 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("clear");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );
            static_volume_part part(volume, 5,
                static_volume_part::init_type::init_all_size,
                static_volume_part::control_type::not_allow_over_use,
                local_mes);

            // 写入测试数据
            byte_1 test_data[320];  // 5*64
            for (size_t i = 0; i < 320; ++i) test_data[i] = (byte_1)(i % 256);
            bmms_f::agent_ptr src_ap(test_data, 320);
            part.store(src_ap, 0, 320, 0, true, local_mes);

            // 执行 clear
            part.clear();

            // 验证所有块都已清零
            byte_1 read_buf[320];
            bmms_f::agent_ptr dst_ap(read_buf, 320);
            part.load(0, 320, dst_ap, 0, true, local_mes);
            for (size_t i = 0; i < 320; ++i) {
                if (read_buf[i] != 0) throw int();
            }
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 8. clear_range 测试（已在 get_range_info 中覆盖，此处再补充边界）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("clear_range");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );
            static_volume_part part(volume, 5,
                static_volume_part::init_type::init_all_size,
                static_volume_part::control_type::not_allow_over_use,
                local_mes);

            // 写入数据到所有块
            byte_1 test_data[320];
            for (size_t i = 0; i < 320; ++i) test_data[i] = 0xAA;
            bmms_f::agent_ptr src_ap(test_data, 320);
            part.store(src_ap, 0, 320, 0, true, local_mes);

            // 清除单个块内的范围
            part.clear_range(10, 20);  // 块0的10-29

            // 验证块0的10-29被清零，其他不变
            byte_1 read_buf0[64];
            bmms_f::agent_ptr dst_ap0(read_buf0, 64);
            part.load(0, 64, dst_ap0, 0, true, local_mes);
            for (size_t i = 0; i < 64; ++i) {
                if (i >= 10 && i < 30) {
                    if (read_buf0[i] != 0) throw int();
                }
                else {
                    if (read_buf0[i] != 0xAA) throw int();
                }
            }

            // 清除跨块边界：块4的最后10字节 + 块5? 但只有5个块，块4是最后一个
            // 清除块4的后10字节
            part.clear_range(4 * 64 + 54, 10);  // 块4的54-63

            byte_1 read_buf4[64];
            bmms_f::agent_ptr dst_ap4(read_buf4, 64);
            part.load(4 * 64, 64, dst_ap4, 0, true, local_mes);
            for (size_t i = 0; i < 64; ++i) {
                if (i >= 54) {
                    if (read_buf4[i] != 0) throw int();
                }
                else {
                    if (read_buf4[i] != 0xAA) throw int();
                }
            }

            // 清除空范围（size=0）应触发编码错误
            TEST_CONVERSE_CODING("clear_range with size 0", {
                part.clear_range(0, 0);
                });

            // 清除越界范围
            TEST_CONVERSE_CODING("clear_range out of bounds", {
                part.clear_range(300, 30);  // 总大小320，300+30>320
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 9. free_block 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("free_block");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );
            static_volume_part part(volume, 10,
                static_volume_part::init_type::init_all_size,  // 预分配10个块
                static_volume_part::control_type::not_allow_over_use,
                local_mes);

            size_t free_before = volume->get_free_block_count(); // 20-10=10
            size_t used_before = part.using_block_count;        // 10

            TEST_POSITIVE("free an allocated block", local_mes, {
                part.free_block(3);

                if (part.using_block_count != used_before - 1) throw int();
                if (volume->get_free_block_count() != free_before + 1) throw int();

                // 验证块指针已置空
                void* ptr = nullptr;
                part.block_ptr_ram_space_agent_ptr.load_to_void_ptr(3 * 8, 8, &ptr, true);
                if (ptr != nullptr) throw int();

                // 再次分配应成功
                bmms_f::agent_ptr new_ap;
                bool is_null;
                size_t block_index = 3;
                part.get_block_ptr(block_index, new_ap, is_null);
                if (!is_null) throw int();  // 当前还是空

                // 通过 store 触发自动分配
                byte_1 data[64] = {1};
                bmms_f::agent_ptr src_ap(data, 64);
                part.store(src_ap, 0, 64, 3 * 64, true, _m);  // 写入块3

                part.get_block_ptr(block_index, new_ap, is_null);
                if (is_null) throw int();  // 现在应已分配
                });

            TEST_POSITIVE("free already freed block (no effect)", local_mes, {
                part.free_block(3);  // 再次释放
            // 使用计数不应再减少
            if (part.using_block_count != used_before - 1) throw int(); // 仍为9
                });

            TEST_CONVERSE_CODING("free with invalid index", {
                size_t bad_idx = part.get_block_count_max();
                part.free_block(bad_idx);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 10. free_range 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("free_range");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );
            static_volume_part part(volume, 10,
                static_volume_part::init_type::init_all_size,
                static_volume_part::control_type::not_allow_over_use,
                local_mes);

            size_t free_before = volume->get_free_block_count(); // 10
            size_t used_before = part.using_block_count;        // 10

            TEST_CONVERSE_CODING("free_range out of bounds (start + size exceed total)", {
                static_volume_part part(volume, 10,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                part.free_range(600, 100);  // 600-699 > 639
                });

            TEST_CONVERSE_CODING("free_range out of bounds (size too large)", {
                static_volume_part part(volume, 10,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                part.free_range(0, 641);  // 0-640 > 639
                });

            TEST_CONVERSE_CODING("free_range out of bounds (start at last byte)", {
                static_volume_part part(volume, 10,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                part.free_range(639, 2);  // 639-640 > 639
                });
            TEST_CONVERSE_CODING("free_range with zero size", {
                part.free_range(0, 0);  // size=0 应触发编码错误
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 11. resize 测试 - 缩小
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("resize - shrink");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );

            // --- 场景1: init_all_size, 缩小 ---
            TEST_POSITIVE("shrink with init_all_size, extra blocks freed", local_mes, {
                static_volume_part part(volume, 10,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                size_t free_before = volume->get_free_block_count(); // 20-10=10
                size_t used_before = part.using_block_count;        // 10

                part.resize(6, _m);  // 缩小到6个块

                if (part.block_count_max != 6) throw int();
                if (part.using_block_count != 6) throw int();  // 保留前6个块
                if (volume->get_free_block_count() != free_before + 4) throw int(); // 释放了4个

                // 验证前6个块仍存在
                for (size_t i = 0; i < 6; ++i) {
                    void* ptr = nullptr;
                    part.block_ptr_ram_space_agent_ptr.load_to_void_ptr(i * 8, 8, &ptr, true);
                    if (ptr == nullptr) throw int();
                }
                // 验证后4个块已释放且不可访问
                TEST_CONVERSE_CODING("access freed block after shrink", {
                    bmms_f::agent_ptr ap;
                    bool is_null;
                    size_t block_index = 7;
                    part.get_block_ptr(block_index, ap, is_null);  // 索引7超出新大小
                });
                });

            // --- 场景2: no_init_all_size, 缩小 ---
            TEST_POSITIVE("shrink with no_init_all_size, no blocks allocated", local_mes, {
                static_volume_part part(volume, 10,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);

                // 分配4个块：通过写入真实数据触发
                byte_1 dummy_data[64] = {0};
                for (size_t i = 0; i < 4; ++i) {
                    part.store_from_void_ptr(dummy_data, 64, i * 64, true, _m);
                    if (_m.code != 0) throw int();
                }

                size_t used_before = part.using_block_count; // 应为4
                part.resize(6, _m);
                if (part.using_block_count != used_before) throw int();
                if (part.block_count_max != 6) throw int();
                });

            // --- 服务错误: resize to zero ---
            {
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    local_mes);
                TEST_CONVERSE_CODING("resize to zero", {
                    part.resize(0, _m);
                    });
            }
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 12. resize 测试 - 扩大
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("resize - expand");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );

            // --- 场景1: init_all_size, 扩大，空闲块足够 ---
            TEST_POSITIVE("expand with init_all_size, enough free blocks", local_mes, {
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                size_t free_before = volume->get_free_block_count(); // 20-5=15
                size_t used_before = part.using_block_count;        // 5

                part.resize(8, _m);

                if (part.block_count_max != 8) throw int();
                if (part.using_block_count != 8) throw int();
                if (volume->get_free_block_count() != free_before - 3) throw int();

                for (size_t i = 5; i < 8; ++i) {
                    void* ptr = nullptr;
                    part.block_ptr_ram_space_agent_ptr.load_to_void_ptr(i * 8, 8, &ptr, true);
                    if (ptr == nullptr) throw int();
                }
                });

            // --- 场景2: 构造时空闲块不足（服务错误）---
            {
                shared_ptr<static_volume> vol2 = make_shared<static_volume>(
                    64, 20, 
                    static_volume::init_type::init_all_size,
                    static_volume::alignment_type::without_alignment,
                    local_mes);

                // 预分配15个块，空闲=5
                bmms_f::agent_ptr blocks[15];
                for (int i = 0; i < 15; ++i) vol2->new_block(blocks[i], local_mes);

                size_t code = bmms_m::err::service_err_static_volume_part_build_not_enough_free_blocks().code;
                TEST_CONVERSE_SERVICE("construct with init_all_size but not enough free blocks",
                    code, local_mes, {
                    static_volume_part part(vol2, 8,  // 需要8个，只有5个空闲
                        static_volume_part::init_type::init_all_size,
                        static_volume_part::control_type::not_allow_over_use,
                        _m);
                    });
            }
            // --- 场景3: resize时空闲块不足（服务错误）---
            {
                shared_ptr<static_volume> vol2 = make_shared<static_volume>(
                    64, 20, 
                    static_volume::init_type::init_all_size,
                    static_volume::alignment_type::without_alignment,
                    local_mes
                );

                // 先构造一个 part，预分配5个块
                static_volume_part part(vol2, 5,
                    static_volume_part::init_type::init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    local_mes);

                // 再分配一些块，减少空闲块
                bmms_f::agent_ptr blocks[10];
                for (int i = 0; i < 10; ++i) vol2->new_block(blocks[i], local_mes);
                // 现在空闲块 = 20-5-10 = 5

                size_t code = bmms_m::err::service_err_static_volume_part_resize_not_enough_blocks().code;
                TEST_CONVERSE_SERVICE("resize with init_all_size but not enough free blocks",
                    code, local_mes, {
                    part.resize(11, _m);  // 需要6个新块，但只有5个空闲
                    });
            }

            // --- 场景4: no_init_all_size, 扩大 ---
            TEST_POSITIVE("expand with no_init_all_size, new blocks unallocated", local_mes, {
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                size_t used_before = part.using_block_count; // 0
                part.resize(8, _m);
                if (part.using_block_count != used_before) throw int();
                if (part.block_count_max != 8) throw int();
                for (size_t i = 5; i < 8; ++i) {
                    void* ptr = nullptr;
                    part.block_ptr_ram_space_agent_ptr.load_to_void_ptr(i * 8, 8, &ptr, true);
                    if (ptr != nullptr) throw int();
                }
                });

            // --- 场景5: allow_over_use, 扩大超过卷最大块数 ---
            TEST_POSITIVE("expand with allow_over_use, exceed volume max", local_mes, {
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::allow_over_use,
                    _m);
                part.resize(25, _m);
                if (part.block_count_max != 25) throw int();
                });

            // --- 服务错误: not_allow_over_use 且扩大后超过卷最大块数 ---
            {
                size_t code = bmms_m::err::service_err_static_volume_part_resize_over_volume_limit().code;
                TEST_CONVERSE_SERVICE("expand with not_allow_over_use, exceed volume max",
                    code, local_mes, {
                    static_volume_part part(volume, 5,
                        static_volume_part::init_type::no_init_all_size,
                        static_volume_part::control_type::not_allow_over_use,
                        _m);
                    part.resize(21, _m);  // 卷最大20
                    });
            }
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
// 13. load/store 测试
// ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("load/store");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );

            // --- store 触发自动分配 ---
            TEST_POSITIVE("store to unallocated block triggers allocation", local_mes, {
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);

                byte_1 src_data[64] = {0x12, 0x34, 0x56};
                bmms_f::agent_ptr src_ap(src_data, 64);
                size_t used_before = part.using_block_count; // 0

                part.store(src_ap, 0, 64, 0, true, _m);
                if (_m.code != 0) throw int();

                if (part.using_block_count != used_before + 1) throw int();

                void* ptr = nullptr;
                part.block_ptr_ram_space_agent_ptr.load_to_void_ptr(0, 8, &ptr, true);
                if (ptr == nullptr) throw int();

                byte_1 dst_data[64] = {0};
                bmms_f::agent_ptr dst_ap(dst_data, 64);
                part.load(0, 64, dst_ap, 0, true, _m);
                if (_m.code != 0) throw int();

                if (dst_data[0] != 0x12 || dst_data[1] != 0x34 || dst_data[2] != 0x56) throw int();
                });

            // --- store 跨块写入 ---
            TEST_POSITIVE("store across block boundary", local_mes, {
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);

                byte_1 src_data[150];
                for (int i = 0; i < 150; ++i) src_data[i] = (byte_1)(i % 256);
                bmms_f::agent_ptr src_ap(src_data, 150);

                part.store(src_ap, 0, 150, 50, true, _m);
                if (_m.code != 0) throw int();

                // 验证块0、1、2被自动分配
                if (part.using_block_count < 3) throw int();

                byte_1 dst_data[150] = {0};
                bmms_f::agent_ptr dst_ap(dst_data, 150);
                part.load(50, 150, dst_ap, 0, true, _m);
                if (_m.code != 0) throw int();

                for (int i = 0; i < 150; ++i) {
                    if (dst_data[i] != (byte_1)(i % 256)) throw int();
                }
                });

            // --- load 从未分配块读取返回0 ---
            TEST_POSITIVE("load from unallocated block returns zeros", local_mes, {
                // ✅ 全新 part，没有任何块被分配
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);

                // 确保没有分配任何块
                if (part.using_block_count != 0) throw int();

                byte_1 dst_data[64] = {0xFF};
                bmms_f::agent_ptr dst_ap(dst_data, 64);

                part.load(2 * 64, 64, dst_ap, 0, true, _m);
                if (_m.code != 0) throw int();

                for (int i = 0; i < 64; ++i) {
                    if (dst_data[i] != 0) throw int();
                }
                });

            // --- load_to_void_ptr and store_from_void_ptr ---
            TEST_POSITIVE("load_to_void_ptr and store_from_void_ptr", local_mes, {
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);

                byte_1 src[64] = {0xAB, 0xCD};
                part.store_from_void_ptr(src, 64, 0, true, _m);
                if (_m.code != 0) throw int();

                byte_1 dst[64] = {0};
                part.load_to_void_ptr(0, 64, dst, true, _m);
                if (_m.code != 0) throw int();

                if (dst[0] != 0xAB || dst[1] != 0xCD) throw int();
                });

            // --- 范围校验反例 ---
            TEST_CONVERSE_CODING("store out of bounds", {
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                byte_1 data[10];
                bmms_f::agent_ptr ap(data, 10);
                part.store(ap, 0, 10, 500, true, _m);
                });

            TEST_CONVERSE_CODING("load out of bounds", {
                static_volume_part part(volume, 5,
                    static_volume_part::init_type::no_init_all_size,
                    static_volume_part::control_type::not_allow_over_use,
                    _m);
                bmms_f::agent_ptr ap(nullptr,0);
                part.load(500, 10, ap, 0, true, _m);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 14. check_build_value_valid 反例覆盖（已通过构造函数隐式测试）
        //     此处补充显式调用反例（如果公有）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("check_build_value_valid");
        {
            mes::a_mes local_mes;
            shared_ptr<static_volume> volume = make_shared<static_volume>(
                64, 20,
                static_volume::init_type::no_init_all_size,
                static_volume::alignment_type::without_alignment,
                local_mes
            );
            static_volume_part part(volume, 5,
                static_volume_part::init_type::no_init_all_size,
                static_volume_part::control_type::not_allow_over_use,
                local_mes);

            // 由于该函数是私有，我们通过构造函数已经覆盖了所有反例
            // 这里不再重复
        }
        TEST_FUNCTION_END();
        TEST_CLASS_END();
        TEST_MODULE_END();
    }

#endif // BMMS_INCLUDE_TEST

} // namespace bmms_f