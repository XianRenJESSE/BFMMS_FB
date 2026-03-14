// BMMS_free_volume_test.cpp
module BMMS;

#include "BMMS_dp.h"
import MES;

#if defined(BMMS_INCLUDE_TEST)
#include "TEST_helper.h"
#endif

namespace bmms_f {
#if defined(BMMS_INCLUDE_TEST)

    // ========== free_volume 单元测试 - 核心功能 ==========
    void free_volume::test() {
        using namespace test_helper;

        TEST_MODULE_BEGIN("FREE_VOLUME");
        TEST_CLASS_BEGIN("free_volume");

        // ------------------------------------------------------------
        // 1. 构造函数测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Constructor");
        {
            mes::a_mes local_mes;

            // ----- 正例：正常构造 -----
            TEST_POSITIVE("construct with valid parameters (size=64, count=10, no_init, no_alignment)", local_mes, {
                free_volume vol(64, 10,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                if (!vol.get_is_init()) throw int();
                if (vol.get_block_count_max() != 10) throw int();
                if (vol.get_block_size() != 64) throw int();
                if (vol.get_free_block_count() != 10) throw int();
                if (vol.get_block_count_used() != 0) throw int();
                });

            TEST_POSITIVE("construct with init_all_size and 8-byte alignment", local_mes, {
                free_volume vol(63, 5,
                    free_volume::init_type::init_all_size,
                    free_volume::alignment_type::byte_8_each_block,
                    _m);
                if (!vol.get_is_init()) throw int();
                if (vol.__actually_block_size__() != 64) throw int();
                });

            // ----- 反例：编码错误（构造函数抛出异常）-----
            TEST_CONVERSE_CODING("block size = 0", {
                free_volume vol(0, 10,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                });

            TEST_CONVERSE_CODING("block size > FREE_VOLUME_MAX_BLOCK_SIZE", {
                free_volume vol(FREE_VOLUME_MAX_BLOCK_SIZE + 1, 10,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                });

            TEST_CONVERSE_CODING("block count = 0", {
                free_volume vol(64, 0,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                });

            TEST_CONVERSE_CODING("block count > FREE_VOLUME_MAX_BLOCK_COUNT", {
                free_volume vol(64, FREE_VOLUME_MAX_BLOCK_COUNT + 1,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                });

            TEST_CONVERSE_CODING("block count > bmms_f::index_max", {
                free_volume vol(64, bmms_f::index_max + 1,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                });

            TEST_CONVERSE_CODING("invalid init_type (2)", {
                free_volume vol(64, 10,
                    static_cast<free_volume::init_type>(2),
                    free_volume::alignment_type::without_alignment,
                    _m);
                });

            TEST_CONVERSE_CODING("invalid alignment_type (6)", {
                free_volume vol(64, 10,
                    free_volume::init_type::no_init_all_size,
                    static_cast<free_volume::alignment_type>(6),
                    _m);
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
                free_volume src(64, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                free_volume dst(move(src));

                if (!dst.get_is_init()) throw int();
                if (src.get_is_init()) throw int();
                if (dst.get_block_count_max() != 5) throw int();
                if (dst.get_block_size() != 64) throw int();
                });

            TEST_CONVERSE_CODING("call new_block on moved-from object", {
                free_volume src(64, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                free_volume dst(move(src));
                bmms_f::agent_ptr ap;
                size_t index;
                src.new_block(ap, index, _m);
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
                free_volume src(64, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                free_volume dst(128, 3,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);

                dst = move(src);

                if (!dst.get_is_init()) throw int();
                if (src.get_is_init()) throw int();
                if (dst.get_block_size() != 64) throw int();
                if (dst.get_block_count_max() != 5) throw int();
                });

            TEST_CONVERSE_CODING("call delete_block on moved-from object", {
                free_volume src(64, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                free_volume dst(128, 3,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                dst = move(src);
                bmms_f::agent_ptr ap;
                size_t index;
                src.delete_block(ap, index);
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
                free_volume vol(64, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                if (!vol.get_is_init()) throw int();
                });

            TEST_POSITIVE("moved-from object returns false", local_mes, {
                free_volume src(64, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                free_volume dst(move(src));
                if (src.get_is_init()) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 5. get_block_count_max / get_block_size / get_free_block_count / get_block_count_used
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Getters");
        {
            mes::a_mes local_mes;
            free_volume vol(64, 10,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                local_mes);

            TEST_POSITIVE("get_block_count_max returns constructor argument", local_mes, {
                if (vol.get_block_count_max() != 10) throw int();
                });

            TEST_POSITIVE("get_block_size returns constructor argument", local_mes, {
                if (vol.get_block_size() != 64) throw int();
                });

            TEST_POSITIVE("get_free_block_count initially equals block_count_max", local_mes, {
                if (vol.get_free_block_count() != 10) throw int();
                });

            TEST_POSITIVE("get_block_count_used initially 0", local_mes, {
                if (vol.get_block_count_used() != 0) throw int();
                });

            TEST_CONVERSE_CODING("get_block_count_max on uninitialized volume", {
                free_volume src(64, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                free_volume dst(move(src));
                src.get_block_count_max();
                });

            TEST_CONVERSE_CODING("get_block_count_used on uninitialized volume", {
                free_volume src(64, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                free_volume dst(move(src));
                src.get_block_count_used();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 6. __actually_block_size__ 对齐测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("__actually_block_size__");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("without_alignment returns block_size", local_mes, {
                free_volume vol(63, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                if (vol.__actually_block_size__() != 63) throw int();
                });

            TEST_POSITIVE("8-byte alignment rounds up to multiple of 8", local_mes, {
                free_volume vol(63, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::byte_8_each_block,
                    _m);
                if (vol.__actually_block_size__() != 64) throw int();
                });

            TEST_POSITIVE("16-byte alignment rounds up to multiple of 16", local_mes, {
                free_volume vol(63, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::byte_16_each_block,
                    _m);
                if (vol.__actually_block_size__() != 64) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 7. new_block 正例 + 服务错误
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("new_block");
        {
            mes::a_mes local_mes;
            free_volume vol(64, 5,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                local_mes);

            TEST_POSITIVE("allocate one block successfully", local_mes, {
                bmms_f::agent_ptr ap;
                size_t index = 999;
                vol.new_block(ap, index, _m);

                if (ap.__self_begin__() == 0) throw int();
                if (index >= vol.get_block_count_max()) throw int();
                if (vol.get_free_block_count() != 4) throw int();
                if (vol.get_block_count_used() != 1) throw int();
                });

            TEST_POSITIVE("allocate all blocks", local_mes, {
                free_volume vol2(64, 3,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                bmms_f::agent_ptr blocks[3];
                size_t indices[3];

                for (int i = 0; i < 3; ++i) {
                    vol2.new_block(blocks[i], indices[i], _m);
                }

                if (vol2.get_free_block_count() != 0) throw int();
                if (vol2.get_block_count_used() != 3) throw int();
                });

            TEST_CONVERSE_CODING("volume not initialized (moved-from)", {
                free_volume src(64, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                free_volume dst(move(src));
                bmms_f::agent_ptr ap;
                size_t index;
                src.new_block(ap, index, _m);
                });

            {
                free_volume vol2(64, 3,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    local_mes);
                bmms_f::agent_ptr blocks[3];
                size_t indices[3];
                for (int i = 0; i < 3; ++i) {
                    vol2.new_block(blocks[i], indices[i], local_mes);
                }

                bmms_f::agent_ptr extra_ap;
                size_t extra_index;
                size_t expected_code = bmms_m::err::service_err_free_volume_recycle_stack_empty().code;

                TEST_CONVERSE_SERVICE("recycle stack empty", expected_code, local_mes, {
                    vol2.new_block(extra_ap, extra_index, _m);
                    });
            }
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 8. delete_block 正例 + 反例
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("delete_block");
        {
            mes::a_mes local_mes;
            free_volume vol(64, 5,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                local_mes);

            bmms_f::agent_ptr ap;
            size_t index;
            vol.new_block(ap, index, local_mes);

            TEST_POSITIVE("delete a valid allocated block", local_mes, {
                vol.delete_block(ap, index);
                if (vol.get_free_block_count() != 5) throw int();
                if (vol.get_block_count_used() != 0) throw int();
                });

            vol.new_block(ap, index, local_mes);

            TEST_CONVERSE_CODING("volume not initialized", {
                free_volume src(64, 5,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    _m);
                free_volume dst(move(src));
                src.delete_block(ap, index);
                });

            TEST_CONVERSE_CODING("null agent_ptr", {
                bmms_f::agent_ptr null_ap;
                vol.delete_block(null_ap, index);
                });

            TEST_CONVERSE_CODING("invalid block index", {
                size_t bad_index = vol.get_block_count_max();
                vol.delete_block(ap, bad_index);
                });

            TEST_CONVERSE_CODING("index and agent_ptr mismatch", {
                bmms_f::agent_ptr ap2;
                size_t index2;
                vol.new_block(ap2, index2, _m);
                vol.delete_block(ap2, index);
                });

            TEST_CONVERSE_CODING("block already freed", {
                vol.delete_block(ap, index);
                vol.delete_block(ap, index);
                });
        }
        TEST_FUNCTION_END();

        TEST_CLASS_END();
        TEST_MODULE_END();
    }


#endif // BMMS_INCLUDE_TEST
} // namespace bmms_f
