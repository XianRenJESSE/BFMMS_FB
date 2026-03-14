// LFMMS_logical_volume_stack_control.cpp
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
	#define DATA this_ptr->data_
    #define LDS logical_volume::DRIVE::stack_control

	LDS::stack_control(logical_volume* this_ptr_in) {
		if (this_ptr_in == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		this_ptr = this_ptr_in;
	}

    void LDS::init_stack_info() {
        //2 校验指针
        if (this_ptr == nullptr) {
            __SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
        }
        if (DATA.bin_file_ptr_ == nullptr) {
            __SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
        }
        //2 准备变量

        //获取与计算关键数据
        byte_1 pair = 0;
        pair = DATA.pair_;

        // logical_block_stack_top_offset_offset
        if (pair == 0) {
            DATA.stack_info_.logical_block_stack_top_offset_0_offset = 4096;
        }
        else {//pair最大256，该运算不溢出
            DATA.stack_info_.logical_block_stack_top_offset_0_offset = 4096 * 2 * pair;
        }

        // logical_block_stack_top_offset_1_offset
        DATA.stack_info_.logical_block_stack_top_offset_1_offset =
            math::nf_add(
                DATA.stack_info_.logical_block_stack_top_offset_0_offset,
                (32 + 32 * (2 * pair - 1))//该值最大16384
            );

        //logical_block_stack_top_offset_flag_offset
        DATA.stack_info_.logical_block_stack_top_offset_flag_offset =
            math::nf_add(
                DATA.stack_info_.logical_block_stack_top_offset_1_offset,
                (32 + 32 * (2 * pair - 1))//该值最大16384
            );

        //logical_block_stack_offset
        DATA.stack_info_.logical_block_stack_offset =
            math::nf_add(
                DATA.stack_info_.logical_block_stack_top_offset_flag_offset,
                8  // flag 占8字节，虽然只用1字节，但按8字节对齐
            );

        //using_range / using_group / stack_group_count / stack_group_size / logical_block_stack_size
        if (DATA.logical_block_alignment_control_
            ==
            logical_volume::logical_block_alignment_control::not_over_physical_block)
        {
            // 如果逻辑块大小小于物理块大小 使用组式表达
            if (DATA.logical_block_size_ <= DATA.physical_block_size_) {
                DATA.stack_info_.using_group = true;
            }
            else {
                __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_stack_control_invalid_value_pass)
            }

            // 每个物理块包含的逻辑块数量
            size_t blocks_per_physical = math::nf_div(
                DATA.physical_block_size_,
                DATA.logical_block_size_
            );

            // 栈组数量 = 逻辑块总数 / 每个物理块的逻辑块数
            DATA.stack_info_.stack_group_count = math::nf_div(
                DATA.logical_block_count_max_,
                blocks_per_physical
            );

            // 计算单个栈条目的大小（带ECC扩展）
            size_t handle_version_size = 32 + 32 * (2 * pair - 1);
            size_t can_pop_size = 4 + 4 * (2 * pair - 1);
            size_t logical_block_index_size = 28 + 28 * (2 * pair - 1);
            size_t a_stack_info_size = math::nf_add(
                math::nf_add(handle_version_size, can_pop_size),
                logical_block_index_size
            );

            // physical_block_offset 大小
            size_t physical_block_offset_size = 32 + 32 * (2 * pair - 1);

            // 每个栈组的大小 = physical_block_offset + a_stack_info * blocks_per_physical
            DATA.stack_info_.stack_group_size = math::nf_add(
                physical_block_offset_size,
                math::nf_mul(a_stack_info_size, blocks_per_physical)
            );

            // 总栈区大小 = 栈组数量 * 每组大小
            DATA.stack_info_.logical_block_stack_size = math::nf_mul(
                DATA.stack_info_.stack_group_count,
                DATA.stack_info_.stack_group_size
            );
        }
        else {  // over_physical_block
            DATA.stack_info_.using_range = true;

            // 区间式：每个逻辑块对应一个栈条目
            size_t logical_block_offset_size = 32 + 32 * (2 * pair - 1);
            size_t handle_version_size = 32 + 32 * (2 * pair - 1);
            size_t can_pop_size = 4 + 4 * (2 * pair - 1);
            size_t logical_block_index_size = 28 + 28 * (2 * pair - 1);

            // 每个栈条目的大小 = logical_block_offset + handle_version + can_pop + logical_block_index
            size_t per_entry_size = math::nf_add(
                math::nf_add(logical_block_offset_size, handle_version_size),
                math::nf_add(can_pop_size, logical_block_index_size)
            );

            DATA.stack_info_.logical_block_stack_size = math::nf_mul(
                DATA.logical_block_count_max_,
                per_entry_size
            );
        }

        // 计算4K对齐填充
        size_t stack_end = math::nf_add(
            DATA.stack_info_.logical_block_stack_offset,
            DATA.stack_info_.logical_block_stack_size
        );

        DATA.stack_info_._4K_alignment_fill_offset = stack_end;
        DATA.stack_info_._4K_alignment_fill_size =
            math::nf_mod(math::nf_sub(4096, math::nf_mod(stack_end, 4096)), 4096);

        // 物理块区域起始偏移
        DATA.stack_info_.physical_blocks_offset = math::nf_add(
            DATA.stack_info_._4K_alignment_fill_offset,
            DATA.stack_info_._4K_alignment_fill_size
        );
        // 计算 actually_logical_block_size
        if (pair == 0) {
            DATA.stack_info_.actually_logical_block_size_ = math::nf_mul(DATA.logical_block_size_, 4);
        }
        else {
            DATA.stack_info_.actually_logical_block_size_ = math::nf_mul(DATA.logical_block_size_, math::nf_mul(8, pair));
        }

        // 物理块区域总大小
        if (DATA.stack_info_.using_group) {
            // 不跨越模式：每个物理块内放固定数量的逻辑块
            size_t blocks_per_physical = math::nf_div(
                DATA.physical_block_size_,
                DATA.logical_block_size_
            );

            size_t physical_block_count = math::nf_div(
                DATA.logical_block_count_max_,
                blocks_per_physical
            );

            DATA.stack_info_.physical_blocks_size = math::nf_mul(
                DATA.physical_block_size_,
                physical_block_count
            );
        }
        else {
            // 跨越模式：连续存储
            DATA.stack_info_.physical_blocks_size = math::nf_mul(
                DATA.logical_block_count_max_,
                DATA.stack_info_.actually_logical_block_size_
            );
        }
    }

	void LDS::get(
		size_t             load_begin_in,
		bmms_f::agent_ptr& data_agent_ptr_out,
		mes::a_mes& mes_out
	) {
		//1 重置消息
		__RSM__;
		//2 校验指针
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (DATA.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}
		//3 读取 pair
		byte_1 pair = 0;
        this_ptr->DRIVE_.head_control_.pair(
            logical_volume::DRIVE::head_control::op::get,
            pair,
            mes_out
        );
		__CAR__;
		//3 申请缓存
		unique_ptr<byte_1[]> cache     = nullptr;
		size_t               load_size = 0;
		if (pair != 0) {
			load_size = math::nf_mul(math::nf_mul(8,pair), data_agent_ptr_out.size);
		}
		else {
			load_size = math::nf_mul(data_agent_ptr_out.size, 4);
		}
		try {
			cache = make_unique<byte_1[]>(load_size);
		}
		catch (...) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
			return;
		}
		bmms_f::agent_ptr cache_ap = move(
			bmms_f::agent_ptr(cache.get(), load_size)
		);
		cache_ap.clear();
		//4 读取数据
		DATA.bin_file_ptr_->load_to_agent_ptr(
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
		byte_1 tmp_res    = 0;
		byte_1 a_byte     = 0;
		byte_1 begin      = 0;
		byte_1 half_byte  = 0;
		byte_1 old_byte   = 0;


		if (pair == 0) {
			for (size_t i = 0; i < load_size; i++) {
				cache_ap.load_to_void_ptr(
					i,
					1,
					&a_byte,
					true
				);
				for (size_t j = 0; j < 2; j++) {
					if (j == 0) {
                        half_byte = a_byte & 0x0F;
						if (half_byte != 0x0F && half_byte != 0x00) {
							mes_out = lfmms_m::err::service_err_logical_volume_stack_control_bad_get();
							return;
						}
					}
					else {
                        half_byte = a_byte & 0xF0;
						if (half_byte != 0xF0 && half_byte != 0x00) {
							mes_out = lfmms_m::err::service_err_logical_volume_stack_control_bad_get();
							return;
						}
					}
					byte_index = math::nf_div(math::nf_add(math::nf_mul(i, 2), j), 8);// 当前处理的是第几个输出字节
					bit_offset = math::nf_mod(math::nf_add(math::nf_mul(i, 2), j), 8);// 在当前输出字节中的第几位 (0-7)

					if (half_byte == 0x0F || half_byte == 0xF0) {
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
			}
		}
		else {
			for (size_t i = 0; i < load_size; i++) {
				cache_ap.load_to_void_ptr(
					i,
					1,
					&a_byte,
					true
				);
				// 因为 2 * pair * 4 是bit的扩展量，所以1bit会被扩展为 pair byte
				old_byte &= a_byte;
				if (a_byte != 0x00 && a_byte != 0xFF) {
					mes_out = lfmms_m::err::service_err_logical_volume_stack_control_bad_get();
					return;
				}
				if (math::nf_mod(i, pair) == 0 && i != 0) {
					byte_index = math::nf_div(math::nf_div(i, pair),8);// 当前处理的是第几个输出字节
					bit_offset = math::nf_mod(math::nf_div(i, pair),8);// 在当前输出字节中的第几位 (0-7)

					if (old_byte == 0xFF) {
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
			}
            old_byte = 0;
		}
	};

	void LDS::set(
		size_t             store_begin_in,
		bmms_f::agent_ptr& data_agent_ptr_in,
		mes::a_mes& mes_out
	) {
		//1 重置消息
		__RSM__;

		//2 校验指针
		if (this_ptr == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
		}
		if (DATA.bin_file_ptr_ == nullptr) {
			__SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
		}

		//3 读取 pair
        byte_1 pair = 0;
        this_ptr->DRIVE_.head_control_.pair(
            logical_volume::DRIVE::head_control::op::get,
            pair,
            mes_out
        );
        __CAR__;

		//4 计算存储大小并申请缓存
		size_t raw_size = data_agent_ptr_in.size;
		size_t store_size = 0;
		size_t factor = 0;  // 每个原始字节对应的存储字节数

		if (pair != 0) {
			store_size = math::nf_mul(math::nf_mul(8, pair), raw_size);
			factor = math::nf_mul(8, pair);
		}
		else {
			store_size = math::nf_mul(raw_size, 4);
			factor = 4;
		}

		// 申请双倍缓存用于写入和回读验证
		unique_ptr<byte_1[]> cache = nullptr;
		try {
			cache = make_unique<byte_1[]>(math::nf_mul(store_size, 2));
		}
		catch (...) {
			mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
			return;
		}

		bmms_f::agent_ptr write_ap = move(bmms_f::agent_ptr(cache.get(), store_size));
		bmms_f::agent_ptr read_ap = move(bmms_f::agent_ptr(
			reinterpret_cast<void*>(
				math::nf_add(
					reinterpret_cast<uintptr_t>(cache.get()),
					store_size
				)
				),
			store_size
		));
		write_ap.clear();
		read_ap.clear();

		//5 编码数据
		size_t raw_bits = math::nf_mul(raw_size, 8);
		size_t byte_index = 0;
		size_t bit_offset = 0;
		byte_1 a_byte = 0;

		if (pair == 0) {
			// pair=0: 每原始位扩展为4bit，每字节存储两个原始位
			for (size_t i = 0; i < raw_bits; i++) {
				// 读取原始位
				size_t raw_byte = math::nf_div(i, 8);
				size_t raw_bit = math::nf_mod(i, 8);
				data_agent_ptr_in.load_to_void_ptr(raw_byte, 1, &a_byte, true);
				uint8_t bit_val = (a_byte >> raw_bit) & 1;

				// 计算存储位置
				size_t store_bit_pos = math::nf_mul(i, 4);  // 存储中的位偏移
				size_t store_byte = math::nf_div(store_bit_pos, 8);
				size_t store_bit_in_byte = math::nf_mod(store_bit_pos, 8);

				// 读取当前存储字节
				byte_1 store_byte_val = 0;
				write_ap.load_to_void_ptr(store_byte, 1, &store_byte_val, true);

				// 写入4bit编码 (0x0F 表示1, 0x00 表示0)
				uint8_t nibble = bit_val ? 0x0F : 0x00;
				store_byte_val &= ~(0x0F << store_bit_in_byte);  // 清除目标4位
				store_byte_val |= (nibble << store_bit_in_byte); // 设置新值

				write_ap.store_from_void_ptr(&store_byte_val, 1, store_byte, true);
			}
		}
		else {
			// pair>0: 每原始位扩展为 8*pair 字节，全0或全FF
			size_t bytes_per_bit = math::nf_mul(8, pair);
			byte_1 ff = 0xFF;
			byte_1 zero = 0x00;

			for (size_t i = 0; i < raw_bits; i++) {
				// 读取原始位
				size_t raw_byte = math::nf_div(i, 8);
				size_t raw_bit = math::nf_mod(i, 8);
				data_agent_ptr_in.load_to_void_ptr(raw_byte, 1, &a_byte, true);
				uint8_t bit_val = (a_byte >> raw_bit) & 1;

				// 计算存储起始字节
				size_t start_byte = math::nf_mul(i, bytes_per_bit);

				// 写入连续 bytes_per_bit 个字节
				for (size_t j = 0; j < bytes_per_bit; j++) {
					if (bit_val) {
						write_ap.store_from_void_ptr(&ff, 1, math::nf_add(start_byte, j), true);
					}
					else {
						write_ap.store_from_void_ptr(&zero, 1, math::nf_add(start_byte, j), true);
					}
				}
			}
		}

        //6 带重试的写入
        const int max_retry = 2;
        int attempt = 0;
        bool write_success = false;

        while (!write_success && attempt < max_retry) {
            __RSM__;

            // 写入数据
            DATA.bin_file_ptr_->store_from_agent_ptr(
                write_ap,
                0,
                store_size,
                store_begin_in,
                true,
                false,
                mes_out
            );

			if (mes_out.code != 0) {
				if (attempt == 0) {
					attempt++;
					continue;
				}
				else {
					mes_out = lfmms_m::err::service_err_logical_volume_stack_control_bad_set();
					return;
				}
			}

			//6.2 刷新到磁盘
			DATA.bin_file_ptr_->flush();

			//6.3 回读验证
			DATA.bin_file_ptr_->load_to_agent_ptr(
				store_begin_in,
				store_size,
				read_ap,
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

			//6.4 逐字节比较
			bool data_ok = true;
			byte_1 write_val = 0;
			byte_1 read_val = 0;

			for (size_t i = 0; i < store_size; i = math::nf_add(i, 1)) {
				write_ap.load_to_void_ptr(i, 1, &write_val, true);
				read_ap.load_to_void_ptr(i, 1, &read_val, true);

				if (write_val != read_val) {
					data_ok = false;
					break;
				}
			}

			if (data_ok) {
				write_success = true;
				break;
			}
			else {
				if (attempt == 0) {
					attempt++;
					continue;
				}
				else {
					mes_out = lfmms_m::err::service_err_logical_volume_stack_control_bad_set();
					return;
				}
			}
		}

		if (!write_success) {
			mes_out = lfmms_m::err::service_err_logical_volume_stack_control_bad_set();
			return;
		}
	}

    void LDS::fix_by_pair_ECC(
        mes::a_mes& mes_out
    ) {
        __RSM__;

        //1 校验指针
        if (this_ptr == nullptr) {
            __SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
        }
        if (DATA.bin_file_ptr_ == nullptr) {
            __SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
        }

        //2 获取pair
        byte_1 pair = 0;
        this_ptr->DRIVE_.head_control_.pair(
            logical_volume::DRIVE::head_control::op::get,
            pair,
            mes_out
        );
        __CAR__;

        //3 检查pair有效性
        if (pair == 0) {
            mes_out = lfmms_m::err::service_err_logical_volume_stack_control_pair_zero_use_fix_4bit();
            return;
        }

        //4 获取栈区原始大小（需要从头部读取logical_block_count_max）
        size_t logical_block_count_max = 0;
        this_ptr->DRIVE_.head_control_.logical_block_count_max(
            logical_volume::DRIVE::head_control::op::get,
            logical_block_count_max,
            mes_out
        );
        __CAR__;

        //5 计算栈区总原始字节数
        // 每个逻辑块对应一个栈条目，每个栈条目包含handle_version(32)+can_pop(4)+index(28)=64字节原始数据
        size_t raw_size = math::nf_mul(logical_block_count_max, 64);

        //6 计算存储大小（每个原始字节扩展为8*pair字节）
        size_t bytes_per_raw_byte = math::nf_mul(8, pair);
        size_t store_size = math::nf_mul(raw_size, bytes_per_raw_byte);

        //7 计算栈区在文件中的起始偏移
        // head_end = 4096 + 4096*(2*pair-1)
        size_t head_end = math::nf_add(
            4096,
            math::nf_mul(4096, math::nf_sub(math::nf_mul(2, pair), 1))
        );

        // 栈顶偏移区大小 = 32 + 32*(2*pair-1) 字节，有两个副本 + 1字节flag
        size_t stack_top_size = math::nf_add(
            32,
            math::nf_mul(32, math::nf_sub(math::nf_mul(2, pair), 1))
        );
        size_t stack_top_region_size = math::nf_add(
            math::nf_mul(stack_top_size, 2),
            1
        );

        size_t file_offset = math::nf_add(head_end, stack_top_region_size);

        //8 申请缓存
        unique_ptr<byte_1[]> data = nullptr;
        unique_ptr<size_t[]> bit_counts = nullptr;

        try {
            data = make_unique<byte_1[]>(store_size);
            bit_counts = make_unique<size_t[]>(math::nf_mul(bytes_per_raw_byte, 2));
        }
        catch (...) {
            mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
            return;
        }

        bmms_f::agent_ptr store_ap = move(bmms_f::agent_ptr(data.get(), store_size));
        store_ap.clear();

        //9 读取数据
        DATA.bin_file_ptr_->load_to_agent_ptr(
            file_offset,
            store_size,
            store_ap,
            0,
            true,
            mes_out
        );
        __CAR__;

        //10 投票修复
        size_t total_raw_bytes = raw_size;

        for (size_t byte_idx = 0; byte_idx < total_raw_bytes; byte_idx++) {
            size_t start_byte = math::nf_mul(byte_idx, bytes_per_raw_byte);

            // 重置计数
            for (size_t k = 0; k < bytes_per_raw_byte; k++) {
                bit_counts[k] = 0;
                bit_counts[math::nf_add(k, bytes_per_raw_byte)] = 0;
            }

            // 统计每个字节的位
            for (size_t j = 0; j < bytes_per_raw_byte; j++) {
                byte_1 b;
                store_ap.load_to_void_ptr(
                    math::nf_add(start_byte, j),
                    1,
                    &b,
                    true
                );

                // 检查数据有效性
                if (b != 0x00 && b != 0xFF) {
                    mes_out = lfmms_m::err::service_err_logical_volume_stack_control_bad_data();
                    return;
                }

                // 统计这个字节的8个位
                for (size_t bit = 0; bit < 8; bit++) {
                    if (b & (1 << bit)) {
                        bit_counts[math::nf_add(j, bytes_per_raw_byte)]++;
                    }
                    else {
                        bit_counts[j]++;
                    }
                }
            }

            // 聚合统计并修复
            size_t total_zero = 0;
            size_t total_one = 0;

            for (size_t j = 0; j < bytes_per_raw_byte; j++) {
                total_zero = math::nf_add(total_zero, bit_counts[j]);
                total_one = math::nf_add(total_one, bit_counts[math::nf_add(j, bytes_per_raw_byte)]);
            }

            // 检查平局
            if (total_zero == total_one) {
                mes_out = lfmms_m::err::service_err_logical_volume_stack_control_fix_tie();
                return;
            }

            // 确定修复值
            byte_1 fix_byte = (total_one > total_zero) ? 0xFF : 0x00;

            // 修复整个组
            for (size_t j = 0; j < bytes_per_raw_byte; j++) {
                store_ap.store_from_void_ptr(
                    &fix_byte,
                    1,
                    math::nf_add(start_byte, j),
                    true
                );
            }
        }

        //11 写回数据
        DATA.bin_file_ptr_->store_from_agent_ptr(
            store_ap,
            0,
            store_size,
            file_offset,
            true,
            true,
            mes_out
        );
        __CAR__;
        DATA.bin_file_ptr_->flush();
    }

    void LDS::fix_by_4bitECC(
        mes::a_mes& mes_out
    ) {
        __RSM__;

        //1 校验指针
        if (this_ptr == nullptr) {
            __SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
        }
        if (DATA.bin_file_ptr_ == nullptr) {
            __SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
        }

        //2 获取pair
        byte_1 pair = 0;
        this_ptr->DRIVE_.head_control_.pair(
            logical_volume::DRIVE::head_control::op::get,
            pair,
            mes_out
        );
        __CAR__;

        //3 检查pair有效性
        if (pair != 0) {
            mes_out = lfmms_m::err::service_err_logical_volume_stack_control_pair_nonzero_use_fix_pair();
            return;
        }

        //4 获取栈区原始大小
        size_t logical_block_count_max = 0;
        this_ptr->DRIVE_.head_control_.logical_block_count_max(
            logical_volume::DRIVE::head_control::op::get,
            logical_block_count_max,
            mes_out
        );
        __CAR__;

        //5 计算栈区总原始字节数
        size_t raw_size = math::nf_mul(logical_block_count_max, 64);

        //6 计算存储大小（每个原始字节扩展为4字节）
        size_t store_size = math::nf_mul(raw_size, 4);

        //7 计算栈区在文件中的起始偏移
        // head_end = 4096
        size_t file_offset = 4096;  // pair=0时，只有1个头，所以head_end=4096

        //8 申请缓存
        unique_ptr<byte_1[]> data = nullptr;
        unique_ptr<size_t[]> bit_counts = nullptr;

        try {
            data = make_unique<byte_1[]>(store_size);
            bit_counts = make_unique<size_t[]>(8);
        }
        catch (...) {
            mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
            return;
        }

        bmms_f::agent_ptr store_ap = move(bmms_f::agent_ptr(data.get(), store_size));
        store_ap.clear();

        //9 读取数据
        DATA.bin_file_ptr_->load_to_agent_ptr(
            file_offset,
            store_size,
            store_ap,
            0,
            true,
            mes_out
        );
        __CAR__;

        //10 投票修复 - 4bit ECC
        size_t total_raw_bits = math::nf_mul(raw_size, 8);

        for (size_t bit_idx = 0; bit_idx < total_raw_bits; bit_idx++) {
            // 计算这个位对应的存储位置
            size_t bit_pos = math::nf_mul(bit_idx, 4);  // 4bit per original bit
            size_t byte_off = math::nf_div(bit_pos, 8);
            size_t bit_in_byte = math::nf_mod(bit_pos, 8);

            // 读取包含这4bit的字节
            byte_1 b;
            store_ap.load_to_void_ptr(byte_off, 1, &b, true);

            // 提取4bit并统计1的个数
            uint8_t nibble = (b >> bit_in_byte) & 0x0F;

            // 重置计数
            for (size_t k = 0; k < 8; k++) {
                bit_counts[k] = 0;
            }

            // 统计这4bit中每个位
            for (size_t bit = 0; bit < 4; bit++) {
                if (nibble & (1 << bit)) {
                    bit_counts[math::nf_add(bit, 4)]++;
                }
                else {
                    bit_counts[bit]++;
                }
            }

            // 聚合统计
            size_t total_zero = 0;
            size_t total_one = 0;

            for (size_t bit = 0; bit < 4; bit++) {
                total_zero = math::nf_add(total_zero, bit_counts[bit]);
                total_one = math::nf_add(total_one, bit_counts[math::nf_add(bit, 4)]);
            }

            // 检查平局
            if (total_zero == total_one) {
                mes_out = lfmms_m::err::service_err_logical_volume_stack_control_fix_tie();
                return;
            }

            // 确定修复的4bit值
            uint8_t fix_nibble = (total_one > total_zero) ? 0x0F : 0x00;

            // 更新字节
            b &= ~(0x0F << bit_in_byte);  // 清除目标4位
            b |= (fix_nibble << bit_in_byte);  // 设置新值

            store_ap.store_from_void_ptr(&b, 1, byte_off, true);
        }

        //11 写回数据
        DATA.bin_file_ptr_->store_from_agent_ptr(
            store_ap,
            0,
            store_size,
            file_offset,
            true,
            true,
            mes_out
        );
        __CAR__;
        DATA.bin_file_ptr_->flush();
    }

    void LDS::chk_by_pair(
        bool& chk_res_out,
        mes::a_mes& mes_out
    ) {
        __RSM__;
        chk_res_out = false;

        //1 校验指针
        if (this_ptr == nullptr) {
            __SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
        }
        if (DATA.bin_file_ptr_ == nullptr) {
            __SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
        }

        //2 获取pair
        byte_1 pair = 0;
        this_ptr->DRIVE_.head_control_.pair(
            logical_volume::DRIVE::head_control::op::get,
            pair,
            mes_out
        );
        __CAR__;

        //3 获取栈区原始大小
        size_t logical_block_count_max = 0;
        this_ptr->DRIVE_.head_control_.logical_block_count_max(
            logical_volume::DRIVE::head_control::op::get,
            logical_block_count_max,
            mes_out
        );
        __CAR__;

        //4 计算相关大小
        size_t raw_size = math::nf_mul(logical_block_count_max, 64);
        size_t store_size;
        size_t file_offset;

        if (pair != 0) {
            // pair>0的情况
            size_t bytes_per_raw_byte = math::nf_mul(8, pair);
            store_size = math::nf_mul(raw_size, bytes_per_raw_byte);

            // head_end = 4096 + 4096*(2*pair-1)
            size_t head_end = math::nf_add(
                4096,
                math::nf_mul(4096, math::nf_sub(math::nf_mul(2, pair), 1))
            );

            // 栈顶偏移区大小
            size_t stack_top_size = math::nf_add(
                32,
                math::nf_mul(32, math::nf_sub(math::nf_mul(2, pair), 1))
            );
            size_t stack_top_region_size = math::nf_add(
                math::nf_mul(stack_top_size, 2),
                1
            );

            file_offset = math::nf_add(head_end, stack_top_region_size);
        }
        else {
            // pair=0的情况
            store_size = math::nf_mul(raw_size, 4);
            file_offset = 4096;  // 只有1个头
        }

        //5 申请缓存
        unique_ptr<byte_1[]> data = nullptr;
        try {
            data = make_unique<byte_1[]>(store_size);
        }
        catch (...) {
            mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
            return;
        }

        bmms_f::agent_ptr store_ap = move(bmms_f::agent_ptr(data.get(), store_size));
        store_ap.clear();

        //6 读取数据
        DATA.bin_file_ptr_->load_to_agent_ptr(
            file_offset,
            store_size,
            store_ap,
            0,
            true,
            mes_out
        );
        __CAR__;

        //7 检查数据一致性
        if (pair == 0) {
            // pair=0: 检查4bit组的一致性
            size_t total_raw_bits = math::nf_mul(raw_size, 8);

            for (size_t bit_idx = 0; bit_idx < total_raw_bits; bit_idx++) {
                size_t bit_pos = math::nf_mul(bit_idx, 4);
                size_t byte_off = math::nf_div(bit_pos, 8);
                size_t bit_in_byte = math::nf_mod(bit_pos, 8);

                byte_1 b;
                store_ap.load_to_void_ptr(byte_off, 1, &b, true);

                uint8_t nibble = (b >> bit_in_byte) & 0x0F;

                // 检查4bit是否全0或全1
                if (nibble != 0x00 && nibble != 0x0F) {
                    chk_res_out = false;
                    return;
                }
            }
        }
        else {
            // pair>0: 检查组内字节是否一致
            size_t bytes_per_raw_byte = math::nf_mul(8, pair);
            size_t total_raw_bytes = raw_size;

            for (size_t byte_idx = 0; byte_idx < total_raw_bytes; byte_idx++) {
                size_t start_byte = math::nf_mul(byte_idx, bytes_per_raw_byte);

                // 读取第一个字节作为基准
                byte_1 base;
                store_ap.load_to_void_ptr(start_byte, 1, &base, true);

                // 检查基准是否有效
                if (base != 0x00 && base != 0xFF) {
                    chk_res_out = false;
                    return;
                }

                // 检查组内其他字节
                for (size_t j = 1; j < bytes_per_raw_byte; j++) {
                    byte_1 b;
                    store_ap.load_to_void_ptr(
                        math::nf_add(start_byte, j),
                        1,
                        &b,
                        true
                    );

                    if (b != base) {
                        chk_res_out = false;
                        return;
                    }
                }
            }
        }

        chk_res_out = true;
    }

    void LDS::init_stack_space(
        mes::a_mes& mes_out
    ) {
        // 1. 重置消息
        __RSM__;

        // 2. 校验指针
        if (this_ptr == nullptr) {
            __SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_this_ptr_null)
        }
        if (DATA.bin_file_ptr_ == nullptr) {
            __SYS_ERROR__(lfmms_m::err::sys_err_logical_volume_head_control_bin_file_ptr_null)
        }

        // 3. 获取必要参数
        byte_1 pair = DATA.pair_;
        bool using_group = DATA.stack_info_.using_group;
        bool using_range = DATA.stack_info_.using_range;
        size_t logical_block_count_max = DATA.logical_block_count_max_;
        size_t physical_block_size = DATA.physical_block_size_;
        size_t logical_block_size = DATA.logical_block_size_;
        size_t actually_logical_block_size = DATA.stack_info_.actually_logical_block_size_;
        size_t blocks_per_physical = math::nf_div(physical_block_size, logical_block_size);
        size_t stack_group_count = DATA.stack_info_.stack_group_count;
        size_t stack_group_size = DATA.stack_info_.stack_group_size; 
        size_t raw_stack_size = DATA.stack_info_.logical_block_stack_size; 

        // 4. 初始化栈顶偏移区（两个副本 + flag）
        size_t top0_off = DATA.stack_info_.logical_block_stack_top_offset_0_offset;
        size_t top1_off = DATA.stack_info_.logical_block_stack_top_offset_1_offset;
        size_t flag_off = DATA.stack_info_.logical_block_stack_top_offset_flag_offset;

        // 每个副本的原始大小（未编码）
        size_t top_copy_raw_size = 32 + 32 * (2 * pair - 1);
        // 但存储时已编码，直接用 clear_range 清零（0编码为全0）
        DATA.bin_file_ptr_->clear_range(top0_off, top_copy_raw_size, mes_out);
        __CAR__;
        DATA.bin_file_ptr_->clear_range(top1_off, top_copy_raw_size, mes_out);
        __CAR__;

        // 写 flag 为 0（使用8字节写入）
        byte_1 flag_zero[8] = { 0 };
        DATA.bin_file_ptr_->store_from_void_ptr(flag_zero, 8, flag_off, true, mes_out);
        __CAR__;
        DATA.bin_file_ptr_->flush();

        // 5. 构造栈区原始数据缓冲区
        auto raw_buf = make_unique<byte_1[]>(raw_stack_size);
        if (!raw_buf) {
            mes_out = lfmms_m::err::service_err_logical_volume_head_control_memory_alloc_failed();
            return;
        }
        bmms_f::agent_ptr raw_ap(raw_buf.get(), raw_stack_size);
        raw_ap.clear(); // 全零初始化

        // 各字段原始大小（单位：字节）
        const size_t RAW_HANDLE_VERSION_SIZE = 8;   // handle_version（64位）
        const size_t RAW_CAN_POP_SIZE = 1;          // can_pop（1字节）
        const size_t RAW_LOGICAL_INDEX_SIZE = 7;    // logical_block_index（7字节，小端）
        const size_t RAW_PHYS_OFFSET_SIZE = 8;       // physical_block_offset（64位，组式用）
        const size_t RAW_LOGICAL_OFFSET_SIZE = 8;    // logical_block_offset（64位，区间式用）

        if (using_group) {
            // 组式布局
            size_t a_stack_info_size = RAW_HANDLE_VERSION_SIZE + RAW_CAN_POP_SIZE + RAW_LOGICAL_INDEX_SIZE; // 16
            for (size_t g = 0; g < stack_group_count; ++g) {
                size_t group_start = g * stack_group_size;
                // 当前组的物理块在文件中的偏移（相对物理块区域起始）
                size_t phys_block_offset = g * physical_block_size;
                raw_ap.store_from_void_ptr(&phys_block_offset, RAW_PHYS_OFFSET_SIZE, group_start, true);

                for (size_t b = 0; b < blocks_per_physical; ++b) {
                    size_t logical_index = g * blocks_per_physical + b;
                    if (logical_index >= logical_block_count_max) break; // 最后一个组可能不满

                    size_t entry_start = group_start + RAW_PHYS_OFFSET_SIZE + b * a_stack_info_size;

                    // handle_version = 1
                    size_t version = 1;
                    raw_ap.store_from_void_ptr(&version, RAW_HANDLE_VERSION_SIZE, entry_start, true);

                    // can_pop = 1
                    byte_1 can = 1;
                    raw_ap.store_from_void_ptr(&can, RAW_CAN_POP_SIZE, entry_start + RAW_HANDLE_VERSION_SIZE, true);

                    // logical_block_index (7字节小端)
                    byte_1 index_buf[RAW_LOGICAL_INDEX_SIZE] = { 0 };
                    for (size_t k = 0; k < RAW_LOGICAL_INDEX_SIZE; ++k) {
                        index_buf[k] = (logical_index >> (k * 8)) & 0xFF;
                    }
                    raw_ap.store_from_void_ptr(index_buf, RAW_LOGICAL_INDEX_SIZE,
                        entry_start + RAW_HANDLE_VERSION_SIZE + RAW_CAN_POP_SIZE, true);
                }
            }
        }
        else if (using_range) {
            // 区间式布局
            size_t entry_size = RAW_LOGICAL_OFFSET_SIZE + RAW_HANDLE_VERSION_SIZE + RAW_CAN_POP_SIZE + RAW_LOGICAL_INDEX_SIZE; // 24
            for (size_t i = 0; i < logical_block_count_max; ++i) {
                size_t entry_start = i * entry_size;

                // logical_block_offset = i * actually_logical_block_size
                size_t logical_offset = i * actually_logical_block_size;
                raw_ap.store_from_void_ptr(&logical_offset, RAW_LOGICAL_OFFSET_SIZE, entry_start, true);

                // handle_version = 1
                size_t version = 1;
                raw_ap.store_from_void_ptr(&version, RAW_HANDLE_VERSION_SIZE,
                    entry_start + RAW_LOGICAL_OFFSET_SIZE, true);

                // can_pop = 1
                byte_1 can = 1;
                raw_ap.store_from_void_ptr(&can, RAW_CAN_POP_SIZE,
                    entry_start + RAW_LOGICAL_OFFSET_SIZE + RAW_HANDLE_VERSION_SIZE, true);

                // logical_block_index = i (7字节小端)
                byte_1 index_buf[RAW_LOGICAL_INDEX_SIZE] = { 0 };
                for (size_t k = 0; k < RAW_LOGICAL_INDEX_SIZE; ++k) {
                    index_buf[k] = (i >> (k * 8)) & 0xFF;
                }
                raw_ap.store_from_void_ptr(index_buf, RAW_LOGICAL_INDEX_SIZE,
                    entry_start + RAW_LOGICAL_OFFSET_SIZE + RAW_HANDLE_VERSION_SIZE + RAW_CAN_POP_SIZE, true);
            }
        }
        else {
            // 不应该发生
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_stack_control_invalid_value_pass)
        }

        // 6. 通过 stack_control 将原始数据写入栈区（使用全局偏移）
        set(
            DATA.stack_info_.logical_block_stack_offset, // 全局文件偏移
            raw_ap,
            mes_out
        );
        __CAR__;

        // 7. 刷新确保写入
        DATA.bin_file_ptr_->flush();

        // 8. 物理块区域无需额外初始化，因为 BOOT resize 时已清零
    }

    void LDS::get_group_info_for_pop(
        size_t index_in,
        size_t& physical_block_offset_out,
        size_t& handle_version_out,
        size_t& handle_version_offset_out,
        bool& can_pop_out,
        size_t& can_pop_offset_out,
        size_t& logical_block_begin_offset_out,
        mes::a_mes& mes_out
    ) {
        __RSM__;

        // 1. 校验索引范围
        if (index_in >= this_ptr->data_.logical_block_count_max_) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_stack_control_invalid_index)
        }

        // 2. 获取基本参数
        byte_1 pair = this_ptr->data_.pair_;
        bool using_group = this_ptr->data_.stack_info_.using_group;
        size_t logical_block_size = this_ptr->data_.logical_block_size_;
        size_t physical_block_size = this_ptr->data_.physical_block_size_;
        size_t actually_logical_block_size = this_ptr->data_.stack_info_.actually_logical_block_size_;
        size_t blocks_per_physical = math::nf_div(physical_block_size, logical_block_size);
        size_t stack_start = this_ptr->data_.stack_info_.logical_block_stack_offset;
        size_t physical_blocks_start = this_ptr->data_.stack_info_.physical_blocks_offset;

        // 3. 计算扩展因子
        size_t factor = (pair == 0) ? 4 : math::nf_mul(8, pair);

        // 4. 计算编码后各字段大小
        size_t enc_handle_version_size = math::nf_mul(8, factor);      // 原始8字节
        size_t enc_can_pop_size        = math::nf_mul(1, factor);      // 原始1字节
        size_t enc_logical_index_size  = math::nf_mul(7, factor);      // 原始7字节
        size_t enc_phys_offset_size    = math::nf_mul(8, factor);      // 原始8字节（组式用）
        size_t enc_logical_offset_size = math::nf_mul(8, factor);      // 原始8字节（区间式用）

        if (using_group) {
            // 组式布局
            size_t group_index = math::nf_div(index_in, blocks_per_physical);
            size_t block_in_group = math::nf_mod(index_in, blocks_per_physical);

            // 组起始偏移（实际存储大小 = 原始组大小 * factor）
            size_t raw_group_size = this_ptr->data_.stack_info_.stack_group_size; // 原始大小
            size_t actual_group_size = math::nf_mul(raw_group_size, factor);
            size_t group_start = math::nf_add(stack_start,
                math::nf_mul(group_index, actual_group_size));

            // physical_block_offset 字段在组开头
            physical_block_offset_out = group_start;

            // a_stack_info 连续排列，每个的大小
            size_t a_stack_info_size = enc_handle_version_size + enc_can_pop_size + enc_logical_index_size;
            size_t info_start = math::nf_add(group_start + enc_phys_offset_size,
                math::nf_mul(block_in_group, a_stack_info_size));

            handle_version_offset_out = info_start;
            can_pop_offset_out = math::nf_add(info_start, enc_handle_version_size);

            // 读取 physical_block_offset 的原始值（相对于物理块区域起始的偏移）
            size_t phys_block_rel = 0;
            bmms_f::agent_ptr phys_ap(&phys_block_rel, sizeof(size_t));
            get(physical_block_offset_out, phys_ap, mes_out);
            __CAR__;

            // 计算逻辑块数据起始偏移
            logical_block_begin_offset_out = math::nf_add(
                physical_blocks_start,
                math::nf_add(phys_block_rel,
                    math::nf_mul(block_in_group, actually_logical_block_size))
            );

            // 读取版本号
            bmms_f::agent_ptr ver_ap(&handle_version_out, sizeof(size_t));
            get(handle_version_offset_out, ver_ap, mes_out);
            __CAR__;

            // 读取 can_pop（原始值1字节）
            byte_1 can = 0;
            bmms_f::agent_ptr can_ap(&can, 1);
            get(can_pop_offset_out, can_ap, mes_out);
            __CAR__;
            can_pop_out = (can != 0);
        }
        else {
            // 区间式布局
            size_t entry_size = enc_logical_offset_size + enc_handle_version_size +
                enc_can_pop_size + enc_logical_index_size;
            size_t entry_start = math::nf_add(stack_start,
                math::nf_mul(index_in, entry_size));

            physical_block_offset_out = 0; // 组式专用，置0

            // logical_block_offset 在条目开头
            size_t logical_offset_off = entry_start;
            handle_version_offset_out = math::nf_add(entry_start, enc_logical_offset_size);
            can_pop_offset_out = math::nf_add(handle_version_offset_out, enc_handle_version_size);

            // 读取 logical_block_offset 的原始值
            size_t logical_offset = 0;
            bmms_f::agent_ptr logical_ap(&logical_offset, sizeof(size_t));
            get(logical_offset_off, logical_ap, mes_out);
            __CAR__;

            // 计算逻辑块数据起始偏移
            logical_block_begin_offset_out = math::nf_add(physical_blocks_start, logical_offset);

            // 读取版本号
            bmms_f::agent_ptr ver_ap(&handle_version_out, sizeof(size_t));
            get(handle_version_offset_out, ver_ap, mes_out);
            __CAR__;

            // 读取 can_pop
            byte_1 can = 0;
            bmms_f::agent_ptr can_ap(&can, 1);
            get(can_pop_offset_out, can_ap, mes_out);
            __CAR__;
            can_pop_out = (can != 0);
        }
    }
    void LDS::get_group_info_for_push_with_check(
        size_t& index_in,
        size_t& version_in,
        size_t& physical_block_offset_out,
        size_t& handle_version_out,
        size_t& handle_version_offset_out,
        bool&   can_pop_out,
        size_t& can_pop_offset_out,
        size_t& logical_block_begin_offset_out,
        mes::a_mes& mes_out
    ) {
        // 先调用 get_group_info_for_pop 获取所有信息
        get_group_info_for_pop(
            index_in,
            physical_block_offset_out,
            handle_version_out,
            handle_version_offset_out,
            can_pop_out,
            can_pop_offset_out,
            logical_block_begin_offset_out,
            mes_out
        );
        __CAR__; // 如果 get_group_info 出错，直接返回

        // 检查版本号是否匹配
        if (handle_version_out != version_in) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_stack_control_use_after_free);
        }

        // 检查 can_pop 是否为 false（块必须处于已分配状态才能释放）
        if (can_pop_out) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_stack_control_use_after_free);
        }
    }

    void LDS::mark_block_can_pop(
        size_t& can_pop_offset_in,
        mes::a_mes& mes_out
    ) {
        __RSM__;
        byte_1 can_pop_value = 1;  // 原始值为 1 表示空闲
        bmms_f::agent_ptr ap(&can_pop_value, 1);
        set(can_pop_offset_in, ap, mes_out);  // set 会自动处理位扩展
    }

    void LDS::mark_block_can_not_pop(
        size_t& can_pop_offset_in,
        mes::a_mes& mes_out
    ) {
        __RSM__;
        byte_1 can_pop_value = 0;  // 原始值为 0 表示已分配
        bmms_f::agent_ptr ap(&can_pop_value, 1);
        set(can_pop_offset_in, ap, mes_out);
    }

    void LDS::increment_version(
        size_t& handle_version_offset_in,
        mes::a_mes& mes_out
    ) {
        __RSM__;
        // 1. 读取当前版本号（原始值）
        size_t current_version = 0;
        bmms_f::agent_ptr read_ap(&current_version, sizeof(size_t));
        get(handle_version_offset_in, read_ap, mes_out);
        __CAR__;

        // 3. 版本号加1
        current_version = math::nf_add(current_version, 1);

        // 4. 写回
        bmms_f::agent_ptr write_ap(&current_version, sizeof(size_t));
        set(handle_version_offset_in, write_ap, mes_out);
    }

    void LDS::CAS_stack_offset_atomic_for_push(
        size_t& index_in,
        mes::a_mes& mes_out
    ) {
        __RSM__;

        // 1. 获取当前使用的副本和栈指针值
        byte_8 flag_val = 0;
        size_t stack_top = 0;
        size_t top0_off = this_ptr->data_.stack_info_.logical_block_stack_top_offset_0_offset;
        size_t top1_off = this_ptr->data_.stack_info_.logical_block_stack_top_offset_1_offset;
        size_t flag_off = this_ptr->data_.stack_info_.logical_block_stack_top_offset_flag_offset;

        // 读取 flag
        bmms_f::agent_ptr flag_ap(&flag_val, 8);
        get(flag_off, flag_ap, mes_out);
        __CAR__;

        // 2. 确定当前使用的副本偏移
        size_t current_top_off = (flag_val == 0) ? top0_off : top1_off;
        size_t backup_top_off = (flag_val == 0) ? top1_off : top0_off;

        // 3. 读取当前栈指针值
        bmms_f::agent_ptr top_ap(&stack_top, sizeof(size_t));
        get(current_top_off, top_ap, mes_out);
        __CAR__;

        // 4. push 操作：栈指针应该向低地址移动（减一个条目大小）
        size_t entry_size = 0;
        if (this_ptr->data_.stack_info_.using_group) {
            // 组式：每个条目大小 = enc_handle_version + enc_can_pop + enc_logical_index
            size_t factor = (this_ptr->data_.pair_ == 0) ? 4 : math::nf_mul(8, this_ptr->data_.pair_);
            entry_size = math::nf_mul(8 + 1 + 7, factor); // 原始16字节 * factor
        }
        else {
            // 区间式：每个条目大小 = enc_logical_offset + enc_handle_version + enc_can_pop + enc_logical_index
            size_t factor = (this_ptr->data_.pair_ == 0) ? 4 : math::nf_mul(8, this_ptr->data_.pair_);
            entry_size = math::nf_mul(8 + 8 + 1 + 7, factor); // 原始24字节 * factor
        }

        size_t new_stack_top = math::nf_sub(stack_top, entry_size);

        // 5. 原子操作：先写备份副本，再切换 flag
        // 5.1 写备份副本
        bmms_f::agent_ptr new_top_ap(&new_stack_top, sizeof(size_t));
        set(backup_top_off, new_top_ap, mes_out);
        __CAR__;

        // 5.2 切换 flag（0→FF 或 FF→0）
        byte_8 new_flag = (flag_val == 0) ? 0xFFFFFFFFFFFFFFFF : 0;
        bmms_f::agent_ptr new_flag_ap(&new_flag, 8);
        set(flag_off, new_flag_ap, mes_out);
        __CAR__;

        // 5.3 验证 flag 切换成功
        byte_8 verify_flag = 0;
        bmms_f::agent_ptr verify_ap(&verify_flag, 8);
        get(flag_off, verify_ap, mes_out);
        __CAR__;

        if (verify_flag != new_flag) {
            mes_out = lfmms_m::err::service_err_logical_volume_stack_control_cas_failed();
            return;
        }

        // 6. 返回的 index_in 就是逻辑块索引，不需要修改
        // 但可以在这里添加验证
        if (index_in >= this_ptr->data_.logical_block_count_max_) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_stack_control_invalid_index);
        }
    }

    void LDS::CAS_stack_offset_atomic_for_pop(
        size_t& index_out,
        mes::a_mes& mes_out
    ) {
        __RSM__;

        // 1. 获取当前使用的副本和栈指针值
        byte_8 flag_val = 0;
        size_t stack_top = 0;
        size_t top0_off = this_ptr->data_.stack_info_.logical_block_stack_top_offset_0_offset;
        size_t top1_off = this_ptr->data_.stack_info_.logical_block_stack_top_offset_1_offset;
        size_t flag_off = this_ptr->data_.stack_info_.logical_block_stack_top_offset_flag_offset;

        // 读取 flag
        bmms_f::agent_ptr flag_ap(&flag_val, 8);
        get(flag_off, flag_ap, mes_out);
        __CAR__;

        // 2. 确定当前使用的副本偏移
        size_t current_top_off = (flag_val == 0) ? top0_off : top1_off;
        size_t backup_top_off = (flag_val == 0) ? top1_off : top0_off;

        // 3. 读取当前栈指针值
        bmms_f::agent_ptr top_ap(&stack_top, sizeof(size_t));
        get(current_top_off, top_ap, mes_out);
        __CAR__;

        // 4. 检查栈是否为空（栈指针指向栈区末尾时表示空）
        size_t stack_end = math::nf_add(
            this_ptr->data_.stack_info_.logical_block_stack_offset,
            this_ptr->data_.stack_info_.logical_block_stack_size
        );
        if (stack_top >= stack_end) {
            mes_out = lfmms_m::err::service_err_logical_volume_stack_control_empty();
            return;
        }

        // 5. 计算编码后各字段大小
        size_t factor = (this_ptr->data_.pair_ == 0) ? 4 : math::nf_mul(8, this_ptr->data_.pair_);
        size_t enc_handle_version_size = math::nf_mul(8, factor);  // 原始8字节
        size_t enc_can_pop_size = math::nf_mul(1, factor);        // 原始1字节
        size_t enc_logical_index_size = math::nf_mul(7, factor);  // 原始7字节
        size_t enc_logical_offset_size = math::nf_mul(8, factor); // 原始8字节（区间式用）

        // 6. 计算条目大小
        size_t entry_size = 0;
        if (this_ptr->data_.stack_info_.using_group) {
            entry_size = enc_handle_version_size + enc_can_pop_size + enc_logical_index_size;
        }
        else {
            entry_size = enc_logical_offset_size + enc_handle_version_size +
                enc_can_pop_size + enc_logical_index_size;
        }

        // 7. pop 操作：从当前栈顶读取索引，然后栈指针向高地址移动
        // 当前栈顶指向的位置就是逻辑块索引所在的位置
        // 需要读取该位置的值得到索引

        // 7.1 读取栈顶条目中的 logical_block_index
        // 注意：栈顶指向的是条目的起始，而 logical_block_index 在条目中的偏移取决于布局
        size_t index_off = stack_top;
        if (this_ptr->data_.stack_info_.using_group) {
            // 组式：条目开头就是 handle_version，index 在最后7字节
            index_off = math::nf_add(stack_top,
                math::nf_add(enc_handle_version_size, enc_can_pop_size));
        }
        else {
            // 区间式：条目开头是 logical_block_offset，然后是 handle_version，index 在最后
            index_off = math::nf_add(stack_top,
                math::nf_add(enc_logical_offset_size,
                    math::nf_add(enc_handle_version_size, enc_can_pop_size)));
        }

        // 读取索引（原始7字节）
        byte_1 index_buf[7] = { 0 };
        bmms_f::agent_ptr index_ap(index_buf, 7);
        get(index_off, index_ap, mes_out);
        __CAR__;

        // 组装成 size_t（小端）
        size_t block_index = 0;
        for (size_t i = 0; i < 7; i++) {
            block_index |= (static_cast<size_t>(index_buf[i]) << (i * 8));
        }

        // 验证索引有效
        if (block_index >= this_ptr->data_.logical_block_count_max_) {
            __CODING_ERROR__(lfmms_m::err::coding_err_logical_volume_stack_control_invalid_index);
        }

        // 8. 计算新栈指针
        size_t new_stack_top = math::nf_add(stack_top, entry_size);

        // 9. 原子操作：先写备份副本，再切换 flag
        bmms_f::agent_ptr new_top_ap(&new_stack_top, sizeof(size_t));
        set(backup_top_off, new_top_ap, mes_out);
        __CAR__;

        byte_8 new_flag = (flag_val == 0) ? 0xFFFFFFFFFFFFFFFF : 0;
        bmms_f::agent_ptr new_flag_ap(&new_flag, 8);
        set(flag_off, new_flag_ap, mes_out);
        __CAR__;

        // 验证
        byte_8 verify_flag = 0;
        bmms_f::agent_ptr verify_ap(&verify_flag, 8);
        get(flag_off, verify_ap, mes_out);
        __CAR__;

        if (verify_flag != new_flag) {
            mes_out = lfmms_m::err::service_err_logical_volume_stack_control_cas_failed();
            return;
        }

        // 10. 返回索引
        index_out = block_index;
    }

    void LDS::push(
        size_t&     logical_block_index_in,
        size_t&     version_in,
        mes::a_mes& mes_out
    ) {
        __RSM__;

        // 1. 获取该块的信息并进行push前检查
        size_t phys_off_out = 0;
        size_t handle_ver_out = 0;
        size_t handle_ver_off_out = 0;
        bool can_pop_out = false;
        size_t can_pop_off_out = 0;
        size_t logical_begin_out = 0;

        get_group_info_for_push_with_check(
            logical_block_index_in,
            version_in,
            phys_off_out,
            handle_ver_out,
            handle_ver_off_out,
            can_pop_out,
            can_pop_off_out,
            logical_begin_out,
            mes_out
        );
        __CAR__;

        // 2. 标记该块为空闲
        mark_block_can_pop(can_pop_off_out, mes_out);
        __CAR__;

        // 3. 版本号递增
        increment_version(handle_ver_off_out, mes_out);
        __CAR__;

        // 4. 原子更新栈指针（push时栈指针下移）
        CAS_stack_offset_atomic_for_push(logical_block_index_in, mes_out);
        __CAR__;
    }

    void LDS::pop(
        size_t&     logical_block_index_out,
        size_t&     version_out,
        mes::a_mes& mes_out
    ) {
        __RSM__;

        // 1. 原子获取栈顶索引并更新栈指针
        CAS_stack_offset_atomic_for_pop(logical_block_index_out, mes_out);
        __CAR__;

        // 2. 获取该块信息
        size_t phys_off_out = 0;
        size_t handle_ver_out = 0;
        size_t handle_ver_off_out = 0;
        bool can_pop_out = false;
        size_t can_pop_off_out = 0;
        size_t logical_begin_out = 0;

        get_group_info_for_pop(
            logical_block_index_out,
            phys_off_out,
            handle_ver_out,
            handle_ver_off_out,
            can_pop_out,
            can_pop_off_out,
            logical_begin_out,
            mes_out
        );
        __CAR__;

        // 3. 检查该块确实空闲
        if (!can_pop_out) {
            __CODING_ERROR__(lfmms_m::err::sys_err_logical_volume_stack_control_block_not_free_but_stack_ptr_point_it);
        }

        // 4. 版本号作为输出
        version_out = handle_ver_out;

        // 5. 标记该块为已分配
        mark_block_can_not_pop(can_pop_off_out, mes_out);
        __CAR__;

        // 6. 版本号递增
        increment_version(handle_ver_off_out, mes_out);
        __CAR__;
    }

    size_t LDS::get_using_count(mes::a_mes& mes_out) {
        __RSM__;

        // 1. 获取当前使用的副本和栈指针值
        byte_8 flag_val = 0;
        size_t stack_top_index = 0;
        size_t top0_off = DATA.stack_info_.logical_block_stack_top_offset_0_offset;
        size_t top1_off = DATA.stack_info_.logical_block_stack_top_offset_1_offset;
        size_t flag_off = DATA.stack_info_.logical_block_stack_top_offset_flag_offset;

        // 读取 flag
        bmms_f::agent_ptr flag_ap(&flag_val, 8);
        get(flag_off, flag_ap, mes_out);
        if (mes_out.code != 0) {
            return 0;  // 出错时返回0
        }

        // 2. 确定当前使用的副本偏移
        size_t current_top_off = (flag_val == 0) ? top0_off : top1_off;

        // 3. 读取当前栈指针值（存的本身就是索引）
        bmms_f::agent_ptr top_ap(&stack_top_index, sizeof(size_t));
        get(current_top_off, top_ap, mes_out);
        if (mes_out.code != 0) {
            return 0;  // 出错时返回0
        }

        // 4. 已使用块数 = 最大逻辑块数 - 栈指针索引
        return math::nf_sub(DATA.logical_block_count_max_, stack_top_index);
    }

    size_t LDS::get_free_count(mes::a_mes& mes_out) {
        __RSM__;

        // 1. 获取当前使用的副本和栈指针值
        byte_8 flag_val = 0;
        size_t stack_top_index = 0;
        size_t top0_off = DATA.stack_info_.logical_block_stack_top_offset_0_offset;
        size_t top1_off = DATA.stack_info_.logical_block_stack_top_offset_1_offset;
        size_t flag_off = DATA.stack_info_.logical_block_stack_top_offset_flag_offset;

        // 读取 flag
        bmms_f::agent_ptr flag_ap(&flag_val, 8);
        get(flag_off, flag_ap, mes_out);
        if (mes_out.code != 0) {
            return 0;  // 出错时返回0
        }

        // 2. 确定当前使用的副本偏移
        size_t current_top_off = (flag_val == 0) ? top0_off : top1_off;

        // 3. 读取当前栈指针值（存的本身就是索引）
        bmms_f::agent_ptr top_ap(&stack_top_index, sizeof(size_t));
        get(current_top_off, top_ap, mes_out);
        if (mes_out.code != 0) {
            return 0;  // 出错时返回0
        }

        // 4. 空闲块数 = 栈指针索引
        return stack_top_index;
    }
};
