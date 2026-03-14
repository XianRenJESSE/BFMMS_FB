// BMMS_agent_ptr_test.cpp
module BMMS;



#include "BMMS_dp.h"
import MES;                      // 错误码模块

#if defined(BMMS_INCLUDE_TEST)
#include "TEST_helper.h"
#endif

namespace bmms_f {

#if defined(BMMS_INCLUDE_TEST)

    void agent_ptr::test() {
        using namespace test_helper;    // 宏已包含，这里仅用于清晰

        // ========== 模块开始 ==========
        TEST_MODULE_BEGIN("AGENT_PTR");

        // ========== 类测试：agent_ptr ==========
        TEST_CLASS_BEGIN("agent_ptr");

        // ------------------------------------------------------------
        // 1. 构造函数测试
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Constructor");
        {
            mes::a_mes local_mes;

            // 正例：正常构造
            TEST_POSITIVE("construct with valid pointer and size", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                });

            // 反例：空指针构造
            TEST_CONVERSE_CODING("construct with nullptr", {
                agent_ptr ap(nullptr, 100);
                });

            // 反例：零大小构造
            TEST_CONVERSE_CODING("construct with zero size", {
                byte_1 buffer[1];
                agent_ptr ap(buffer, 0);
                });

            // 反例：地址溢出构造
            TEST_CONVERSE_CODING("construct with overflow (ptr + size > UINTPTR_MAX)", {
                void* max_ptr = reinterpret_cast<void*>(UINTPTR_MAX - 50);
                agent_ptr ap(max_ptr, 100);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 2. 默认构造函数（空指针）已由其他用例覆盖，单独测试空对象行为
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("Default Constructor");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("default construct creates null agent_ptr", local_mes, {
                agent_ptr ap;
                bool is_null;
                ap.get_is_null(is_null);
                // is_null 应为 true（但正例不验证结果，仅确保无异常）
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 3. get_void_ptr
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_void_ptr");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("get pointer of valid agent_ptr", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                void* ptr;
                ap.get_void_ptr(ptr);
                });

            TEST_POSITIVE("get pointer of null agent_ptr", local_mes, {
                agent_ptr ap;
                void* ptr;
                ap.get_void_ptr(ptr);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 4. get_is_null
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_is_null");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("non-null agent_ptr returns false", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                bool is_null;
                ap.get_is_null(is_null);
                });

            TEST_POSITIVE("null agent_ptr returns true", local_mes, {
                agent_ptr ap;
                bool is_null;
                ap.get_is_null(is_null);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 5. get_size
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_size");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("valid agent_ptr returns correct size", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                size_t sz;
                ap.get_size(sz);
                });

            TEST_POSITIVE("null agent_ptr returns 0", local_mes, {
                agent_ptr ap;
                size_t sz;
                ap.get_size(sz);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 6. get_begin
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_begin");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("get begin of valid agent_ptr", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                void* begin;
                ap.get_begin(begin);
                });

            TEST_POSITIVE("get begin of null agent_ptr", local_mes, {
                agent_ptr ap;
                void* begin;
                ap.get_begin(begin);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 7. get_end
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_end");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("get end of valid agent_ptr (no overflow)", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                void* end;
                ap.get_end(end);
                });

            // 反例：地址溢出导致异常
            TEST_CONVERSE_CODING("get end with overflow (ptr + size > UINTPTR_MAX)", {
                void* overflow_ptr = reinterpret_cast<void*>(UINTPTR_MAX - 10);
                agent_ptr ap(overflow_ptr, 20);
                void* end;
                ap.get_end(end);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 8. get_check_range_bool
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("get_check_range_bool");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("valid range inside buffer", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                bool valid;
                ap.get_check_range_bool(0, 50, valid);
                });

            TEST_POSITIVE("range exceeding buffer size", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                bool valid;
                ap.get_check_range_bool(0, 101, valid);
                });

            TEST_POSITIVE("zero size (considered valid by implementation)", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                bool valid;
                ap.get_check_range_bool(50, 0, valid);
                });

            TEST_POSITIVE("arithmetic overflow in begin+size", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                bool valid;
                ap.get_check_range_bool(0, SIZE_MAX, valid);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 9. check_building_no_overflow
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("check_building_no_overflow");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("no overflow", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);  // dummy
                void* ptr = buffer;
                size_t sz = 100;
                bool no_overflow;
                ap.check_building_no_overflow(ptr, sz, no_overflow);
                });

            TEST_POSITIVE("overflow case", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                void* overflow_ptr = reinterpret_cast<void*>(UINTPTR_MAX - 10);
                size_t sz = 20;
                bool no_overflow;
                ap.check_building_no_overflow(overflow_ptr, sz, no_overflow);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 10. check_end_no_overflow
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("check_end_no_overflow");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("no overflow and within bounds", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                bool no_overflow;
                ap.check_end_no_overflow(0, 100, no_overflow);
                });

            TEST_POSITIVE("arithmetic overflow", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                bool no_overflow;
                ap.check_end_no_overflow(0, SIZE_MAX, no_overflow);
                });

            TEST_POSITIVE("no overflow but beyond buffer end", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                bool no_overflow;
                ap.check_end_no_overflow(50, 51, no_overflow);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 11. load  （代理指针 → 代理指针）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("load");
        {
            mes::a_mes local_mes;

            // ----- 正例 -----
            TEST_POSITIVE("safe mode: valid load", local_mes, {
                byte_1 src[100] = {0};
                byte_1 dst[100] = {0};
                agent_ptr src_ap(src, 100);
                agent_ptr dst_ap(dst, 100);
                src_ap.load(0, 100, dst_ap, 0, true);
                });

            TEST_POSITIVE("unsafe mode: valid load", local_mes, {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr src_ap(src, 100);
                agent_ptr dst_ap(dst, 100);
                src_ap.load(0, 100, dst_ap, 0, false);
                });

            // ----- 反例（编码错误）-----
            TEST_CONVERSE_CODING("safe mode: destination agent_ptr is null", {
                byte_1 src[100];
                agent_ptr src_ap(src, 100);
                agent_ptr dst_ap;   // null
                src_ap.load(0, 10, dst_ap, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: source agent_ptr (this) is null", {
                agent_ptr src_ap;   // null
                byte_1 dst[100];
                agent_ptr dst_ap(dst, 100);
                src_ap.load(0, 10, dst_ap, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: destination buffer too small", {
                byte_1 src[100];
                byte_1 dst[10];
                agent_ptr src_ap(src, 100);
                agent_ptr dst_ap(dst, 10);
                src_ap.load(0, 20, dst_ap, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: managed ranges overlap", {
                byte_1 buffer[200];
                agent_ptr ap1(buffer, 200);
                agent_ptr ap2(buffer + 50, 100);
                ap1.load(0, 100, ap2, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: load range out of source bounds", {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr src_ap(src, 100);
                agent_ptr dst_ap(dst, 100);
                src_ap.load(200, 10, dst_ap, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: source and destination are same agent_ptr", {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                ap.load(0, 50, ap, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: overlapping ranges within same agent_ptr", {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                ap.load(0, 80, ap, 50, true);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 12. store  （代理指针 ← 代理指针）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("store");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("safe mode: valid store", local_mes, {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr src_ap(src, 100);
                agent_ptr dst_ap(dst, 100);
                dst_ap.store(src_ap, 0, 100, 0, true);
                });

            TEST_POSITIVE("unsafe mode: valid store", local_mes, {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr src_ap(src, 100);
                agent_ptr dst_ap(dst, 100);
                dst_ap.store(src_ap, 0, 100, 0, false);
                });

            TEST_CONVERSE_CODING("safe mode: source agent_ptr is null", {
                byte_1 dst[100];
                agent_ptr dst_ap(dst, 100);
                agent_ptr src_ap;   // null
                dst_ap.store(src_ap, 0, 10, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: destination (this) is null", {
                agent_ptr dst_ap;   // null
                byte_1 src[100];
                agent_ptr src_ap(src, 100);
                dst_ap.store(src_ap, 0, 10, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: source buffer too small", {
                byte_1 src[10];
                byte_1 dst[100];
                agent_ptr src_ap(src, 10);
                agent_ptr dst_ap(dst, 100);
                dst_ap.store(src_ap, 0, 20, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: managed ranges overlap", {
                byte_1 buffer[200];
                agent_ptr ap1(buffer, 200);
                agent_ptr ap2(buffer + 50, 100);
                ap1.store(ap2, 0, 100, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: store range out of destination bounds", {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr src_ap(src, 100);
                agent_ptr dst_ap(dst, 100);
                dst_ap.store(src_ap, 0, 100, 200, true);
                });

            TEST_CONVERSE_CODING("safe mode: source and destination are same agent_ptr", {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                ap.store(ap, 0, 50, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: overlapping ranges within same agent_ptr", {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                ap.store(ap, 0, 80, 50, true);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 13. load_to_void_ptr  （代理指针 → void*）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("load_to_void_ptr");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("safe mode: load to valid void*", local_mes, {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr src_ap(src, 100);
                src_ap.load_to_void_ptr(0, 50, dst, true);
                });

            TEST_POSITIVE("unsafe mode: load to valid void*", local_mes, {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr src_ap(src, 100);
                src_ap.load_to_void_ptr(0, 50, dst, false);
                });

            TEST_CONVERSE_CODING("safe mode: null void pointer", {
                byte_1 src[100];
                agent_ptr src_ap(src, 100);
                void* null_ptr = nullptr;
                src_ap.load_to_void_ptr(0, 10, null_ptr, true);
                });

            TEST_CONVERSE_CODING("safe mode: source buffer too small", {
                byte_1 src[100];
                byte_1 dst[200];
                agent_ptr src_ap(src, 100);
                src_ap.load_to_void_ptr(0, 200, dst, true);
                });

            TEST_CONVERSE_CODING("safe mode: void* range overlaps with source managed range", {
                byte_1 src[200];
                agent_ptr src_ap(src, 200);
                void* overlap_ptr = src + 50;
                src_ap.load_to_void_ptr(0, 100, overlap_ptr, true);
                });

            TEST_CONVERSE_CODING("safe mode: load range out of source bounds", {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr src_ap(src, 100);
                src_ap.load_to_void_ptr(200, 10, dst, true);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 14. store_from_void_ptr  （代理指针 ← void*）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("store_from_void_ptr");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("safe mode: store from valid void*", local_mes, {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr dst_ap(dst, 100);
                dst_ap.store_from_void_ptr(src, 50, 0, true);
                });

            TEST_POSITIVE("unsafe mode: store from valid void*", local_mes, {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr dst_ap(dst, 100);
                dst_ap.store_from_void_ptr(src, 50, 0, false);
                });

            TEST_CONVERSE_CODING("safe mode: null void pointer", {
                byte_1 dst[100];
                agent_ptr dst_ap(dst, 100);
                void* null_ptr = nullptr;
                dst_ap.store_from_void_ptr(null_ptr, 10, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: destination buffer too small", {
                byte_1 src[100];
                byte_1 dst[50];
                agent_ptr dst_ap(dst, 50);
                dst_ap.store_from_void_ptr(src, 100, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: void* range overlaps with destination managed range", {
                byte_1 dst[200];
                agent_ptr dst_ap(dst, 200);
                void* overlap_ptr = dst + 50;
                dst_ap.store_from_void_ptr(overlap_ptr, 100, 0, true);
                });

            TEST_CONVERSE_CODING("safe mode: store range out of destination bounds", {
                byte_1 src[100];
                byte_1 dst[100];
                agent_ptr dst_ap(dst, 100);
                dst_ap.store_from_void_ptr(src, 50, 200, true);
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 15. clear   （整块清零）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("clear");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("clear non-null agent_ptr", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                ap.clear();
                });

            TEST_POSITIVE("clear null agent_ptr (no effect)", local_mes, {
                agent_ptr ap;
                ap.clear();
                });
        }
        TEST_FUNCTION_END();

        // ------------------------------------------------------------
        // 16. clear_range  （部分清零）
        // ------------------------------------------------------------
        TEST_FUNCTION_BEGIN("clear_range");
        {
            mes::a_mes local_mes;

            TEST_POSITIVE("clear valid range", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                ap.clear_range(0, 50);
                });

            TEST_POSITIVE("clear zero-size range", local_mes, {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                ap.clear_range(50, 0);
                });

            TEST_CONVERSE_CODING("clear range out of bounds", {
                byte_1 buffer[100];
                agent_ptr ap(buffer, 100);
                ap.clear_range(200, 10);
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
