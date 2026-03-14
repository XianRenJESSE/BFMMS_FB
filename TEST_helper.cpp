// TEST_helper.cpp

#include "TEST_helper.h"

namespace test_helper {

    // ---------- 测试上下文（单例）实现 ----------
    TestContext& TestContext::instance() {
        static TestContext ctx;
        return ctx;
    }

    void TestContext::begin_module(const std::string& name) {
        module_name = name;
        err_count = 0;
        i = 0;
        j = 0;
        k = 0;
        in_class = false;
        in_function = false;
        std::cout << "\n============================== " << module_name
            << " MOD TEST BEGIN ==============================\n";
        std::cout << "err_count = " << err_count << "\n\n";
    }

    void TestContext::end_module() {
        std::cout << "\n[SUMMARY]\n";
        std::cout << "[ALL TEST COUNT] " << total_tests << "\n";
        std::cout << "[PASS TEST COUNT] " << pass_count << "\n";
        std::cout << "[ERR TEST COUNT] " << err_count << "\n";
        std::cout << "============================== " << module_name
            << " MOD TEST END ==============================\n\n";
        reset();
    }

    void TestContext::begin_class(const std::string& name) {
        if (in_class) end_class();
        class_name = name;
        i++;
        j = 0;
        k = 0;
        in_class = true;
        in_function = false;
        std::cout << "[" << i << "] " << class_name << " test : \n";
    }

    void TestContext::end_class() {
        if (in_function) end_function();
        in_class = false;
        class_name.clear();
        std::cout << "\n";
    }

    void TestContext::begin_function(const std::string& name) {
        if (!in_class) {
            std::cerr << "Error: begin_function() called without begin_class()\n";
            return;
        }
        if (in_function) end_function();
        function_name = name;
        j++;
        k = 0;
        in_function = true;
        std::cout << "[" << i << "." << j << "] " << function_name << " test :\n";
        std::cout << "[" << i << "." << j << "] original statement test :\n";
    }

    void TestContext::end_function() {
        in_function = false;
        function_name.clear();
    }

    void TestContext::inc_total() {
        total_tests++;
    }

    void TestContext::inc_pass() {
        pass_count++;
    }

    void TestContext::reset() {
        module_name.clear();
        class_name.clear();
        function_name.clear();
        err_count = 0;
        total_tests = 0;
        pass_count = 0;
        i = j = k = 0;
        in_class = in_function = false;
    }

} // namespace test_helper