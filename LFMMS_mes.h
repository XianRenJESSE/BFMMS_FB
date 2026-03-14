#pragma once
#include "SIMPLE_TYPE_NAME.h"
#include "INIT.h"

import MES;

namespace lfmms_m {

	namespace sug{
		inline mes::a_mes null() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "null"); };
	};

	namespace war {
		inline mes::a_mes null() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "null"); };
	};

	namespace err {
		inline mes::a_mes coding_err_lfmms_add_overflow() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_lfmms_add_overflow"); };
		inline mes::a_mes coding_err_lfmms_sub_overflow() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_lfmms_sub_overflow"); };
		inline mes::a_mes coding_err_lfmms_mul_overflow() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_lfmms_mul_overflow"); };
		inline mes::a_mes coding_err_lfmms_div_by_zero() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_lfmms_div_by_zero"); };
		inline mes::a_mes coding_err_lfmms_mod_by_zero() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_lfmms_mod_by_zero"); };
		inline mes::a_mes sys_err_logical_volume_check_this_ptr_null() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "sys_err_logical_volume_check_this_ptr_null"); };
		inline mes::a_mes coding_err_logical_volume_check_init_type_invalid() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_init_type_invalid"); };
		inline mes::a_mes coding_err_logical_volume_check_use_control_invalid() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_use_control_invalid"); };
		inline mes::a_mes coding_err_logical_volume_check_physical_block_size_zero() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_physical_block_size_zero"); };
		inline mes::a_mes coding_err_logical_volume_check_logical_block_size_zero() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_logical_block_size_zero"); };
		inline mes::a_mes coding_err_logical_volume_check_block_size_mismatch() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_block_size_mismatch"); };
		inline mes::a_mes coding_err_logical_volume_check_logical_block_alignment_type_invalid() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_logical_block_alignment_type_invalid"); };
		inline mes::a_mes coding_err_logical_volume_check_logical_block_alignment_control_invalid() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_logical_block_alignment_control_invalid"); };
		inline mes::a_mes coding_err_logical_volume_check_block_count_max_zero() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_block_count_max_zero"); };
		inline mes::a_mes coding_err_logical_volume_check_block_count_max_exceed() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_block_count_max_exceed"); };
		inline mes::a_mes coding_err_logical_volume_check_file_control_invalid() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_file_control_invalid"); };
		inline mes::a_mes coding_err_logical_volume_check_load_control_invalid() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_load_control_invalid"); };
		inline mes::a_mes coding_err_logical_volume_check_cache_control_invalid() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_cache_control_invalid"); };
		inline mes::a_mes coding_err_logical_volume_check_memory_cache_control_invalid() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_memory_cache_control_invalid"); };
		inline mes::a_mes coding_err_logical_volume_check_stack_accelerate_invalid() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_stack_accelerate_invalid"); };
		inline mes::a_mes coding_err_logical_volume_check_permission_invalid() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_permission_invalid"); };
		inline mes::a_mes coding_err_logical_volume_check_stack_accelerate_size_zero() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_stack_accelerate_size_zero"); };
		inline mes::a_mes coding_err_logical_volume_check_cache_count_max_zero() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_cache_count_max_zero"); };
		inline mes::a_mes coding_err_logical_volume_check_cache_count_mismatch() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_cache_count_mismatch"); };
		inline mes::a_mes coding_err_logical_volume_check_encryption_mismatch() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_encryption_mismatch"); };
		inline mes::a_mes coding_err_logical_volume_check_not_initialized() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_check_not_initialized"); };
		inline mes::a_mes coding_err_logical_volume_stack_control_invalid_index() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_stack_control_invalid_index"); };
		inline mes::a_mes coding_err_logical_volume_head_control_pair_not_zero() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_head_control_pair_not_zero"); };
		inline mes::a_mes coding_err_logical_volume_stack_control_invalid_value_pass() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_stack_control_invalid_value_pass"); };
		inline mes::a_mes coding_err_logical_volume_stack_control_use_after_free() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_err_logical_volume_stack_control_use_after_free"); };
		inline mes::a_mes sys_err_logical_volume_head_control_this_ptr_null() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "sys_err_logical_volume_head_control_this_ptr_null"); };
		inline mes::a_mes sys_err_logical_volume_head_control_bin_file_ptr_null() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "sys_err_logical_volume_head_control_bin_file_ptr_null"); };
		inline mes::a_mes service_err_logical_volume_head_control_memory_alloc_failed() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_head_control_memory_alloc_failed"); };
		inline mes::a_mes service_err_logical_volume_head_control_bad_get() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_head_control_bad_get"); };
		inline mes::a_mes service_err_logical_volume_head_control_bad_set() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_head_control_bad_set"); };
		inline mes::a_mes service_err_logical_volume_head_control_fix_failed() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_head_control_fix_failed"); };
		inline mes::a_mes service_err_logical_volume_head_control_fix_failed_zero_and_one_coune_is_equal() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_head_control_fix_failed_zero_and_one_coune_is_equal"); };
		inline mes::a_mes service_err_logical_volume_head_control_init_pair_failed() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_head_control_init_pair_failed"); };
		inline mes::a_mes service_err_logical_volume_stack_control_bad_get() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_stack_control_bad_get"); };
		inline mes::a_mes service_err_logical_volume_stack_control_bad_set() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_stack_control_bad_set"); };
		inline mes::a_mes service_err_logical_volume_stack_control_bad_data() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_stack_control_bad_data"); };
		inline mes::a_mes service_err_logical_volume_stack_control_fix_tie() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_stack_control_fix_tie"); };
		inline mes::a_mes service_err_logical_volume_stack_control_pair_zero_use_fix_4bit() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_stack_control_pair_zero_use_fix_4bit"); };
		inline mes::a_mes service_err_logical_volume_stack_control_pair_nonzero_use_fix_pair() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_stack_control_pair_nonzero_use_fix_pair"); };
		inline mes::a_mes service_err_logical_volume_stack_control_inconsistent() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_stack_control_inconsistent"); };
		inline mes::a_mes service_err_logical_volume_stack_control_decode_tie() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_stack_control_decode_tie"); };
		inline mes::a_mes service_err_logical_volume_stack_control_cas_failed() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_stack_control_cas_failed"); };
		inline mes::a_mes service_err_logical_volume_stack_control_empty() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "service_err_logical_volume_stack_control_empty"); };
		inline mes::a_mes sys_err_logical_volume_stack_control_block_not_free_but_stack_ptr_point_it() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "sys_err_logical_volume_stack_control_block_not_free_but_stack_ptr_point_it"); };
		inline mes::a_mes coding_error_cache_agent_ptr_make_type_over_range() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_error_cache_agent_ptr_make_type_over_range"); };
		inline mes::a_mes coding_error_cache_agent_ptr_make_type_again() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_error_cache_agent_ptr_make_type_again"); };
		inline mes::a_mes coding_error_cache_RAII_lock_build_from_null_ptr() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_error_cache_RAII_lock_build_from_null_ptr"); };
		inline mes::a_mes coding_error_cache_RAII_lock_build_from_not_init_handle() { return mes::a_mes(MOD_CODE_LFMMS, __LINE__, "coding_error_cache_RAII_lock_build_from_not_init_handle"); };
	};

};