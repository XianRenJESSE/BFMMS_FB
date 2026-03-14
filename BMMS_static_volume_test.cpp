// BMMS_static_volume_test.cpp
module BMMS;

#include "BMMS_dp.h"
import MES;

#if defined(BMMS_INCLUDE_TEST)
#include "TEST_helper.h"
#endif
namespace bmms_f {
#if defined(BMMS_INCLUDE_TEST)
    // ========== static_volume 单元测试 ==========
    void static_volume::test() {
        using namespace test_helper;

        // ---------- 模块开始 ----------
        TEST_MODULE_BEGIN("STATIC_VOLUME");

        // ---------- 类测试：static_volume ----------
        TEST_CLASS_BEGIN("static_volume");

        // ------------------------------------------------------------
        // 1. 构造函数测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Constructor");
        {
            mes::a_mes local_mes;

            // ----- 正例：正常构造 -----
            TEST_POSITIVE("construct with valid parameters (size=64, count=10, no_init, no_alignment)", local_mes, {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                if (!vol.get_is_init()) throw int();
                });

            TEST_POSITIVE("construct with init_all_size and 8-byte alignment", local_mes, {
                static_volume vol(32, 5, static_volume::init_type::init_all_size,
                                  static_volume::alignment_type::byte_8_each_block, _m);
                if (!vol.get_is_init()) throw int();
                });

            // ----- 反例：编码错误（构造函数抛出异常）-----
            TEST_CONVERSE_CODING("block size = 0", {
                static_volume vol(0, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                });

            TEST_CONVERSE_CODING("block size > STATIC_VOLUME_MAX_BLOCK_SIZE", {
                static_volume vol(STATIC_VOLUME_MAX_BLOCK_SIZE + 1, 10,
                                  static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                });

            TEST_CONVERSE_CODING("block count = 0", {
                static_volume vol(64, 0, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                });

            TEST_CONVERSE_CODING("block count > STATIC_VOLUME_MAX_BLOCK_COUNT", {
                static_volume vol(64, STATIC_VOLUME_MAX_BLOCK_COUNT + 1,
                                  static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                });

            TEST_CONVERSE_CODING("block count > bmms_f::index_max", {
                static_volume vol(64, bmms_f::index_max + 1,
                                  static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                });

            TEST_CONVERSE_CODING("invalid init_type (2)", {
                static_volume vol(64, 10, static_cast<static_volume::init_type>(2),
                                  static_volume::alignment_type::without_alignment, _m);
                });

            TEST_CONVERSE_CODING("invalid alignment_type (6)", {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_cast<static_volume::alignment_type>(6), _m);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 2. 移动构造函数
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Move Constructor");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("move construct, source becomes invalid, target works", local_mes, {
                static_volume src(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                static_volume dst(move(src));
                if (!dst.get_is_init() || src.get_is_init()) throw int();
                if (dst.get_block_size() != 64 || dst.get_block_count_max() != 5) throw int();
                });

            TEST_CONVERSE_CODING("call new_block on moved-from object", {
                static_volume src(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                static_volume dst(move(src));
                bmms_f::agent_ptr ap;
                src.new_block(ap, _m);
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
                static_volume src(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                static_volume dst(128, 3, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                dst = move(src);
                if (!dst.get_is_init() || src.get_is_init()) throw int();
                if (dst.get_block_size() != 64 || dst.get_block_count_max() != 5) throw int();
                });

            TEST_CONVERSE_CODING("call delete_block on moved-from object", {
                static_volume src(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                static_volume dst(128, 3, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                dst = move(src);
                bmms_f::agent_ptr ap;
                src.delete_block(ap);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 4. get_is_init
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_is_init");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("after construction returns true", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                if (!vol.get_is_init()) throw int();
                });

            TEST_POSITIVE("moved-from object returns false", local_mes, {
                static_volume src(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                static_volume dst(move(src));
                if (src.get_is_init()) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 5. get_block_count_max
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_block_count_max");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("returns constructor argument", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                if (vol.get_block_count_max() != 5) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 6. get_free_block_count
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_free_block_count");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("initially equals block_count_max", local_mes, {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                if (vol.get_free_block_count() != 10) throw int();
                });

            TEST_POSITIVE("after new_block decreases by 1", local_mes, {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr ap;
                vol.new_block(ap, _m);
                if (vol.get_free_block_count() != 9) throw int();
                });

            TEST_POSITIVE("after delete_block increases by 1", local_mes, {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr ap;
                vol.new_block(ap, _m);
                vol.delete_block(ap);
                if (vol.get_free_block_count() != 10) throw int();
                });

            TEST_POSITIVE("moved-from object returns 0", local_mes, {
                static_volume src(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                static_volume dst(move(src));
                if (src.get_free_block_count() != 0) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 7. get_block_size
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_block_size");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("returns constructor block_size", local_mes, {
                static_volume vol(128, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                if (vol.get_block_size() != 128) throw int();
                });

            TEST_POSITIVE("moved-from object returns 0", local_mes, {
                static_volume src(128, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                static_volume dst(move(src));
                if (src.get_block_size() != 0) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 8. __total_ram_size__
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("__total_ram_size__");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("returns correct total size (without_alignment)", local_mes, {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                size_t actual = vol.__actually_block_size__();
                size_t expected = (actual + 8) * 10;
                if (vol.__total_ram_size__() != expected) throw int();
                });

            TEST_POSITIVE("returns correct total size (with alignment)", local_mes, {
                static_volume vol(63, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::byte_8_each_block, _m);
                size_t actual = vol.__actually_block_size__();
                size_t expected = (actual + 8) * 10;
                if (vol.__total_ram_size__() != expected) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 9. __recycle_stack_size__
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("__recycle_stack_size__");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("returns block_count_max * 8", local_mes, {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                if (vol.__recycle_stack_size__() != 10 * 8) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 10. __actually_block_size__
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("__actually_block_size__");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("without_alignment returns block_size", local_mes, {
                static_volume vol(63, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                if (vol.__actually_block_size__() != 63) throw int();
                });

            TEST_POSITIVE("8-byte alignment rounds up to multiple of 8", local_mes, {
                static_volume vol(63, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::byte_8_each_block, _m);
                if (vol.__actually_block_size__() != 64) {
                    output << "expected block size 64 but actually size is "
                        << to_string(vol.__actually_block_size__()) << endl;
                    throw int();
                } 
                });
            

            TEST_POSITIVE("16-byte alignment rounds up to multiple of 16", local_mes, {
                static_volume vol(63, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::byte_16_each_block, _m);
                if (vol.__actually_block_size__() != 64) {
                    output << "expected block size 64 but actually size is "
                        << to_string(vol.__actually_block_size__()) << endl;
                    throw int();
                }
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 11. __block_space_begin__ / __block_space_end__
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("__block_space_begin__ / __block_space_end__");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("end - begin == __block_space_size__", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                uintptr_t begin = vol.__block_space_begin__();
                uintptr_t end = vol.__block_space_end__();
                if (end - begin != vol.__block_space_size__()) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 12. __stack_begin__ / __stack_end__
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("__stack_begin__ / __stack_end__");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("stack_end - stack_begin == __recycle_stack_size__", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                uintptr_t stack_begin = vol.__stack_begin__();
                uintptr_t stack_end = vol.__stack_end__();
                if (stack_end - stack_begin != vol.__recycle_stack_size__()) throw int();
                });

            TEST_POSITIVE("stack_begin == __block_space_end__", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                if (vol.__stack_begin__() != vol.__block_space_end__()) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 13. check_total_size_no_overflow
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("check_total_size_no_overflow");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("call on valid volume (no throw)", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                vol.check_total_size_no_overflow();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 14. check_enum_init_type_valid
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("check_enum_init_type_valid");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("valid init_type 0", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                vol.check_enum_init_type_valid(static_volume::init_type::no_init_all_size);
                });
            TEST_POSITIVE("valid init_type 1", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                vol.check_enum_init_type_valid(static_volume::init_type::init_all_size);
                });
            TEST_CONVERSE_CODING("invalid init_type 2", {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                vol.check_enum_init_type_valid(static_cast<static_volume::init_type>(2));
                });
            TEST_CONVERSE_CODING("invalid init_type 100", {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                vol.check_enum_init_type_valid(static_cast<static_volume::init_type>(100));
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 15. check_enum_alignment_type_valid
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("check_enum_alignment_type_valid");
        {
            mes::a_mes local_mes;

            for (int i = 0; i <= 5; ++i) {
                TEST_POSITIVE("valid alignment_type " + to_string(i), local_mes, {
                    static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                      static_volume::alignment_type::without_alignment, _m);
                    vol.check_enum_alignment_type_valid(static_cast<static_volume::alignment_type>(i));
                    });
            }
            TEST_CONVERSE_CODING("invalid alignment_type 6", {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                vol.check_enum_alignment_type_valid(static_cast<static_volume::alignment_type>(6));
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 16. check_block_belong_to_self
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("check_block_belong_to_self");
        {
            mes::a_mes local_mes;

            // 正例1：属于本卷的块
            TEST_POSITIVE("block belongs to self", local_mes, {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr ap;
                vol.new_block(ap, _m);
                vol.check_block_belong_to_self(ap);  // 应无异常
                });

            // 正例2：释放后，地址仍然属于卷（保存地址副本）
            TEST_POSITIVE("block address still belongs after deletion", local_mes, {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr ap;
                vol.new_block(ap, _m);

                // 保存地址和大小
                void* saved_ptr = nullptr;
                ap.get_void_ptr(saved_ptr);
                size_t saved_size = 0;
                ap.get_size(saved_size);

                vol.delete_block(ap);  // ap 被置空，这是设计行为

                // 用保存的地址构造新的代理指针
                bmms_f::agent_ptr address_check(saved_ptr, saved_size);
                vol.check_block_belong_to_self(address_check);  // 应通过
                });

            // 反例1：空代理指针
            TEST_CONVERSE_CODING("null agent_ptr", {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr null_ap;
                vol.check_block_belong_to_self(null_ap);  // 应抛出编码错误
                });

            // 反例2：大小不匹配
            TEST_CONVERSE_CODING("size mismatch", {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                byte_1 dummy[128];
                bmms_f::agent_ptr wrong_size(dummy, 128);
                vol.check_block_belong_to_self(wrong_size);  // 应抛出编码错误
                });

            // 反例3：地址不在卷空间内
            TEST_CONVERSE_CODING("address out of range", {
                static_volume vol(64, 10, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                byte_1 dummy[64];
                bmms_f::agent_ptr out_of_range(dummy, 64);
                vol.check_block_belong_to_self(out_of_range);  // 应抛出编码错误
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 17. check_can_push
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("check_can_push");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("block not yet freed, stack not full", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr ap;
                vol.new_block(ap, _m);
                vol.check_can_push(ap);
                });

            TEST_CONVERSE_CODING("block already freed", {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr ap;
                vol.new_block(ap, _m);
                vol.delete_block(ap);
                vol.check_can_push(ap);
                });
            // 栈满分支不可达，不测试
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 18. check_can_pop
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("check_can_pop");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("initially returns true", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                if (!vol.check_can_pop()) throw int();
                });

            TEST_POSITIVE("after allocating all blocks returns false", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr blocks[5];
                for (int i = 0; i < 5; ++i) {
                    vol.new_block(blocks[i], _m);
                }
                if (vol.check_can_pop()) throw int();
                });

            TEST_POSITIVE("after freeing one block returns true", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr blocks[5];
                for (int i = 0; i < 5; ++i) {
                    vol.new_block(blocks[i], _m);
                }
                vol.delete_block(blocks[0]);
                if (!vol.check_can_pop()) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 19. new_block
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("new_block");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("allocate one block successfully", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr ap;
                vol.new_block(ap, _m);
                if (ap.__self_begin__() == 0) throw int();
                if (vol.get_free_block_count() != 4) throw int();
                });

            TEST_CONVERSE_CODING("volume not initialized (moved-from)", {
                static_volume src(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                static_volume dst(move(src));
                bmms_f::agent_ptr ap;
                src.new_block(ap, _m);
                });

            // 服务错误：回收栈空
            {
                static_volume vol(64, 3, static_volume::init_type::no_init_all_size,
                    static_volume::alignment_type::without_alignment, local_mes);
                bmms_f::agent_ptr blocks[3];
                for (int i = 0; i < 3; ++i) {
                    vol.new_block(blocks[i], local_mes);
                }
                bmms_f::agent_ptr extra;
                size_t expected_code = bmms_m::err::service_err_static_volume_recycle_stack_empty().code;
                TEST_CONVERSE_SERVICE("recycle stack empty", expected_code, local_mes, {
                    vol.new_block(extra, _m);
                    });
            }
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 20. delete_block
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("delete_block");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("delete a valid allocated block", local_mes, {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr ap;
                vol.new_block(ap, _m);
                vol.delete_block(ap);
                if (vol.get_free_block_count() != 5) throw int();
                });

            TEST_CONVERSE_CODING("volume not initialized", {
                static_volume src(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                static_volume dst(move(src));
                bmms_f::agent_ptr ap;
                src.delete_block(ap);
                });

            TEST_CONVERSE_CODING("null agent_ptr", {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr null_ap;
                vol.delete_block(null_ap);
                });

            TEST_CONVERSE_CODING("block size mismatch", {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                byte_1 dummy[128];
                bmms_f::agent_ptr wrong_size(dummy, 128);
                vol.delete_block(wrong_size);
                });

            TEST_CONVERSE_CODING("address out of range", {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                byte_1 dummy[64];
                bmms_f::agent_ptr out_of_range(dummy, 64);
                vol.delete_block(out_of_range);
                });

            TEST_CONVERSE_CODING("block already freed", {
                static_volume vol(64, 5, static_volume::init_type::no_init_all_size,
                                  static_volume::alignment_type::without_alignment, _m);
                bmms_f::agent_ptr ap;
                vol.new_block(ap, _m);
                vol.delete_block(ap);
                vol.delete_block(ap);
                });
        }
        TEST_FUNCTION_END();

        // ========== 类测试结束 ==========
        TEST_CLASS_END();

        // ========== 模块结束 ==========
        TEST_MODULE_END();
    }

#endif // BMMS_INCLUDE_TEST

} // namespace bmms_f
