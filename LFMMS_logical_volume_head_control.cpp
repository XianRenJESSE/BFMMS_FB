// LFMMS_logical_volume_head_control.cpp
module LFMMS;

#include "LFMMS_dp.h"
namespace {  // 匿名命名空间，内部链接
	static uint64_t crc64_table[256];
	static bool crc64_table_initialized = false;

	static void init_crc64_table() {
		uint64_t poly = 0xC96C5795D7870F42ULL; // ECMA-182 多项式
		for (int i = 0; i < 256; ++i) {
			uint64_t crc = i;
			for (int j = 0; j < 8; ++j) {
				crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
			}
			crc64_table[i] = crc;
		}
		crc64_table_initialized = true;
	}

	static uint64_t crc64(const uint8_t* data, size_t len) {
		if (!crc64_table_initialized) {
			init_crc64_table();
		}
		uint64_t crc = ~0ULL;
		for (size_t i = 0; i < len; ++i) {
			crc = (crc >> 8) ^ crc64_table[(crc ^ data[i]) & 0xFF];
		}
		return ~crc;
	}
}

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
	#define LDH logical_volume::DRIVE::head_control

	LDH::head_control(logical_volume* this_ptr_in){
		if (this_ptr_in == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		this_ptr = this_ptr_in;
	};

	void LDH::get(
		size_t load_begin_in,
		bmms_f::agent_ptr& data_agent_ptr_out,
		mes::a_mes& mes_out
	) {
		//1 重置消息
		__RSM__;
		//2 校验指针
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (this_ptr->data_.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}
		//3 申请缓存
		unique_ptr<byte_1[]> cache = nullptr;
		size_t load_size = math::nf_mul(data_agent_ptr_out.size, 8);
		try {
			cache = make_unique<byte_1[]>(load_size);
		}
		catch(...){
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
			return;
		}
		bmms_f::agent_ptr cache_ap = move(
			bmms_f::agent_ptr(cache.get(), load_size)
		);
		cache_ap.clear();

		//4 读取数据
		this_ptr->data_.bin_file_ptr_->load_to_agent_ptr(
			load_begin_in,
			load_size,
			cache_ap,
			0,
			true,
			mes_out
		);
		__CAR__;

		//5 转换数据
		size_t byte_index = 0;
		size_t bit_offset = 0;
		byte_1 tmp_res = 0;
		byte_1 a_byte = 0;

		for (size_t i = 0; i < load_size; i++) {
			cache_ap.load_to_void_ptr(
				i,
				1,
				&a_byte,
				true
			);

			// 检查数据有效性
			if (a_byte != 0x00 && a_byte != 0xFF) {
				mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_get();
				return;
			}

			byte_index = math::nf_div(i, 8);      // 当前处理的是第几个输出字节
			bit_offset = math::nf_mod(i, 8);      // 在当前输出字节中的第几位 (0-7)

			if (a_byte == 0xFF) {
				tmp_res |= (1 << bit_offset);     // 直接使用 bit_offset，不颠倒
			}
			// 0x00 的情况不需要操作，因为 tmp_res 初始为0

			if (bit_offset == 7) {  // 每8个字节完成一个目标字节
				data_agent_ptr_out.store_from_void_ptr(
					&tmp_res,
					1,
					byte_index,
					true
				);
				tmp_res = 0;  // 重置，准备下一个字节
			}
		}
	};

	void LDH::set(
		size_t store_begin_in,
		bmms_f::agent_ptr& data_agent_ptr_in,
		mes::a_mes& mes_out
	) {
		//1 重置消息
		__RSM__;
		//2 校验指针
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (this_ptr->data_.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}
		//3 申请缓存
		unique_ptr<byte_1[]> cache      = nullptr;
		size_t               store_size = math::nf_mul(data_agent_ptr_in.size, 8);
		try {
			cache = make_unique<byte_1[]>(math::nf_mul(store_size,2));
		}
		catch (...) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
			return;
		}
		bmms_f::agent_ptr cache_ap = move(
			bmms_f::agent_ptr(cache.get(), store_size)
		);
		bmms_f::agent_ptr read_value_ap = move(
			bmms_f::agent_ptr(
				reinterpret_cast<void*>(
					math::nf_add(
						reinterpret_cast<uintptr_t>(cache.get()), 
						store_size
					)
				), 
				store_size
			)
		);
		cache_ap.clear();
		read_value_ap.clear();

		//4 转译数据
		size_t byte_index = 0;
		size_t bit_offset = 0;
		byte_1 a_byte     = 0;
		byte_1 tmp_value  = 0xFF;
		for (size_t i = 0; i < data_agent_ptr_in.size; i = math::nf_add(i,1)) {
			byte_index = i;
			data_agent_ptr_in.load_to_void_ptr(
				byte_index,
				1,
				&a_byte,
				true
			);
			for (bit_offset = 0; bit_offset < 8; bit_offset = math::nf_add(bit_offset, 1)) {
				if ((a_byte >> bit_offset & 1) == 1) {
					cache_ap.store_from_void_ptr(
						&tmp_value,
						1,
						math::nf_add(math::nf_mul(byte_index,8), bit_offset),
						true
					);
				}
			}
		}

		//5 写入数据（带重试）
		const int max_retry     = 2;  // 最多尝试两次
		int       attempt       = 0;
		bool      write_success = false;
		this_ptr->data_.bin_file_ptr_->flush();
		while (!write_success && attempt < max_retry) {
			// 重置消息，准备新的一次尝试
			__RSM__;

			//5.1 置空标记
			size_t flag_value = 0;
			size_t pair_flag_value = 0xFFFFFFFFFFFFFFFF;
			size_t flag_begin = math::nf_add(store_begin_in, store_size);
			if (math::nf_mod(math::nf_div(flag_begin,4096),2) == 0) {
				this_ptr->data_.bin_file_ptr_->clear_range(
					flag_begin,
					8,
					mes_out
				);
				this_ptr->data_.bin_file_ptr_->flush();
			}
			else {
				this_ptr->data_.bin_file_ptr_->store_from_void_ptr(
					&pair_flag_value,
					8,
					math::nf_add(store_begin_in, store_size),
					true,
					mes_out
				);
				this_ptr->data_.bin_file_ptr_->flush();
			}
			if (mes_out.code != 0) {
				if (attempt == 0) {
					// 第一次失败，重试
					attempt++;
					continue;
				}
				else {
					// 第二次失败，返回错误
					mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
					return;
				}
			}

			//5.2 验证标记
			this_ptr->data_.bin_file_ptr_->load_to_void_ptr(
				math::nf_add(store_begin_in, store_size),
				8,
				&flag_value,
				true,
				mes_out
			);
			if (math::nf_mod(math::nf_div(flag_begin, 4096), 2) == 0) {
				if (mes_out.code != 0 || flag_value != 0) {
					if (attempt == 0) {
						attempt++;
						continue;
					}
					else {
						mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
						return;
					}
				}
			}
			else {
				if (mes_out.code != 0 || flag_value != 0xFFFFFFFFFFFFFFFF) {
					if (attempt == 0) {
						attempt++;
						continue;
					}
					else {
						mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
						return;
					}
				}
			}
			
			//5.3 写入值
			this_ptr->data_.bin_file_ptr_->store_from_agent_ptr(
				cache_ap,
				0,
				store_size,
				store_begin_in,
				true,
				false,
				mes_out
			);
			this_ptr->data_.bin_file_ptr_->flush();
			if (mes_out.code != 0) {
				if (attempt == 0) {
					// 第一次失败，尝试清理标记后重试
					if (math::nf_mod(math::nf_div(flag_begin, 4096), 2) == 0) {
						this_ptr->data_.bin_file_ptr_->clear_range(
							flag_begin,
							8,
							mes_out
						);
						this_ptr->data_.bin_file_ptr_->flush();
					}
					else {
						this_ptr->data_.bin_file_ptr_->store_from_void_ptr(
							&pair_flag_value,
							8,
							math::nf_add(store_begin_in, store_size),
							true,
							mes_out
						);
						this_ptr->data_.bin_file_ptr_->flush();
					}
					// 忽略清理标记的错误，直接重试
					attempt++;
					continue;
				}
				else {
					mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
					return;
				}
			}

			//5.4 验证值
			this_ptr->data_.bin_file_ptr_->load_to_agent_ptr(
				store_begin_in,
				store_size,
				read_value_ap,
				0,
				true,
				mes_out
			);
			if (mes_out.code != 0) {
				if (attempt == 0) {
					attempt++;
					continue;
				}
				else {
					return;
				}
			}

			bool   data_ok = true;
			byte_1 write_value = 0;
			byte_1 read_value  = 0;
			for (size_t i = 0; i < store_size; i = math::nf_add(i, 1)) {
				cache_ap.load_to_void_ptr(i, 1, &write_value, true);
				read_value_ap.load_to_void_ptr(i, 1, &read_value, true);
				if (write_value != read_value) {
					data_ok = false;
					break;
				}
			}

			if (data_ok) {
				//5.5 打标记
				flag_value      = 0xFFFFFFFFFFFFFFFF;
				pair_flag_value = 0;
				if (math::nf_mod(math::nf_div(flag_begin, 4096), 2) == 0) {
					this_ptr->data_.bin_file_ptr_->store_from_void_ptr(
						&flag_value,
						8,
						math::nf_add(store_begin_in, store_size),
						true,
						mes_out
					);
					this_ptr->data_.bin_file_ptr_->flush();
				}
				else {
					this_ptr->data_.bin_file_ptr_->store_from_void_ptr(
						&pair_flag_value,
						8,
						math::nf_add(store_begin_in, store_size),
						true,
						mes_out
					);
					this_ptr->data_.bin_file_ptr_->flush();
				}
				if (mes_out.code != 0) {
					if (attempt == 0) {
						attempt++;
						continue;
					}
					else {
						mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
						return;
					}
				}

				//5.5.2 验证标记
				this_ptr->data_.bin_file_ptr_->load_to_void_ptr(
					math::nf_add(store_begin_in, store_size),
					8,
					&flag_value,
					true,
					mes_out
				);
				if (math::nf_mod(math::nf_div(flag_begin, 4096), 2) == 0) {
					if (mes_out.code != 0 || flag_value != 0xFFFFFFFFFFFFFFFF) {
						if (attempt == 0) {
							attempt++;
							continue;
						}
						else {
							mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
							return;
						}
					}
				}
				else {
					if (mes_out.code != 0 || flag_value != 0) {
						if (attempt == 0) {
							attempt++;
							continue;
						}
						else {
							mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
							return;
						}
					}
				}
				
				write_success = true;  // 成功！
				break;
			}
			else {
				// 数据验证失败
				if (attempt == 0) {
					// 第一次失败，清理标记后重试
					flag_value = 0;
					this_ptr->data_.bin_file_ptr_->store_from_void_ptr(
						&flag_value,
						8,
						math::nf_add(store_begin_in, store_size),
						true,
						mes_out
					);
					this_ptr->data_.bin_file_ptr_->flush();
					// 忽略清理标记的错误
					attempt++;
					continue;
				}
				else {
					mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
					return;
				}
			}
		}

		if (!write_success) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
			return;
		}
	};

	void LDH::fix_by_pair(
		mes::a_mes& mes_out
	) {
		//1 重置消息
		__RSM__;
		//2 校验指针
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (this_ptr->data_.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}
		//3 获取pair
		byte_1 pair = 0;
		bmms_f::agent_ptr pair_ap = move(bmms_f::agent_ptr(&pair, sizeof(byte_1)));
		get(
			this_ptr->data_.head_off_.pair,
			pair_ap,
			mes_out
		);
		__CAR__;

		//4 校验是否有pair供修复
		if (pair == 0) {
			return;
		}

		//4 申请缓存
		size_t               head_size = math::nf_mul(math::nf_mul(pair,2), 4096);
		unique_ptr<byte_1[]> data       = nullptr;
		unique_ptr<size_t[]> bit_counts = nullptr;

		try {
			data       = make_unique<byte_1[]>(head_size);
			bit_counts = make_unique<size_t[]>(math::nf_mul(pair, 4));
		}
		catch (...) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
			return;
		}
		bmms_f::agent_ptr head_ap = bmms_f::agent_ptr(data.get(), head_size);
		head_ap.clear();

		bool   chk_res = false;
		size_t retry_limit = 2;
		while (retry_limit > 0) {
			//4 读取数据
			this_ptr->data_.bin_file_ptr_->load_to_agent_ptr(
				0,
				head_size,
				head_ap,
				0,
				true,
				mes_out
			);
			__CAR__;

			//5 处理数据
			byte_1 first_head_data = 0;
			byte_1 other_head_data = 0;
			size_t zero_index = 0;
			size_t one_index = 0;
			char   name[15] = "LOGICAL_VOLUME";
			char   ch = 0;
			size_t total_zero_count = 0;
			size_t total_one_count = 0;
			byte_1 fix_value = 0;

			for (size_t i = 0; i < 4096; i = math::nf_add(i, 1)) {
				head_ap.load_to_void_ptr(
					i,
					1,
					&first_head_data,
					true
				);

				// 修复name错误
				if (i < 14) {
					if (first_head_data != name[i]) {
						ch = name[i];
						head_ap.store_from_void_ptr(
							&ch,
							1,
							i,
							true
						);
					}
				}
				// 计数1和0
				if (i >= 64) {
					bit_counts[0] = 0;
					bit_counts[1] = 0;
					if (~first_head_data & 0b00000001)bit_counts[0]++;
					if (~first_head_data & 0b00000010)bit_counts[0]++;
					if (~first_head_data & 0b00000100)bit_counts[0]++;
					if (~first_head_data & 0b00001000)bit_counts[0]++;
					if (~first_head_data & 0b00010000)bit_counts[0]++;
					if (~first_head_data & 0b00100000)bit_counts[0]++;
					if (~first_head_data & 0b01000000)bit_counts[0]++;
					if (~first_head_data & 0b10000000)bit_counts[0]++;
					if (first_head_data & 0b00000001)bit_counts[1]++;
					if (first_head_data & 0b00000010)bit_counts[1]++;
					if (first_head_data & 0b00000100)bit_counts[1]++;
					if (first_head_data & 0b00001000)bit_counts[1]++;
					if (first_head_data & 0b00010000)bit_counts[1]++;
					if (first_head_data & 0b00100000)bit_counts[1]++;
					if (first_head_data & 0b01000000)bit_counts[1]++;
					if (first_head_data & 0b10000000)bit_counts[1]++;
				}

				for (size_t j = 1; j < math::nf_mul(pair, 2); j = math::nf_add(j, 1)) {
					head_ap.load_to_void_ptr(
						math::nf_add(math::nf_mul(4096, j), i),
						1,
						&other_head_data,
						true
					);
					// 修复name错误
					if (i < 14) {
						if (math::nf_mod(j, 2) == 0) {
							if (other_head_data != name[i]) {
								ch = name[i];
								head_ap.store_from_void_ptr(
									&ch,
									1,
									math::nf_add(math::nf_mul(4096, j), i),
									true
								);
							}
						}
						else {
							if (other_head_data != ~name[i]) {
								ch = ~name[i];
								head_ap.store_from_void_ptr(
									&ch,
									1,
									math::nf_add(math::nf_mul(4096, j), i),
									true
								);
							}
						}
					}
					// 计数1和0
					if (i >= 64) {
						zero_index = math::nf_mul(j, 2);
						one_index = math::nf_add(zero_index, 1);
						bit_counts[zero_index] = 0;
						bit_counts[one_index] = 0;
						if (math::nf_mod(j, 2) == 0) {
							if (~other_head_data & 0b00000001)bit_counts[zero_index]++;
							if (~other_head_data & 0b00000010)bit_counts[zero_index]++;
							if (~other_head_data & 0b00000100)bit_counts[zero_index]++;
							if (~other_head_data & 0b00001000)bit_counts[zero_index]++;
							if (~other_head_data & 0b00010000)bit_counts[zero_index]++;
							if (~other_head_data & 0b00100000)bit_counts[zero_index]++;
							if (~other_head_data & 0b01000000)bit_counts[zero_index]++;
							if (~other_head_data & 0b10000000)bit_counts[zero_index]++;
							if (other_head_data & 0b00000001)bit_counts[one_index]++;
							if (other_head_data & 0b00000010)bit_counts[one_index]++;
							if (other_head_data & 0b00000100)bit_counts[one_index]++;
							if (other_head_data & 0b00001000)bit_counts[one_index]++;
							if (other_head_data & 0b00010000)bit_counts[one_index]++;
							if (other_head_data & 0b00100000)bit_counts[one_index]++;
							if (other_head_data & 0b01000000)bit_counts[one_index]++;
							if (other_head_data & 0b10000000)bit_counts[one_index]++;
						}
						else {
							if (other_head_data & 0b00000001)bit_counts[zero_index]++;
							if (other_head_data & 0b00000010)bit_counts[zero_index]++;
							if (other_head_data & 0b00000100)bit_counts[zero_index]++;
							if (other_head_data & 0b00001000)bit_counts[zero_index]++;
							if (other_head_data & 0b00010000)bit_counts[zero_index]++;
							if (other_head_data & 0b00100000)bit_counts[zero_index]++;
							if (other_head_data & 0b01000000)bit_counts[zero_index]++;
							if (other_head_data & 0b10000000)bit_counts[zero_index]++;
							if (~other_head_data & 0b00000001)bit_counts[one_index]++;
							if (~other_head_data & 0b00000010)bit_counts[one_index]++;
							if (~other_head_data & 0b00000100)bit_counts[one_index]++;
							if (~other_head_data & 0b00001000)bit_counts[one_index]++;
							if (~other_head_data & 0b00010000)bit_counts[one_index]++;
							if (~other_head_data & 0b00100000)bit_counts[one_index]++;
							if (~other_head_data & 0b01000000)bit_counts[one_index]++;
							if (~other_head_data & 0b10000000)bit_counts[one_index]++;
						}
					}
				}

				// 根据统计数据修复
				if (i >= 64) {
					// 重置计数
					total_zero_count = 0;
					total_one_count = 0;
					// 聚合数据
					for (size_t k = 0; k < math::nf_mul(pair, 2); k++) {
						total_zero_count = math::nf_add(total_zero_count, bit_counts[math::nf_mul(k, 2)]);
						total_one_count = math::nf_add(total_one_count, bit_counts[math::nf_add(math::nf_mul(k, 2), 1)]);
					}
					// 修复数据
					if (total_zero_count > total_one_count) {
						fix_value = 0;
						head_ap.store_from_void_ptr(
							&fix_value,
							1,
							i,
							true
						);
						for (size_t j = 1; j < math::nf_mul(pair, 2); j = math::nf_add(j, 1)) {
							if (math::nf_mod(j, 2) == 0) {
								head_ap.store_from_void_ptr(
									&fix_value,
									1,
									math::nf_add(math::nf_mul(4096, j), i),
									true
								);
							}
							else {
								fix_value = 0xFF;
								head_ap.store_from_void_ptr(
									&fix_value,
									1,
									math::nf_add(math::nf_mul(4096, j), i),
									true
								);
							}
						}
					}
					if (total_zero_count < total_one_count) {
						fix_value = 0xFF;
						head_ap.store_from_void_ptr(
							&fix_value,
							1,
							i,
							true
						);
						for (size_t j = 1; j < math::nf_mul(pair, 2); j = math::nf_add(j, 1)) {
							if (math::nf_mod(j, 2) == 0) {
								head_ap.store_from_void_ptr(
									&fix_value,
									1,
									math::nf_add(math::nf_mul(4096, j), i),
									true
								);
							}
							else {
								fix_value = 0;
								head_ap.store_from_void_ptr(
									&fix_value,
									1,
									math::nf_add(math::nf_mul(4096, j), i),
									true
								);
							}
						}
					}
					if (total_zero_count == total_one_count) {
						mes_out = lfmms_m::err::service_err_logical_volume_head_control_fix_failed_zero_and_one_coune_is_equal();
						return;
					}
				}
			}

			//6 写回数据
			this_ptr->data_.bin_file_ptr_->store_from_agent_ptr(
				head_ap,
				0,
				head_size,
				0,
				true,
				true,
				mes_out
			);
			__CAR__;
			this_ptr->data_.bin_file_ptr_->flush();

			//7 检验数据,含重试

			chk_by_pair(chk_res, mes_out);
			__CAR__;
			if (chk_res) {
				break;
			}
			else {
				retry_limit--;
			}
		}
		if (!chk_res) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_fix_failed();
		}
	};

	void LDH::fix_by_1byte_ECC(
		mes::a_mes& mes_out
	) {
		//1 重置消息
		__RSM__;

		//2 校验指针
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (this_ptr->data_.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}

		//3 获取pair并确认pair=0
		byte_1 pair = 0;
		bmms_f::agent_ptr pair_ap = move(bmms_f::agent_ptr(&pair, sizeof(byte_1)));
		get(
			this_ptr->data_.head_off_.pair,
			pair_ap,
			mes_out
		);
		__CAR__;

		//4 检查pair值
		if (pair != 0) {
			// pair不为0时不应该调用此函数
			__CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_head_control_pair_not_zero)
		}

		//5 申请缓存
		size_t head_size = 4096;  // pair=0时只有一个头
		unique_ptr<byte_1[]> data = nullptr;
		unique_ptr<size_t[]> bit_counts = nullptr;

		try {
			data = make_unique<byte_1[]>(head_size);
			bit_counts = make_unique<size_t[]>(256);  // 每个字节8位，但每位单独统计
		}
		catch (...) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
			return;
		}
		bmms_f::agent_ptr head_ap = bmms_f::agent_ptr(data.get(), head_size);
		head_ap.clear();

		bool chk_res = false;
		size_t retry_limit = 2;

		while (retry_limit > 0) {
			//6 读取数据
			this_ptr->data_.bin_file_ptr_->load_to_agent_ptr(
				0,
				head_size,
				head_ap,
				0,
				true,
				mes_out
			);
			__CAR__;

			//7 处理数据 - 按字节修复
			// name字段 (前64字节) 特殊处理
			const char expected_name[15] = "LOGICAL_VOLUME";

			for (size_t i = 0; i < head_size; i = math::nf_add(i, 1)) {
				byte_1 current_byte = 0;
				head_ap.load_to_void_ptr(i, 1, &current_byte, true);

				// 处理name字段 (前14字节)
				if (i < 14) {
					if (current_byte != expected_name[i]) {
						// name字段直接修复为目标字符
						char fix_char = expected_name[i];
						head_ap.store_from_void_ptr(&fix_char, 1, i, true);
					}
					continue;  // name字段不参与位投票
				}

				// 处理其他字段 - 按位投票修复
				// 统计当前字节中1的个数
				size_t one_count = 0;
				for (size_t bit = 0; bit < 8; bit++) {
					if (current_byte & (1 << bit)) {
						one_count++;
					}
				}
				size_t zero_count = math::nf_sub(8, one_count);

				// 决定修复值
				byte_1 fixed_byte;
				if (one_count > zero_count) {
					// 1更多，修复为0xFF
					fixed_byte = 0xFF;
				}
				else if (zero_count > one_count) {
					// 0更多，修复为0x00
					fixed_byte = 0x00;
				}
				else {
					// 4个1和4个0，平局无法修复
					// 这种情况应该极少见，但需要处理
					// 尝试查看相邻字节的规律？
					if (i > 0 && i < head_size - 1) {
						byte_1 prev_byte = 0, next_byte = 0;
						head_ap.load_to_void_ptr(math::nf_sub(i, 1), 1, &prev_byte, true);
						head_ap.load_to_void_ptr(math::nf_add(i, 1), 1, &next_byte, true);

						// 如果前后字节一致，则采用相同值
						if (prev_byte == next_byte && (prev_byte == 0x00 || prev_byte == 0xFF)) {
							fixed_byte = prev_byte;
						}
						else {
							// 仍无法确定，报告错误
							mes_out = lfmms_m::err::service_err_logical_volume_head_control_fix_failed_zero_and_one_coune_is_equal();
							return;
						}
					}
					else {
						mes_out = lfmms_m::err::service_err_logical_volume_head_control_fix_failed_zero_and_one_coune_is_equal();
						return;
					}
				}

				// 写回修复后的字节
				if (fixed_byte != current_byte) {
					head_ap.store_from_void_ptr(&fixed_byte, 1, i, true);
				}
			}

			//8 写回数据
			this_ptr->data_.bin_file_ptr_->store_from_agent_ptr(
				head_ap,
				0,
				head_size,
				0,
				true,
				true,
				mes_out
			);
			__CAR__;
			this_ptr->data_.bin_file_ptr_->flush();

			//9 检验数据
			chk_by_pair(chk_res, mes_out);
			__CAR__;

			if (chk_res) {
				break;  // 修复成功
			}
			else {
				retry_limit--;
			}
		}

		if (!chk_res) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_fix_failed();
		}
	}

	void LDH::chk_by_pair(
		bool& chk_res_out,
		mes::a_mes& mes_out
	) {
		//1 重置消息
		__RSM__;
		//2 校验指针
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (this_ptr->data_.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}
		//3 获取pair
		byte_1 pair = 0;
		bmms_f::agent_ptr pair_ap = move(bmms_f::agent_ptr(&pair,sizeof(byte_1)));
		get(
			this_ptr->data_.head_off_.pair,
			pair_ap,
			mes_out
		);
		__CAR__;

		//4 校验是否有pair供校验
		if (pair == 0) {
			chk_res_out = true;
			return;
		}

		//4 申请缓存
		size_t               head_size = math::nf_mul(math::nf_mul(pair, 2), 4096);
		unique_ptr<byte_1[]> data      = nullptr;
		try {
			data = make_unique<byte_1[]>(head_size);
		}
		catch (...) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
			return;
		}
		bmms_f::agent_ptr head_ap = bmms_f::agent_ptr(data.get(), head_size);
		head_ap.clear();

		//4 读取数据
		this_ptr->data_.bin_file_ptr_->load_to_agent_ptr(
			0,
			head_size,
			head_ap,
			0,
			true,
			mes_out
		);
		__CAR__;

		//5 对比数据
		byte_1 first_head_data = 0;
		byte_1 other_head_data = 0;
		for (size_t i = 0; i < 4096;i = math::nf_add(i,1)) {
			head_ap.load_to_void_ptr(
				i,
				1,
				&first_head_data,
				true
			);
			for (size_t j = 1; j < math::nf_mul(pair, 2); j = math::nf_add(j, 1)) {
				head_ap.load_to_void_ptr(
					math::nf_add(math::nf_mul(4096, j), i),
					1,
					&other_head_data,
					true
				);
				if (first_head_data != 0x00 && first_head_data != 0xFF) {
					chk_res_out = false;
					return;
				}
				if (math::nf_mod(j, 2) == 0) {
					if (first_head_data != other_head_data) {
						chk_res_out = false;
						return;
					}
					if ( i >= 64 && other_head_data != 0x00 && other_head_data != 0xFF) {
						chk_res_out = false;
						return;
					}
				}
				else {
					if (first_head_data != static_cast<byte_1>(~other_head_data)) {
						chk_res_out = false;
						return;
					}
					if ( i >= 64 && other_head_data != 0x00 && other_head_data != 0xFF) {
						chk_res_out = false;
						return;
					}
				}
			}
			
		}
		chk_res_out = true;
	};

	void LDH::init_pair(
		mes::a_mes& mes_out
	) {
		__RSM__;

		// 1. 校验指针
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (this_ptr->data_.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}

		// 2. 获取 pair 值
		byte_1 pair = 0;
		bmms_f::agent_ptr pair_ap(&pair, 1);
		get(this_ptr->data_.head_off_.pair, pair_ap, mes_out);
		__CAR__;

		// 3. 如果 pair == 0，不需要复制
		if (pair == 0) {
			return;
		}

		// 4. 计算头大小和申请缓存
		size_t head_count = math::nf_mul(pair, 2);
		size_t head_size = math::nf_mul(head_count, 4096);
		auto data = make_unique<byte_1[]>(head_size);
		if (!data) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
			return;
		}
		bmms_f::agent_ptr head_ap(data.get(), head_size);
		head_ap.clear();

		// 5. 读取第一个头（索引0）
		this_ptr->data_.bin_file_ptr_->load_to_agent_ptr(0, 4096, head_ap, 0, true, mes_out);
		__CAR__;

		// 6. 复制数据到其他头，注意奇偶取反
		for (size_t h = 1; h < head_count; ++h) {
			size_t dest_off = math::nf_mul(h, 4096);

			if (math::nf_mod(h, 2) == 0) {
				// 偶数头：直接复制第一个头的数据
				this_ptr->data_.bin_file_ptr_->store_from_agent_ptr(
					head_ap, 0, 4096, dest_off, true, false, mes_out);
			}
			else {
				// 奇数头：取反后写入
				auto temp_buf = make_unique<byte_1[]>(4096);
				if (!temp_buf) {
					mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
					return;
				}
				bmms_f::agent_ptr temp_ap(temp_buf.get(), 4096);

				// 逐字节取反
				for (size_t i = 0; i < 4096; ++i) {
					byte_1 val;
					head_ap.load_to_void_ptr(i, 1, &val, true);
					byte_1 not_val = ~val;
					temp_ap.store_from_void_ptr(&not_val, 1, i, true);
				}

				this_ptr->data_.bin_file_ptr_->store_from_agent_ptr(
					temp_ap, 0, 4096, dest_off, true, false, mes_out);
			}
			__CAR__;
		}

		this_ptr->data_.bin_file_ptr_->flush();

		// 7. 验证复制结果（使用 chk_by_pair）
		bool chk_ok = false;
		const int max_retry = 2;
		int retry = 0;

		while (retry < max_retry) {
			chk_by_pair(chk_ok, mes_out);
			__CAR__;

			if (chk_ok) {
				break;  // 验证通过
			}

			// 验证失败，重试
			if (retry == 0) {
				// 重新执行复制（重用当前逻辑，通过递归调用）
				// 为了避免递归，这里可以重新执行复制部分
				for (size_t h = 1; h < head_count; ++h) {
					size_t dest_off = math::nf_mul(h, 4096);

					if (math::nf_mod(h, 2) == 0) {
						this_ptr->data_.bin_file_ptr_->store_from_agent_ptr(
							head_ap, 0, 4096, dest_off, true, false, mes_out);
					}
					else {
						auto temp_buf = make_unique<byte_1[]>(4096);
						if (!temp_buf) {
							mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
							return;
						}
						bmms_f::agent_ptr temp_ap(temp_buf.get(), 4096);

						for (size_t i = 0; i < 4096; ++i) {
							byte_1 val;
							head_ap.load_to_void_ptr(i, 1, &val, true);
							byte_1 not_val = ~val;
							temp_ap.store_from_void_ptr(&not_val, 1, i, true);
						}

						this_ptr->data_.bin_file_ptr_->store_from_agent_ptr(
							temp_ap, 0, 4096, dest_off, true, false, mes_out);
					}
					__CAR__;
				}
				this_ptr->data_.bin_file_ptr_->flush();
				retry++;
			}
			else {
				// 第二次失败
				mes_out = lfmms_m::err::service_err_logical_volume_head_control_init_pair_failed();
				return;
			}
		}

		if (!chk_ok) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_init_pair_failed();
		}
	}

	void LDH::name(
		op          op_in, 
		str&        str_io,
		mes::a_mes& mes_out
	) {
		__RSM__;
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (this_ptr->data_.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}

		const size_t len = 64;
		size_t off = this_ptr->data_.head_off_.name;
		size_t flag_off = math::nf_add(off, len);  // name 后面的 flag 位置

		if (op_in == op::get) {
			// 先检查 flag
			size_t flag_value = 0;
			this_ptr->data_.bin_file_ptr_->load_to_void_ptr(flag_off, 8, &flag_value, true, mes_out);
			__CAR__;

			// 根据头奇偶判断 flag 有效性
			bool flag_ok = false;
			if (math::nf_mod(math::nf_div(off, 4096), 2) == 0) {
				flag_ok = (flag_value == 0xFFFFFFFFFFFFFFFF);
			}
			else {
				flag_ok = (flag_value == 0);
			}

			if (!flag_ok) {
				mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_get();
				return;
			}

			// flag 有效，读取数据
			char buf[64] = { 0 };
			this_ptr->data_.bin_file_ptr_->load_to_void_ptr(off, len, buf, true, mes_out);
			__CAR__;
			str_io.assign(buf, len);
		}
		else { // set
			const int max_retry = 2;
			int attempt = 0;
			bool success = false;

			while (!success && attempt < max_retry) {
				__RSM__;

				// 5.1 清空标记
				size_t flag_clear = (math::nf_mod(math::nf_div(off, 4096), 2) == 0) ? 0 : 0xFFFFFFFFFFFFFFFF;
				this_ptr->data_.bin_file_ptr_->store_from_void_ptr(&flag_clear, 8, flag_off, true, mes_out);
				this_ptr->data_.bin_file_ptr_->flush();
				if (mes_out.code != 0) {
					if (attempt == 0) { attempt++; continue; }
					else { mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set(); return; }
				}

				// 5.2 验证标记
				size_t flag_read = 0;
				this_ptr->data_.bin_file_ptr_->load_to_void_ptr(flag_off, 8, &flag_read, true, mes_out);
				if (mes_out.code != 0 || flag_read != flag_clear) {
					if (attempt == 0) { attempt++; continue; }
					else { mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set(); return; }
				}

				// 5.3 准备数据缓冲区
				char buf[64] = { 0 };
				size_t copy_len = str_io.size() < len ? str_io.size() : len;
				bmms_f::agent_ptr buf_ap(buf, len);
				buf_ap.store_from_void_ptr(const_cast<void*>(static_cast<const void*>(str_io.c_str())),
					copy_len, 0, true);

				// 5.4 写入数据
				this_ptr->data_.bin_file_ptr_->store_from_void_ptr(buf, len, off, true, mes_out);
				this_ptr->data_.bin_file_ptr_->flush();
				if (mes_out.code != 0) {
					if (attempt == 0) {
						// 清理标记后重试
						this_ptr->data_.bin_file_ptr_->store_from_void_ptr(&flag_clear, 8, flag_off, true, mes_out);
						this_ptr->data_.bin_file_ptr_->flush();
						attempt++;
						continue;
					}
					else {
						mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
						return;
					}
				}

				// 5.5 验证数据
				char verify_buf[64] = { 0 };
				this_ptr->data_.bin_file_ptr_->load_to_void_ptr(off, len, verify_buf, true, mes_out);
				if (mes_out.code != 0) {
					if (attempt == 0) { attempt++; continue; }
					else return;
				}

				bool data_ok = (std::memcmp(buf, verify_buf, len) == 0);

				if (data_ok) {
					// 5.6 写入完成标记
					size_t flag_done = (math::nf_mod(math::nf_div(off, 4096), 2) == 0) ? 0xFFFFFFFFFFFFFFFF : 0;
					this_ptr->data_.bin_file_ptr_->store_from_void_ptr(&flag_done, 8, flag_off, true, mes_out);
					this_ptr->data_.bin_file_ptr_->flush();
					if (mes_out.code != 0) {
						if (attempt == 0) { attempt++; continue; }
						else { mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set(); return; }
					}

					// 验证完成标记
					size_t flag_check = 0;
					this_ptr->data_.bin_file_ptr_->load_to_void_ptr(flag_off, 8, &flag_check, true, mes_out);
					if (mes_out.code != 0 || flag_check != flag_done) {
						if (attempt == 0) { attempt++; continue; }
						else { mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set(); return; }
					}

					success = true;
					break;
				}
				else {
					// 数据验证失败
					if (attempt == 0) {
						// 清理标记后重试
						this_ptr->data_.bin_file_ptr_->store_from_void_ptr(&flag_clear, 8, flag_off, true, mes_out);
						this_ptr->data_.bin_file_ptr_->flush();
						attempt++;
						continue;
					}
					else {
						mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
						return;
					}
				}
			}

			if (!success) {
				mes_out = lfmms_m::err::service_err_logical_volume_head_control_bad_set();
			}
		}
	}

	void LDH::head_check_sum_value(
		op          op_in, 
		size_t&     sum_value_io,
		mes::a_mes& mes_out
	) {
		__RSM__;
		size_t off = this_ptr->data_.head_off_.checksum;
		bmms_f::agent_ptr ap(&sum_value_io, sizeof(size_t));
		if (op_in == op::get)
			get(off, ap, mes_out);
		else
			set(off, ap, mes_out);
	}

	void LDH::version(
		op          op_in, 
		size_t&     version_io, 
		mes::a_mes& mes_out
	) {
		__RSM__;
		size_t off = this_ptr->data_.head_off_.version;
		bmms_f::agent_ptr ap(&version_io, sizeof(size_t));
		if (op_in == op::get)
			get(off, ap, mes_out);
		else
			set(off, ap, mes_out);
	}

	void LDH::init_type(
		op                         op_in, 
		logical_volume::init_type& init_type_io, 
		mes::a_mes&                mes_out
	) {
		__RSM__;
		byte_1 val = static_cast<byte_1>(init_type_io);
		bmms_f::agent_ptr ap(&val, 1);
		size_t off = this_ptr->data_.head_off_.init_type;
		if (op_in == op::get) {
			get(off, ap, mes_out);
			if (mes_out.code == 0)
				init_type_io = static_cast<logical_volume::init_type>(val);
		}
		else {
			set(off, ap, mes_out);
		}
	}

	void LDH::use_control(
		op                           op_in, 
		logical_volume::use_control& use_control_io, 
		mes::a_mes&                  mes_out
	) {
		__RSM__;
		byte_1 val = static_cast<byte_1>(use_control_io);
		bmms_f::agent_ptr ap(&val, 1);
		size_t off = this_ptr->data_.head_off_.use_control;
		if (op_in == op::get) {
			get(off, ap, mes_out);
			if (mes_out.code == 0)
				use_control_io = static_cast<logical_volume::use_control>(val);
		}
		else {
			set(off, ap, mes_out);
		}
	}

	void LDH::physical_block_size(
		op          op_in, 
		size_t&     size_io,
		mes::a_mes& mes_out
	) {
		__RSM__;
		size_t off = this_ptr->data_.head_off_.physical_block_size;
		bmms_f::agent_ptr ap(&size_io, sizeof(size_t));
		if (op_in == op::get)
			get(off, ap, mes_out);
		else
			set(off, ap, mes_out);
	}

	void LDH::logical_block_size(
		op          op_in, 
		size_t&     size_io,
		mes::a_mes& mes_out
	) {
		__RSM__;
		size_t off = this_ptr->data_.head_off_.logical_block_size;
		bmms_f::agent_ptr ap(&size_io, sizeof(size_t));
		if (op_in == op::get)
			get(off, ap, mes_out);
		else
			set(off, ap, mes_out);
	}

	void LDH::logical_block_alignment_type(
		op op_in, 
		logical_volume::logical_block_alignment_type& type_io, 
		mes::a_mes& mes_out
	) {
		__RSM__;
		byte_1 val = static_cast<byte_1>(type_io);
		bmms_f::agent_ptr ap(&val, 1);
		size_t off = this_ptr->data_.head_off_.alignment_type;
		if (op_in == op::get) {
			get(off, ap, mes_out);
			if (mes_out.code == 0)
				type_io = static_cast<logical_volume::logical_block_alignment_type>(val);
		}
		else {
			set(off, ap, mes_out);
		}
	}

	void LDH::logical_block_alignment_control(
		op                                               op_in, 
		logical_volume::logical_block_alignment_control& ctrl_io, 
		mes::a_mes&                                      mes_out
	) {
		__RSM__;
		byte_1 val = static_cast<byte_1>(ctrl_io);
		bmms_f::agent_ptr ap(&val, 1);
		size_t off = this_ptr->data_.head_off_.alignment_control;
		if (op_in == op::get) {
			get(off, ap, mes_out);
			if (mes_out.code == 0)
				ctrl_io = static_cast<logical_volume::logical_block_alignment_control>(val);
		}
		else {
			set(off, ap, mes_out);
		}
	}

	void LDH::logical_block_count_max(
		op     op_in,
		size_t& logical_block_count_max_io,
		mes::a_mes& mes_out
	) {
		__RSM__;

		// 1. 校验指针
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (this_ptr->data_.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}

		// 2. 获取字段偏移（使用更新后的偏移量）
		size_t off = this_ptr->data_.head_off_.logical_block_count_max;  // 应为 424

		// 3. 创建代理指针包装输入/输出值
		// 注意：size_t 是 8 字节，但在文件中用 64 字节表示（1字节当1bit）
		// 所以这里我们传递的是原始 size_t 值，get/set 内部会处理编解码
		bmms_f::agent_ptr ap(reinterpret_cast<void*>(&logical_block_count_max_io), sizeof(size_t));

		// 4. 调用通用的 get/set 方法
		if (op_in == op::get) {
			get(off, ap, mes_out);
		}
		else { // set
			set(off, ap, mes_out);
		}
	}


	void LDH::cache_control(
		op                             op_in, 
		logical_volume::cache_control& ctrl_io,
		mes::a_mes&                    mes_out
	) {
		__RSM__;
		byte_1 val = static_cast<byte_1>(ctrl_io);
		bmms_f::agent_ptr ap(&val, 1);
		size_t off = this_ptr->data_.head_off_.cache_control;
		if (op_in == op::get) {
			get(off, ap, mes_out);
			if (mes_out.code == 0)
				ctrl_io = static_cast<logical_volume::cache_control>(val);
		}
		else {
			set(off, ap, mes_out);
		}
	}

	void LDH::memory_cache_control(
		op                                    op_in,
		logical_volume::memory_cache_control& ctrl_io, 
		mes::a_mes&                            mes_out
	) {
		__RSM__;
		byte_1 val = static_cast<byte_1>(ctrl_io);
		bmms_f::agent_ptr ap(&val, 1);
		size_t off = this_ptr->data_.head_off_.memory_cache_control;
		if (op_in == op::get) {
			get(off, ap, mes_out);
			if (mes_out.code == 0)
				ctrl_io = static_cast<logical_volume::memory_cache_control>(val);
		}
		else {
			set(off, ap, mes_out);
		}
	}

	void LDH::stack_accelerate(
		op                                op_in, 
		logical_volume::stack_accelerate& accel_io,
		mes::a_mes&                       mes_out
	) {
		__RSM__;
		byte_1 val = static_cast<byte_1>(accel_io);
		bmms_f::agent_ptr ap(&val, 1);
		size_t off = this_ptr->data_.head_off_.stack_accelerate;
		if (op_in == op::get) {
			get(off, ap, mes_out);
			if (mes_out.code == 0)
				accel_io = static_cast<logical_volume::stack_accelerate>(val);
		}
		else {
			set(off, ap, mes_out);
		}
	}

	void LDH::pair(
		op          op_in, 
		byte_1&     pair_io,
		mes::a_mes& mes_out
	) {
		__RSM__;
		bmms_f::agent_ptr ap(&pair_io, 1);
		size_t off = this_ptr->data_.head_off_.pair;
		if (op_in == op::get) {
			get(off, ap, mes_out);
		}
		else {
			set(off, ap, mes_out);
		}
	}

	void LDH::permission(
		op                          op_in, 
		logical_volume::permission& perm_io,
		mes::a_mes&                 mes_out
	) {
		__RSM__;
		byte_1 val = static_cast<byte_1>(perm_io);
		bmms_f::agent_ptr ap(&val, 1);
		size_t off = this_ptr->data_.head_off_.permission;
		if (op_in == op::get) {
			get(off, ap, mes_out);
			if (mes_out.code == 0)
				perm_io = static_cast<logical_volume::permission>(val);
		}
		else {
			set(off, ap, mes_out);
		}
	}

	void LDH::password_encryption_result(
		op          op_in,
		size_t&     pwd_io,
		mes::a_mes& mes_out
	) {
		__RSM__;
		size_t off = this_ptr->data_.head_off_.password_encryption;
		bmms_f::agent_ptr ap(&pwd_io, sizeof(size_t));
		if (op_in == op::get)
			get(off, ap, mes_out);
		else
			set(off, ap, mes_out);
	}

	void LDH::created_time(
		op          op_in, 
		byte_8&     time_io, 
		mes::a_mes& mes_out) {
		__RSM__;
		size_t off = this_ptr->data_.head_off_.created_time;
		bmms_f::agent_ptr ap(&time_io, sizeof(byte_8));
		if (op_in == op::get)
			get(off, ap, mes_out);
		else
			set(off, ap, mes_out);
	}

	void LDH::last_change_time(
		op          op_in, 
		byte_8&     time_io, 
		mes::a_mes& mes_out
	) {
		__RSM__;
		size_t off = this_ptr->data_.head_off_.last_change_time;
		bmms_f::agent_ptr ap(&time_io, sizeof(byte_8));
		if (op_in == op::get)
			get(off, ap, mes_out);
		else
			set(off, ap, mes_out);
	}

	void LDH::init_head(mes::a_mes& mes_out) {
		__RSM__;

		// 校验指针
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (this_ptr->data_.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}

		// 设置各字段
		str volume_name = "LOGICAL_VOLUME";
		name(op::set, volume_name, mes_out);
		__CAR__;

		size_t ver = lfmms_f::version;
		version(op::set, ver, mes_out);
		__CAR__;

		init_type(op::set, this_ptr->data_.init_type_, mes_out);
		__CAR__;

		use_control(op::set, this_ptr->data_.use_control_, mes_out);
		__CAR__;

		physical_block_size(op::set, this_ptr->data_.physical_block_size_, mes_out);
		__CAR__;

		logical_block_size(op::set, this_ptr->data_.logical_block_size_, mes_out);
		__CAR__;

		logical_block_alignment_type(op::set, this_ptr->data_.logical_block_alignment_type_, mes_out);
		__CAR__;

		logical_block_alignment_control(op::set, this_ptr->data_.logical_block_alignment_control_, mes_out);
		__CAR__;

		cache_control(op::set, this_ptr->data_.cache_control_, mes_out);
		__CAR__;

		memory_cache_control(op::set, this_ptr->data_.memory_cache_control_, mes_out);
		__CAR__;

		stack_accelerate(op::set, this_ptr->data_.stack_accelerate_, mes_out);
		__CAR__;

		byte_1 pair_val = this_ptr->data_.pair_;
		pair(op::set, pair_val, mes_out);
		__CAR__;

		permission(op::set, this_ptr->data_.permission_, mes_out);
		__CAR__;

		size_t pwd_res = this_ptr->data_.key_encryption_result_;
		password_encryption_result(op::set, pwd_res, mes_out);
		__CAR__;

		uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		byte_8 now_byte8;
		{
			bmms_f::agent_ptr now_ap(&now_byte8, sizeof(now_byte8));
			now_ap.store_from_void_ptr(&now, sizeof(now_byte8), 0, true);
		}

		created_time(op::set, now_byte8, mes_out);
		__CAR__;

		last_change_time(op::set, now_byte8, mes_out);
		__CAR__;

		// 读取整个头用于计算校验和
		byte_1 head_buf[4096];
		bmms_f::agent_ptr head_ap(head_buf, 4096);
		this_ptr->data_.bin_file_ptr_->load_to_agent_ptr(0, 4096, head_ap, 0, true, mes_out);
		__CAR__;

		// 将校验和区域清零
		size_t checksum_off = this_ptr->data_.head_off_.checksum;
		head_ap.clear_range(checksum_off, 64);
		
		// 计算 CRC64
		uint64_t crc = crc64(reinterpret_cast<const uint8_t*>(head_buf), 4096);
		size_t crc_val = static_cast<size_t>(crc);

		// 写入校验和
		head_check_sum_value(op::set, crc_val, mes_out);
		__CAR__;

		// 复制配对头
		init_pair(mes_out);
		__CAR__;
	}
};