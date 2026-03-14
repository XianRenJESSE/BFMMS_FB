//SIMPLE_TYPE_NAME.h

#pragma once
import <any>;
import <array>;
import <atomic>;
import <algorithm>;
import <bitset>;
import <chrono>;
import <cctype>;
import <cstdint>;
import <cstring>;
import <cstdlib>;
import <cassert>;
import <condition_variable>;
import <concepts>;
import <memory>;
import <format>;
import <fstream>;
import <functional>;
import <filesystem>;
import <iomanip>;
import <iostream>;
import <limits>;
import <map>;
import <mutex>;
import <queue>;
import <random>;
import <set>;
import <span>;
import <string>;
import <string_view>;
import <sstream>;
import <stdexcept>;
import <shared_mutex>;
import <thread>;
import <tuple>;
import <type_traits>;
import <utility>;
import <unordered_map>;
import <unordered_set>;
import <vector>;
import <stdint.h>;

//常用即简单类型定义
#define any      std::any
#define str      std::string
#define to_string std::to_string
#define str_view std::string_view
#define vector   std::vector
#define hash_map std::unordered_map
#define hash_set std::unordered_set

#define bitset        std::bitset
#define byte          std::byte
#define byte_1        std::uint8_t
#define byte_2        std::uint16_t
#define byte_4        std::uint32_t
#define byte_8        std::uint64_t
#define atomic_byte_1 std::atomic_uint8_t
#define atomic_byte_2 std::atomic_uint16_t
#define atomic_byte_4 std::atomic_uint32_t
#define atomic_byte_8 std::atomic_uint64_t

#define shared_ptr std::shared_ptr
#define weak_ptr   std::weak_ptr
#define unique_ptr std::unique_ptr

#define memcpy std::memcpy
#define memset std::memset
#define move std::move

#define make_shared std::make_shared
#define make_unique std::make_unique

#define make_pair  std::make_pair

#define atomic     std::atomic

#define lock_sha   std::shared_mutex
#define lock_mtx   std::mutex

#define print      std::printf

#define input      std::cin
#define output     std::cout
#define endl       std::endl

#define as_const std::as_const

#define tuple    std::tuple

#define memory_order_acquire std::memory_order_acquire
#define memory_order_release std::memory_order_release
#define memory_order_relaxed std::memory_order_relaxed
#define memory_order_consume std::memory_order_consume
#define memory_order_acq_rel std::memory_order_acq_rel
#define memory_order_seq_cst std::memory_order_seq_cst

#define f_s  std::filesystem