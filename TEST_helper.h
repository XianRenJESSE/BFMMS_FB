// TEST_helper.h
#pragma once
// test_helper_helper.h
// 符合 DOC_test_rule.txt 的 BFS/BMMS 单元测试辅助模块
// 使用方法：在测试文件开头 #include "test_helper_helper.h"

#include <iostream>
#include <string>
#include <functional>
#include <sstream>
#include <iomanip>
import MES;

namespace test_helper {

    // ---------- 测试上下文（单例）----------
    class TestContext {
    public:
        static TestContext& instance();

        // ----- 层级管理 -----
        void begin_module(const std::string& name);
        void end_module();
        void begin_class(const std::string& name);
        void end_class();
        void begin_function(const std::string& name);
        void end_function();

        // ----- 测试执行 -----
        template<typename Func>
        void run_positive(const std::string& desc, Func test_func, mes::a_mes& mes_out) {
            total_tests++;
            std::cout << "            " << desc << "\n";

            bool passed = false;
            try {
                test_func(mes_out);
                if (mes_out.code == 0) {
                    passed = true;
                }
            }
            catch (mes::a_mes e) {

            }
            catch (...) {
            }

            if (passed) {
                std::cout << "        [PASS]\n\n";
                pass_count++;
            }
            else {
                std::cout << "        [ERR][" << ++err_count << "] positive test failed\n\n";
            }
        }

        template<typename Func>
        void run_converse_service(const std::string& desc, size_t expected_code, Func test_func, mes::a_mes& mes_out) {
            mes_out = mes::a_mes(0, 0, "");
            total_tests++;
            std::cout << "        converse test [" << k++ << "]:\n";
            std::cout << "            " << desc << "\n";

            bool triggered = false;
            try {
                test_func(mes_out);
                if (mes_out.code == expected_code) {
                    mes_out.out();
                    std::cout << "        [PASS]\n";
                    pass_count++;
                    triggered = true;
                }
            }
            catch (mes::a_mes e) {
                std::cout << "        [PASS]  (coding error caught, though expecting service error)\n";
                pass_count++;
                triggered = true;
            }
            catch (...) {
            }

            if (!triggered) {
                std::cout << "        [ERR][" << ++err_count << "] converse test not pass\n";
            }
        }

        template<typename Func>
        void run_converse_coding(const std::string& desc, Func test_func) {
            total_tests++;
            std::cout << "        converse test [" << k++ << "]:\n";
            std::cout << "            " << desc << "\n";

            bool caught = false;
            try {
                mes::a_mes dummy_mes;
                test_func(dummy_mes);
            }
            catch (mes::a_mes e) {
                std::cout << "        [PASS]\n";
                pass_count++;
                caught = true;
            }
            catch (...) {
            }

            if (!caught) {
                std::cout << "        [ERR][" << ++err_count << "] converse test not pass\n";
            }
        }

        // ----- 统计 -----
        void inc_total();
        void inc_pass();

        // 允许测试宏访问内部变量
        size_t err_count = 0;
        size_t total_tests = 0;
        size_t pass_count = 0;

    private:
        TestContext() = default;
        void reset();

        std::string module_name;
        std::string class_name;
        std::string function_name;

        size_t i = 0;  // 类序号
        size_t j = 0;  // 函数序号
        size_t k = 0;  // 反例序号

        bool in_class = false;
        bool in_function = false;
    };

} // namespace test_helper

// ---------- 用户友好宏 ----------
// (宏定义保持不变)
#define TEST_MODULE_BEGIN(name) \
    test_helper::TestContext::instance().begin_module(name)

#define TEST_MODULE_END() \
    test_helper::TestContext::instance().end_module()

#define TEST_CLASS_BEGIN(name) \
    test_helper::TestContext::instance().begin_class(name)

#define TEST_CLASS_END() \
    test_helper::TestContext::instance().end_class()

#define TEST_FUNCTION_BEGIN(name) \
    test_helper::TestContext::instance().begin_function(name)

#define TEST_FUNCTION_END() \
    test_helper::TestContext::instance().end_function()

#define TEST_POSITIVE(desc, mes_out, ...) \
    do { \
        auto test_func = [&](mes::a_mes& _m) { __VA_ARGS__; }; \
        test_helper::TestContext::instance().run_positive(desc, test_func, mes_out); \
    } while(0)

#define TEST_CONVERSE_SERVICE(desc, expected_code, mes_out, ...) \
    do { \
        auto test_func = [&](mes::a_mes& _m) { __VA_ARGS__; }; \
        test_helper::TestContext::instance().run_converse_service(desc, expected_code, test_func, mes_out); \
    } while(0)

#define TEST_CONVERSE_CODING(desc, ...) \
    do { \
        auto test_func = [&](mes::a_mes& _m) { __VA_ARGS__; }; \
        test_helper::TestContext::instance().run_converse_coding(desc, test_func); \
    } while(0)

#define TEST_REPORT_ERROR(desc) \
    do { \
        std::cout << "        [ERR][" << ++test_helper::TestContext::instance().err_count << "] " << desc << "\n"; \
    } while(0)