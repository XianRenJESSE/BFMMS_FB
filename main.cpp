#include "SIMPLE_TYPE_NAME.h"
#include <vector>
#include <string>
#include <functional>

import MES;
import BMMS;
import BFS;

// ========== BMMS 测试注册表 ==========
struct BMMSTestEntry {
    std::string name;
    std::function<void()> func;
};

vector<BMMSTestEntry> bmms_tests = {
    {"test agent_ptr",          []() { bmms_f::agent_ptr().test(); }},
    {"test static volume",      []() {
        mes::a_mes tmp_mes;
        bmms_f::static_volume v(
            1024, 10,
            bmms_f::static_volume::init_type::init_all_size,
            bmms_f::static_volume::alignment_type::without_alignment,
            tmp_mes
        );
        v.test();
    }},
    {"test static volume ptr", [](){ 
        mes::a_mes tmp_mes;

        shared_ptr<bmms_f::static_volume> vp = make_shared<bmms_f::static_volume>(
            1024, 10,
            bmms_f::static_volume::init_type::init_all_size,
            bmms_f::static_volume::alignment_type::without_alignment,
            tmp_mes
        );

        bmms_f::static_volume_ptr(vp, tmp_mes).test();
    }},
    {"test static volume part",[](){
        mes::a_mes tmp_mes;
        shared_ptr<bmms_f::static_volume> vp = make_shared<bmms_f::static_volume>(
            1024, 10,
            bmms_f::static_volume::init_type::init_all_size,
            bmms_f::static_volume::alignment_type::without_alignment,
            tmp_mes
        );
        bmms_f::static_volume_part(
            vp,
            10,
            bmms_f::static_volume_part::init_type::init_all_size,
            bmms_f::static_volume_part::control_type::not_allow_over_use,
            tmp_mes
        ).test();
    }},
    {"test free volume",       [](){
        mes::a_mes tmp_mes;
        bmms_f::free_volume(
            1024,10,
            bmms_f::free_volume::init_type::no_init_all_size,
            bmms_f::free_volume::alignment_type::without_alignment,
            tmp_mes
        ).test();
    }},
    {"test free volume ptr",   [](){
        mes::a_mes tmp_mes;
        shared_ptr<bmms_f::free_volume> vp = make_shared<bmms_f::free_volume>(
            1024,10,
            bmms_f::free_volume::init_type::init_all_size,
            bmms_f::free_volume::alignment_type::without_alignment,
            tmp_mes
        );
        bmms_f::free_volume_ptr(vp, tmp_mes).test();
    }},
    {"test free part",         [](){
        mes::a_mes tmp_mes;
        bmms_f::free_part(
            1024,10,
            bmms_f::free_part::init_type::no_init_all_size,
            tmp_mes
            ).test(); 
    }},
};

// ========== BFS 测试注册表 ==========
struct BFSTestEntry {
    std::string name;
    std::function<void()> func;
};

vector<BFSTestEntry> bfs_tests = {
    {"test bin file",          []() { bfs_f::bin_file().test(); }},
    {"test file control",      []() { bfs_f::dir_control::test(); }},
    {"test file_agent_ptr",    []() { bfs_f::file_agent_ptr().test(); }},
};

int main() {
    /*
    output << "file test" << endl;
    str path = "D:/lib/cache/test.bin";
    mes::a_mes mes_out;
    byte_8 value = 0x0123456789ABCDEF;
    bfs_f::bin_file handle = move(bfs_f::bin_file(
        path,
        4096,
        bfs_f::bin_file::init_type::cover_and_create,
        mes_out
    ));

    handle.store_from_void_ptr(
        &value,
        8,
        0,
        true,
        mes_out
    );
    byte_1 a_byte = 0;
    for (size_t i = 0; i < 8; i++) {
        handle.load_to_void_ptr(
            i,
            1,
            &a_byte,
            true,
            mes_out
        );
        for (size_t j = 0; j < 8; j++) {
            std::cout << (a_byte >> j & 1);

        }
        std::cout << " 0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(a_byte) << endl;
    }
    */

    ///*
    while (true) {
        size_t inp = 0;
        size_t base = 1;

        output <<"========== BMMS TEST ==========\n";
        for (size_t i = 0; i < bmms_tests.size(); ++i) {
            output <<to_string(base + i)<< " : " <<bmms_tests[i].name <<"\n";
        }

        output <<"========== BFS TEST ==========\n";
        for (size_t i = 0; i < bfs_tests.size(); ++i) {
            output <<to_string(base + bmms_tests.size() + i)<< " : " << bfs_tests[i].name << "\n";
        }

        output <<"Enter a number to choose a test: ";
        input >> inp;
        output <<"\n";

        // 执行选中的测试
        if (inp >= 1 && inp <= (size_t)bmms_tests.size()) {
            // BMMS 测试
            output <<bmms_tests[inp - 1].name << "\n";
            bmms_tests[inp - 1].func();
        }
        else if (inp > (size_t)bmms_tests.size() &&
            inp <= (size_t)(bmms_tests.size() + bfs_tests.size())) {
            // BFS 测试
            size_t bfs_index = inp - bmms_tests.size() - 1;
            output <<bfs_tests[bfs_index].name <<"\n";
            bfs_tests[bfs_index].func();
        }
        else {
            output <<"Invalid input! Please try again.\n\n";
        }
        
    }
    //*/
}