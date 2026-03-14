// LFMMS_logical_volume_check.cpp
module LFMMS;

#include "LFMMS_dp.h"

namespace lfmms_f {
	#define EXIT_FAILURE 1

	#if defined(LFMMS_INCLUDE_TEST)
	#define __CODING_ERROR__(mes)\
			mes().out(); \
			throw mes();
	#else
	#define __CODING_ERROR__(mes)\
			 mes().out();std::exit(EXIT_FAILURE);
	#endif

	#define __SYS_ERROR__(mes)\
			mes().out(); \
			throw mes();

    #define __RSM__ mes_out = mes::a_mes()        //reset mes_out
    #define __CAR__ if(mes_out.code != 0)return   //check and return

	logical_volume::DRIVE::check::check(logical_volume* this_ptr_in) {
		if (this_ptr_in == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_check_this_ptr_null)
		}
		this_ptr = this_ptr_in;
	};

	void logical_volume::DRIVE::check::this_ptr_not_null() {
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_check_this_ptr_null)
		}
	};

    void logical_volume::DRIVE::check::init_value(mes::a_mes& mes_out) {
        this_ptr_not_null();

        // 1. 检查 init_type
        if (
            this_ptr->data_.init_type_ != logical_volume::init_type::no_init_all_size
            &&
            this_ptr->data_.init_type_ != logical_volume::init_type::init_all_size
            ) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_init_type_invalid)
        }

        // 2. 检查 use_control
        if (
            this_ptr->data_.use_control_ != logical_volume::use_control::not_allow_over_use
            &&
            this_ptr->data_.use_control_ != logical_volume::use_control::allow_over_use
            ) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_use_control_invalid)
        }

        // 3. 检查物理块大小（必须大于0）
        if (this_ptr->data_.physical_block_size_ == 0) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_physical_block_size_zero)
        }

        // 4. 检查逻辑块大小（必须大于0）
        if (this_ptr->data_.logical_block_size_ == 0) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_logical_block_size_zero)
        }

        // 5. 检查逻辑块大小不能超过物理块大小（除非允许跨越物理块边界）
        if (this_ptr->data_.logical_block_size_ > this_ptr->data_.physical_block_size_ &&
            this_ptr->data_.logical_block_alignment_control_ != logical_volume::logical_block_alignment_control::over_physical_block) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_block_size_mismatch)
        }

        // 6. 检查 logical_block_alignment_type
        if (
            this_ptr->data_.logical_block_alignment_type_ != logical_volume::logical_block_alignment_type::without_alignment &&
            this_ptr->data_.logical_block_alignment_type_ != logical_volume::logical_block_alignment_type::byte_4_each_block &&
            this_ptr->data_.logical_block_alignment_type_ != logical_volume::logical_block_alignment_type::byte_8_each_block &&
            this_ptr->data_.logical_block_alignment_type_ != logical_volume::logical_block_alignment_type::byte_16_each_block &&
            this_ptr->data_.logical_block_alignment_type_ != logical_volume::logical_block_alignment_type::byte_32_each_block &&
            this_ptr->data_.logical_block_alignment_type_ != logical_volume::logical_block_alignment_type::byte_64_each_block &&
            this_ptr->data_.logical_block_alignment_type_ != logical_volume::logical_block_alignment_type::byte_128_each_block
            ) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_logical_block_alignment_type_invalid)
        }

        // 7. 检查 logical_block_alignment_control
        if (
            this_ptr->data_.logical_block_alignment_control_ != logical_volume::logical_block_alignment_control::not_over_physical_block &&
            this_ptr->data_.logical_block_alignment_control_ != logical_volume::logical_block_alignment_control::over_physical_block
            ) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_logical_block_alignment_control_invalid)
        }

        // 8. 检查逻辑块最大数量（不能为0）
        if (this_ptr->data_.logical_block_count_max_ == 0) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_block_count_max_zero)
        }

        // 9. 检查逻辑块最大数量是否超过索引最大值
        if (this_ptr->data_.logical_block_count_max_ > index_max) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_block_count_max_exceed)
        }

        // 10. 检查 file_control
        if (
            this_ptr->data_.file_control_ != logical_volume::file_control::open_existed &&
            this_ptr->data_.file_control_ != logical_volume::file_control::cover_and_create &&
            this_ptr->data_.file_control_ != logical_volume::file_control::create
            ) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_file_control_invalid)
        }

        // 11. 检查 load_control
        if (
            this_ptr->data_.load_control_ != logical_volume::load_control::auto_load &&
            this_ptr->data_.load_control_ != logical_volume::load_control::not_auto_load
            ) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_load_control_invalid)
        }

        // 12. 检查 cache_control
        if (
            this_ptr->data_.cache_control_ != logical_volume::cache_control::static_size &&
            this_ptr->data_.cache_control_ != logical_volume::cache_control::dynamic_size
            ) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_cache_control_invalid)
        }

        // 13. 检查 memory_cache_control
        if (
            this_ptr->data_.memory_cache_control_ != logical_volume::memory_cache_control::no_cache &&
            this_ptr->data_.memory_cache_control_ != logical_volume::memory_cache_control::cache_all &&
            this_ptr->data_.memory_cache_control_ != logical_volume::memory_cache_control::cache_part
            ) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_memory_cache_control_invalid)
        }

        // 14. 检查 stack_accelerate
        if (
            this_ptr->data_.stack_accelerate_ != logical_volume::stack_accelerate::accelerate &&
            this_ptr->data_.stack_accelerate_ != logical_volume::stack_accelerate::not_accelerate
            ) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_stack_accelerate_invalid)
        }

        // 15. 检查 permission
        if (static_cast<int>(this_ptr->data_.permission_) > 0b0111 &&
            this_ptr->data_.permission_ != logical_volume::permission::no_init) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_permission_invalid)
        }

        // 16. 如果启用栈加速，检查栈加速大小
        if (this_ptr->data_.stack_accelerate_ == logical_volume::stack_accelerate::accelerate &&
            this_ptr->data_.stack_accelerate_size_ == 0) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_stack_accelerate_size_zero)
        }

        // 17. 检查缓存逻辑块最大数量
        if (this_ptr->data_.cache_logical_block_count_max_ == 0 &&
            this_ptr->data_.memory_cache_control_ != logical_volume::memory_cache_control::no_cache) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_cache_count_max_zero)
        }

        // 18. 如果内存缓存控制为 no_cache，检查缓存计数是否为0
        if (this_ptr->data_.memory_cache_control_ == logical_volume::memory_cache_control::no_cache &&
            this_ptr->data_.cache_logical_block_count_max_ != 0) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_cache_count_mismatch)
        }

        // 19. 检查加密函数配对（如果一个有值另一个没有，则错误）
        if ((this_ptr->data_.encryption_function_ == nullptr && this_ptr->data_.decryption_function_ != nullptr) ||
            (this_ptr->data_.encryption_function_ != nullptr && this_ptr->data_.decryption_function_ == nullptr)) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_encryption_mismatch)
        }

        // 20. 检查 is_init_ 状态
        if (!this_ptr->data_.is_init_) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_check_not_initialized)
        }
    }

    void logical_volume::DRIVE::check::strategy_matching(mes::a_mes& mes_out) {
        
    };
};