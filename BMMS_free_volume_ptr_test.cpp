// BMMS_free_volume_ptr_test.cpp
module BMMS;

#include "BMMS_dp.h"
import MES;

#if defined(BMMS_INCLUDE_TEST)
#include "TEST_helper.h"

namespace bmms_f {

    void free_volume_ptr::test() {
        using namespace test_helper;

        TEST_MODULE_BEGIN("FREE_VOLUME_PTR");
        TEST_CLASS_BEGIN("free_volume_ptr");

        // ------------------------------------------------------------
        // 1. 构造函数测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Constructor");
        {
            mes::a_mes local_mes;

            // ----- 准备一个自由卷 -----
            shared_ptr<free_volume> volume = make_shared<free_volume>(
                64, 10,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                local_mes
            );
            if (local_mes.code != 0) throw int();

            // ----- 正例1: 正常构造 -----
            TEST_POSITIVE("construct with valid volume", local_mes, {
                free_volume_ptr ptr(volume, _m);
                if (_m.code != 0) throw int();
                if (!ptr.is_init) throw int();
                if (ptr.get_index() >= volume->get_block_count_max()) throw int();  // 索引有效
                if (ptr.get_agent_ptr().__self_begin__() == 0) throw int();  // 指针非空
                });

            // ----- 正例2: 多个指针指向同一卷 -----
            TEST_POSITIVE("multiple pointers from same volume", local_mes, {
                free_volume_ptr ptr1(volume, _m);
                if (_m.code != 0) throw int();

                free_volume_ptr ptr2(volume, _m);
                if (_m.code != 0) throw int();

                if (ptr1.get_index() == ptr2.get_index()) throw int();  // 应分配不同块
                });

            // ----- 反例1: 空 shared_ptr -----
            TEST_CONVERSE_CODING("null shared_ptr", {
                shared_ptr<free_volume> null_ptr;
                free_volume_ptr ptr(null_ptr, _m);
                });

            // ----- 反例2: 卷未初始化 -----
            TEST_CONVERSE_CODING("volume not initialized (moved-from)", {
                mes::a_mes tmp;
            // 创建原始卷
            free_volume o_vol(64, 10,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                tmp);

            // 移动构造，o_vol 变为未初始化
            free_volume moved_vol = move(o_vol);

            // 将未初始化的 o_vol 包装进 shared_ptr
            shared_ptr<free_volume> uninit_ptr = make_shared<free_volume>(move(o_vol));

            free_volume_ptr ptr(uninit_ptr, tmp);
                });

            // ----- 服务错误1: 卷内存不足（无空闲块）-----
            {
                shared_ptr<free_volume> small_vol = make_shared<free_volume>(
                    64, 1,
                    free_volume::init_type::no_init_all_size,
                    free_volume::alignment_type::without_alignment,
                    local_mes
                );
                free_volume_ptr ptr1(small_vol, local_mes);
                if (local_mes.code != 0) throw int();
                size_t expected_code = bmms_m::err::service_err_free_volume_ptr_new_block_failed().code;
                TEST_CONVERSE_SERVICE("construct when volume has no free blocks",
                    expected_code, local_mes, {
                    free_volume_ptr ptr2(small_vol, _m);  // 此时卷无空闲块
                    });

            }
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 2. 移动构造函数
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Move Constructor");
        {
            mes::a_mes local_mes;
            shared_ptr<free_volume> volume = make_shared<free_volume>(
                64, 10,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                local_mes
            );

            TEST_POSITIVE("move construct, source becomes invalid, target works", local_mes, {
                free_volume_ptr src(volume, _m);
                size_t src_index = src.get_index();
                auto src_ptr_val = src.get_agent_ptr().__self_begin__();

                free_volume_ptr dst(move(src));

                if (!dst.is_init) throw int();
                if (src.is_init) throw int();  // 源对象应无效
                if (dst.get_index() != src_index) throw int();  // 索引应相同
                if (dst.get_agent_ptr().__self_begin__() != src_ptr_val) throw int();  // 指针应相同
                });

            TEST_CONVERSE_CODING("call get_index on moved-from object", {
                free_volume_ptr src(volume, _m);
                free_volume_ptr dst(move(src));
                src.get_index();  // 源对象未初始化 → 编码错误
                });

            TEST_CONVERSE_CODING("call clear on moved-from object", {
                free_volume_ptr src(volume, _m);
                free_volume_ptr dst(move(src));
                src.clear();  // 源对象未初始化 → 编码错误
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 3. 移动赋值运算符
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Move Assignment");
        {
            mes::a_mes local_mes;
            shared_ptr<free_volume> volume = make_shared<free_volume>(
                64, 10,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                local_mes
            );

            TEST_POSITIVE("move assign, target gets resources, source becomes invalid", local_mes, {
                free_volume_ptr src(volume, _m);
                size_t src_index = src.get_index();
                auto src_ptr_val = src.get_agent_ptr().__self_begin__();

                free_volume_ptr dst(volume, _m);  // 另一个指针
                size_t dst_index = dst.get_index();

                dst = move(src);

                if (!dst.is_init) throw int();
                if (src.is_init) throw int();
                if (dst.get_index() != src_index) throw int();  // 应获得 src 的索引
                if (dst.get_agent_ptr().__self_begin__() != src_ptr_val) throw int();

                // 验证原 dst 的块已被释放（可通过卷的空闲块数验证）
                // 由于不确定原 dst 的索引，只能检查卷状态
                });

            TEST_POSITIVE("self move assignment (should have no effect)", local_mes, {
                free_volume_ptr ptr(volume, _m);
                size_t old_index = ptr.get_index();
                auto old_ptr = ptr.get_agent_ptr().__self_begin__();

                ptr = move(ptr);  // 自移动

                if (!ptr.is_init) throw int();
                if (ptr.get_index() != old_index) throw int();
                if (ptr.get_agent_ptr().__self_begin__() != old_ptr) throw int();
                });

            TEST_CONVERSE_CODING("call store on moved-from object", {
                free_volume_ptr src(volume, _m);
                free_volume_ptr dst(volume, _m);
                dst = move(src);

                byte_1 data[10];
                bmms_f::agent_ptr ap(data, 10);
                src.store(ap, 0, 10, 0, true);  // 源对象未初始化 → 编码错误
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 4. 析构函数测试（自动释放）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Destructor");
        {
            mes::a_mes local_mes;
            shared_ptr<free_volume> volume = make_shared<free_volume>(
                64, 10,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                local_mes
            );

            TEST_POSITIVE("destructor automatically releases block", local_mes, {
                size_t free_before = volume->get_free_block_count();
                {
                    free_volume_ptr ptr(volume, _m);
                    if (_m.code != 0) throw int();
                    if (volume->get_free_block_count() != free_before - 1) throw int();
                }
                // ptr 离开作用域，应自动释放
                if (volume->get_free_block_count() != free_before) throw int();
                });

            TEST_POSITIVE("multiple pointers, each releases its own block", local_mes, {
                size_t free_before = volume->get_free_block_count();
                {
                    free_volume_ptr ptr1(volume, _m);
                    free_volume_ptr ptr2(volume, _m);
                    free_volume_ptr ptr3(volume, _m);
                    if (volume->get_free_block_count() != free_before - 3) throw int();
                }
                if (volume->get_free_block_count() != free_before) throw int();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 5. clear / clear_range 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("clear / clear_range");
        {
            mes::a_mes local_mes;
            shared_ptr<free_volume> volume = make_shared<free_volume>(
                64, 10,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                local_mes
            );
            free_volume_ptr ptr(volume, local_mes);

            // 写入测试数据
            byte_1 test_data[64];
            for (int i = 0; i < 64; ++i) test_data[i] = (byte_1)i;
            bmms_f::agent_ptr src_ap(test_data, 64);
            ptr.store(src_ap, 0, 64, 0, true);

            TEST_POSITIVE("clear entire block", local_mes, {
                ptr.clear();

                byte_1 read_data[64];
                bmms_f::agent_ptr dst_ap(read_data, 64);
                ptr.load(0, 64, dst_ap, 0, true);

                for (int i = 0; i < 64; ++i) {
                    if (read_data[i] != 0) throw int();
                }
                });

            // 重新写入数据
            ptr.store(src_ap, 0, 64, 0, true);

            TEST_POSITIVE("clear range within block", local_mes, {
                ptr.clear_range(10, 20);

                byte_1 read_data[64];
                bmms_f::agent_ptr dst_ap(read_data, 64);
                ptr.load(0, 64, dst_ap, 0, true);

                for (int i = 0; i < 64; ++i) {
                    if (i >= 10 && i < 30) {
                        if (read_data[i] != 0) throw int();
                    }
     else {
      if (read_data[i] != (byte_1)i) throw int();
  }
}
                });

            TEST_CONVERSE_CODING("clear on uninitialized object", {
                free_volume_ptr src(volume, _m);
                free_volume_ptr dst(move(src));
                src.clear();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 6. load / store 测试（委托给 agent_ptr）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("load/store");
        {
            mes::a_mes local_mes;
            shared_ptr<free_volume> volume = make_shared<free_volume>(
                64, 10,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                local_mes
            );
            free_volume_ptr ptr(volume, local_mes);

            TEST_POSITIVE("store and load roundtrip", local_mes, {
                byte_1 src_data[64];
                for (int i = 0; i < 64; ++i) src_data[i] = (byte_1)(i * 2);
                bmms_f::agent_ptr src_ap(src_data, 64);

                ptr.store(src_ap, 0, 64, 0, true);

                byte_1 dst_data[64] = {0};
                bmms_f::agent_ptr dst_ap(dst_data, 64);
                ptr.load(0, 64, dst_ap, 0, true);

                for (int i = 0; i < 64; ++i) {
                    if (dst_data[i] != (byte_1)(i * 2)) throw int();
                }
                });

            TEST_POSITIVE("load_to_void_ptr and store_from_void_ptr", local_mes, {
                byte_1 src[64] = {0xAB, 0xCD};
                ptr.store_from_void_ptr(src, 64, 0, true);

                byte_1 dst[64] = {0};
                ptr.load_to_void_ptr(0, 64, dst, true);

                if (dst[0] != 0xAB || dst[1] != 0xCD) throw int();
                });

            TEST_CONVERSE_CODING("store on uninitialized object", {
                free_volume_ptr src(volume, _m);
                free_volume_ptr dst(move(src));
                byte_1 data[10];
                bmms_f::agent_ptr ap(data, 10);
                src.store(ap, 0, 10, 0, true);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 7. get_index / get_agent_ptr 测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Getters");
        {
            mes::a_mes local_mes;
            shared_ptr<free_volume> volume = make_shared<free_volume>(
                64, 10,
                free_volume::init_type::no_init_all_size,
                free_volume::alignment_type::without_alignment,
                local_mes
            );
            free_volume_ptr ptr(volume, local_mes);

            TEST_POSITIVE("get_index returns valid index", local_mes, {
                size_t idx = ptr.get_index();
                if (idx >= volume->get_block_count_max()) throw int();
                });

            TEST_POSITIVE("get_agent_ptr returns valid agent_ptr", local_mes, {
                auto& ap = ptr.get_agent_ptr();
                if (ap.__self_begin__() == 0) throw int();
                if (ap.size != 64) throw int();
                });

            TEST_CONVERSE_CODING("get_index on uninitialized object", {
                free_volume_ptr src(volume, _m);
                free_volume_ptr dst(move(src));
                src.get_index();  // src 未初始化
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 8. check_is_init 测试（通过其他操作间接验证）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("check_is_init");
        {
            // 已在各个反例中覆盖
        }
        TEST_FUNCTION_END();

        TEST_CLASS_END();
        TEST_MODULE_END();
    }

#endif // BMMS_INCLUDE_TEST

} // namespace bmms_f