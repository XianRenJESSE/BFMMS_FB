//agent_ptr.cpp
module BMMS;

#include "BMMS_dp.h"

namespace bmms_f {

	#define EXIT_FAILURE 1

	#if defined(BMMS_INCLUDE_TEST)
	#define __CODING_ERROR__(mes)\
			mes().out(); \
			throw mes();
	#else
	#define __CODING_ERROR__(mes)\
			 mes().out();std::exit(EXIT_FAILURE);
	#endif

	// 地址&&大小构造函数
	agent_ptr::agent_ptr(
		void* ptr_in,
		size_t size_in
	) {
		check_building(ptr_in, size_in);
		raw_ptr = ptr_in;
		size = size_in;
	};

	// 将自身数据读取到代理指针
	void agent_ptr::load(
		size_t load_begin_in,
		size_t load_size_in,
		agent_ptr& save_agent_ptr_in,
		size_t     store_begin_in,
		bool       safe
	) {
		if (safe) {
			// 校验数据准备
			uintptr_t self_self_begin = __self_begin__();
			uintptr_t self_self_end = __self_end__();
			uintptr_t self_op_begin = __op_begin__(load_begin_in);
			uintptr_t self_op_end = __op_end__(load_begin_in, load_size_in);
			size_t    self_op_size = load_size_in;

			uintptr_t other_self_begin = save_agent_ptr_in.__self_begin__();
			uintptr_t other_self_end = save_agent_ptr_in.__self_end__();
			uintptr_t other_op_begin = save_agent_ptr_in.__op_begin__(store_begin_in);
			uintptr_t other_op_end = save_agent_ptr_in.__op_end__(store_begin_in, load_size_in);
			size_t    other_op_size = load_size_in;

			// 定理校验
			check_null_agent_ptr(save_agent_ptr_in);
			check_size(self_op_size, other_op_size);
			check_self_and_other_managemented_range(
				self_self_begin, self_self_end,
				other_self_begin, other_self_end
			);
			check_op_range_overlap(
				self_self_begin, self_self_end,
				self_op_begin, self_op_end, self_op_size,
				other_self_begin, other_self_end,
				other_op_begin, other_op_end, other_op_size
			);
			check_op_on_the_spot(self_op_begin, self_op_end, other_op_begin, other_op_end);
			check_op_range_not_overlapping(self_op_begin, self_op_end, other_op_begin, other_op_end);

			// 写入数据
			memcpy(
				reinterpret_cast<void*>(other_op_begin),
				reinterpret_cast<void*>(self_op_begin),
				load_size_in
			);
		}
		else {
			// 写入数据
			memcpy(
				reinterpret_cast<void*>(save_agent_ptr_in.__op_begin__(store_begin_in)),
				reinterpret_cast<void*>(__op_begin__(load_begin_in)),
				load_size_in
			);
		}

	};

	// 从代理指针读取数据写入到自身
	void agent_ptr::store(
		agent_ptr& load_agent_ptr_in,
		size_t     load_begin_in,
		size_t     load_size_in,
		size_t     store_begin_in,
		bool       safe
	) {
		if (safe) {
			// 校验数据准备
			uintptr_t self_self_begin = __self_begin__();
			uintptr_t self_self_end = __self_end__();
			uintptr_t self_op_begin = __op_begin__(store_begin_in);
			uintptr_t self_op_end = __op_end__(store_begin_in, load_size_in);
			size_t    self_op_size = load_size_in;

			uintptr_t other_self_begin = load_agent_ptr_in.__self_begin__();
			uintptr_t other_self_end = load_agent_ptr_in.__self_end__();
			uintptr_t other_op_begin = load_agent_ptr_in.__op_begin__(load_begin_in);
			uintptr_t other_op_end = load_agent_ptr_in.__op_end__(load_begin_in, load_size_in);
			size_t    other_op_size = load_size_in;

			// 定理校验
			check_null_agent_ptr(load_agent_ptr_in);
			check_size(self_op_size, other_op_size);
			check_self_and_other_managemented_range(
				self_self_begin, self_self_end,
				other_self_begin, other_self_end
			);
			check_op_range_overlap(
				self_self_begin, self_self_end,
				self_op_begin, self_op_end, self_op_size,
				other_self_begin, other_self_end,
				other_op_begin, other_op_end, other_op_size
			);
			check_op_on_the_spot(self_op_begin, self_op_end, other_op_begin, other_op_end);
			check_op_range_not_overlapping(self_op_begin, self_op_end, other_op_begin, other_op_end);
			// 从源代理指针读取数据写入到自身
			memcpy(
				reinterpret_cast<void*>(self_op_begin),
				reinterpret_cast<void*>(other_op_begin),
				load_size_in
			);
		}
		else {
			// 从源代理指针读取数据写入到自身
			memcpy(
				reinterpret_cast<void*>(__op_begin__(store_begin_in)),
				reinterpret_cast<void*>(load_agent_ptr_in.__op_begin__(load_begin_in)),
				load_size_in
			);
		}

	};

	// 将自身数据读取到void指针
	void agent_ptr::load_to_void_ptr(
		size_t load_begin_in,
		size_t load_size_in,
		void* store_ptr_in,
		bool       safe
	) {
		if (safe) {
			// 校验数据准备
			uintptr_t self_self_begin = __self_begin__();
			uintptr_t self_self_end = __self_end__();
			uintptr_t self_op_begin = __op_begin__(load_begin_in);
			uintptr_t self_op_end = __op_end__(load_begin_in, load_size_in);
			size_t    self_op_size = load_size_in;

			uintptr_t void_ptr_op_begin = __void_ptr_begin__(store_ptr_in);
			uintptr_t void_ptr_op_end = __void_ptr_end__(store_ptr_in, load_size_in);
			size_t    void_ptr_op_size = load_size_in;

			// 定理校验
			check_null_void_ptr(store_ptr_in);
			check_self_and_void_ptr_op_size(self_op_size, void_ptr_op_size);
			check_self_op_range(
				self_self_begin, self_self_end,
				self_op_begin, self_op_end
			);
			check_self_managemented_range_and_void_ptr_op_range_not_overlapping(
				self_self_begin, self_self_end,
				self_op_begin, self_op_end,
				void_ptr_op_begin, void_ptr_op_end
			);
			check_op_range_not_overlapping(self_op_begin, self_op_end, void_ptr_op_begin, void_ptr_op_end);

			// 读取数据到void指针
			memcpy(
				store_ptr_in,
				reinterpret_cast<void*>(self_op_begin),
				load_size_in
			);
		}
		else {
			// 读取数据到void指针
			memcpy(
				store_ptr_in,
				reinterpret_cast<void*>(__op_begin__(load_begin_in)),
				load_size_in
			);
		}
	};

	// 从void指针读数据写入自身
	void agent_ptr::store_from_void_ptr(
		void* load_ptr_in,
		size_t load_size_in,
		size_t store_begin_in,
		bool       safe
	) {
		if (safe) {
			// 校验数据准备
			uintptr_t self_self_begin = __self_begin__();
			uintptr_t self_self_end = __self_end__();
			uintptr_t self_op_begin = __op_begin__(store_begin_in);
			uintptr_t self_op_end = __op_end__(store_begin_in, load_size_in);
			size_t    self_op_size = load_size_in;

			uintptr_t void_ptr_op_begin = __void_ptr_begin__(load_ptr_in);
			uintptr_t void_ptr_op_end = __void_ptr_end__(load_ptr_in, load_size_in);
			size_t    void_ptr_op_size = load_size_in;

			// 定理校验
			check_null_void_ptr(load_ptr_in);
			check_self_and_void_ptr_op_size(self_op_size, void_ptr_op_size);
			check_self_op_range(
				self_self_begin, self_self_end,
				self_op_begin, self_op_end
			);
			check_self_managemented_range_and_void_ptr_op_range_not_overlapping(
				self_self_begin, self_self_end,
				self_op_begin, self_op_end,
				void_ptr_op_begin, void_ptr_op_end
			);
			check_op_range_not_overlapping(self_op_begin, self_op_end, void_ptr_op_begin, void_ptr_op_end);

			// 从void指针读取数据写入自身
			memcpy(
				reinterpret_cast<void*>(self_op_begin),
				load_ptr_in,
				load_size_in
			);
		}
		else {
			// 从void指针读取数据写入自身
			memcpy(
				reinterpret_cast<void*>(__op_begin__(store_begin_in)),
				load_ptr_in,
				load_size_in
			);
		}
	};

	// 清空数据
	void agent_ptr::clear() {
		if (raw_ptr != nullptr && size != 0) {
			memset(
				raw_ptr,
				0,
				size
			);
		}
	};

	// 范围清空数据
	void agent_ptr::clear_range(
		size_t begin_in,
		size_t size_in
	) {
		uintptr_t self_begin = __self_begin__();
		uintptr_t self_end = __self_end__();
		uintptr_t op_begin = __op_begin__(begin_in);
		uintptr_t op_end = __op_end__(begin_in, size_in);
		// 范围检查
		check_self_op_range(
			self_begin,
			self_end,
			op_begin,
			op_end
		);

		//清空数据
		memset(
			reinterpret_cast<void*>(
				__op_begin__(begin_in)
				),
			0,
			size_in
		);
	};

	// 获取指针
	void agent_ptr::get_void_ptr(void*& ptr_out) {
		ptr_out = raw_ptr;
	};

	// 获取是否为空
	void agent_ptr::get_is_null(bool& is_null_in) {
		if (size != 0) {
			is_null_in = false;
		}
		else {
			is_null_in = true;
		}
	};

	// 获取大小
	void agent_ptr::get_size(size_t& size_out) {
		size_out = size;
	};

	// 获取起点
	void agent_ptr::get_begin(void*& begin_out) {
		begin_out = raw_ptr;
	};

	// 获取终点
	void agent_ptr::get_end(void*& end_out) {
		//检查是否溢出
		if (!no_overflow(__self_begin__(), size)) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_self_end_overflow)
		}

		end_out = reinterpret_cast<void*>(
			reinterpret_cast<uintptr_t>(raw_ptr)
			+
			size
			);
	};

	// 获取范围检查布尔
	void agent_ptr::get_check_range_bool(
		size_t begin_in,
		size_t size_in,
		bool& is_valid_out
	) {
		if (!no_overflow(begin_in, size_in)) {
			is_valid_out = false;
		}
		else if (!(0 <= begin_in && (begin_in + size_in) <= size)) {
			is_valid_out = false;
		}
		else {
			is_valid_out = true;
		}
	};

	// 获取构造无溢出
	void agent_ptr::check_building_no_overflow(
		void*& ptr_in,
		size_t size_in,
		bool& building_is_no_overflow
	) {
		if (no_overflow(reinterpret_cast<uintptr_t>(ptr_in), size_in)) {
			building_is_no_overflow = true;
		}
		else {
			building_is_no_overflow = false;
		}
	};

	// 获取操作区间运算无溢出
	void  agent_ptr::check_end_no_overflow(
		size_t begin_in,
		size_t size_in,
		bool& is_no_overflow
	) {
		if (no_overflow(begin_in, size_in)) {
			if (begin_in + size_in < size) {
				is_no_overflow = true;
			}
			else {
				is_no_overflow = false;
			}
		}
		else {
			is_no_overflow = false;
		}
	};

	uintptr_t agent_ptr::__self_begin__() {
		return reinterpret_cast<uintptr_t>(raw_ptr);
	};

	uintptr_t agent_ptr::__self_end__() {
		if (!no_overflow(__self_begin__(), size)) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_self_end_overflow)
		}
		return reinterpret_cast<uintptr_t>(raw_ptr) + size;
	};

	uintptr_t agent_ptr::__op_begin__(
		size_t begin_in
	) {
		//检测是否会导致溢出
		if (!no_overflow(__self_begin__(), begin_in)) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_op_begin_overflow)
		}
		return  __self_begin__() + begin_in;
	};

	uintptr_t agent_ptr::__op_end__(
		size_t op_begin_in,
		size_t op_size_in
	) {
		//检测是否会导致溢出
		if (!no_overflow(__self_begin__(), op_begin_in)) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_op_begin_overflow)
		}
		if (!no_overflow(__self_begin__() + op_begin_in, op_size_in)) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_op_end_overflow)
		}
		return  __self_begin__() + op_begin_in + op_size_in;
	};

	uintptr_t agent_ptr::__void_ptr_begin__(
		void*& op_ptr_in
	) {
		return reinterpret_cast<uintptr_t>(op_ptr_in);
	};

	uintptr_t agent_ptr::__void_ptr_end__(
		void*& op_ptr_in,
		size_t op_size_in
	) {
		//检测是否会溢出
		if (!no_overflow(reinterpret_cast<uintptr_t>(op_ptr_in), op_size_in)) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_op_end_overflow)
		}
		return reinterpret_cast<uintptr_t>(op_ptr_in) + op_size_in;
	};

	//========== 公共方法专有校验 ==========
	void agent_ptr::check_building(
		void*& ptr_in,
		size_t& size_in
	) {
		if (ptr_in == nullptr) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_ctor_null_ptr)
		}
		if (size_in == 0) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_ctor_zero_size)
		}
		if (!no_overflow(reinterpret_cast<uintptr_t>(ptr_in), size_in)) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_ctor_overflow)
		}
	};

	//========== 定理校验 ==========

	//前提校验
	bool agent_ptr::no_overflow(uintptr_t a, size_t b) {
		return a <= UINTPTR_MAX - b;
	};

	// 空代理指针检查
	void agent_ptr::check_null_agent_ptr(agent_ptr& other_agent_ptr_in) {
		if (raw_ptr == nullptr) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_self_null)
		}
		if (other_agent_ptr_in.raw_ptr == nullptr) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_other_null)
		}
	};

	// 大小检查
	void agent_ptr::check_size(
		size_t& self_op_size_in,
		size_t& other_op_size_in
	) {
		if (self_op_size_in == 0) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_self_op_zero)
		}
		if (other_op_size_in == 0) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_other_op_zero)
		}
	};

	// 双方代理指针部分重叠管理空间检查
	void agent_ptr::check_self_and_other_managemented_range(
		uintptr_t& self_self_begin_in,
		uintptr_t& self_self_end_in,
		uintptr_t& other_self_begin_in,
		uintptr_t& other_self_end_in
	) {
		if (
			(
				self_self_begin_in < other_self_begin_in
				&&
				(
					other_self_begin_in < self_self_end_in
					&&
					self_self_end_in <= other_self_end_in
					)
				)
			||
			(
				(
					other_self_begin_in <= self_self_begin_in
					&&
					self_self_begin_in < other_self_end_in
					)
				&&
				other_self_end_in < self_self_end_in
				)
			) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_managed_ranges_overlap)
		}
	};

	// 操作区间有效检查
	void agent_ptr::check_op_range_overlap(
		uintptr_t& self_self_begin_in,
		uintptr_t& self_self_end_in,
		uintptr_t& self_op_begin_in,
		uintptr_t& self_op_end_in_in,
		size_t& self_op_size,
		uintptr_t& other_self_begin_in,
		uintptr_t& other_self_end_in,
		uintptr_t& other_op_begin_in,
		uintptr_t& other_op_end_in,
		size_t& other_op_size
	) {
		if (
			!(self_self_begin_in <= self_op_begin_in && self_op_end_in_in <= self_self_end_in)
			) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_self_op_out_of_range)
		}
		if (!
			(other_self_begin_in <= other_op_begin_in && other_op_end_in <= other_self_end_in)
			) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_other_op_out_of_range)
		}
	};

	// 原地操作数据检查
	void agent_ptr::check_op_on_the_spot(
		uintptr_t& self_op_begin_in,
		uintptr_t& self_op_end_in,
		uintptr_t& other_op_begin_in,
		uintptr_t& other_op_end_in
	) {
		if (
			!(
				self_op_begin_in != other_op_begin_in
				&& self_op_end_in != other_op_end_in
				)
			) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_src_dst_identical)
		}
	};

	// 操作范围不重叠检查
	void agent_ptr::check_op_range_not_overlapping(
		uintptr_t& self_begin_in,
		uintptr_t& self_end_in,
		uintptr_t& other_begin_in,
		uintptr_t& other_end_in
	) {
		if (
			!(
				self_end_in <= other_begin_in
				||
				other_end_in <= self_begin_in
				)
			) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_op_ranges_overlap)
		}
	};

	// 空void指针检查
	void agent_ptr::check_null_void_ptr(void* ptr_in) {
		if (ptr_in == nullptr) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_void_ptr_null)
		}
	};

	//代理指针管理的空间 与 void*指针操作的空间 不 部分重叠 检查
	void agent_ptr::check_self_managemented_range_and_void_ptr_op_range_not_overlapping(
		uintptr_t& self_self_begin_in,
		uintptr_t& self_self_end_in,
		uintptr_t& self_op_begin_in,
		uintptr_t& self_op_end_in_in,
		uintptr_t& void_ptr_op_begin_in,
		uintptr_t& void_ptr_op_end_in
	) {
		if (
			(
				void_ptr_op_begin_in < self_self_begin_in
				&&
				(
					self_self_begin_in < void_ptr_op_end_in
					&&
					void_ptr_op_end_in <= self_self_end_in
					)
				)
			||
			(
				(
					self_self_begin_in <= void_ptr_op_begin_in
					&&
					void_ptr_op_begin_in < self_self_end_in
					)
				&&
				self_self_end_in < void_ptr_op_end_in
				)
			) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_managed_range_overlaps_void_range)
		}
	};

	// self与void_ptr操作的大小有效性检验
	void agent_ptr::check_self_and_void_ptr_op_size(
		size_t& self_op_size_in,
		size_t& void_ptr_op_size_in
	) {
		if (!(size >= void_ptr_op_size_in)
			) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_managed_range_too_small)
		}
		if (self_op_size_in == 0) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_self_op_zero)
		}
		if (void_ptr_op_size_in == 0) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_other_op_zero)
		}
	};

	// 操作的区间有效性检验
	void agent_ptr::check_self_op_range(
		uintptr_t& self_self_begin_in,
		uintptr_t& self_self_end_in,
		uintptr_t& self_op_begin_in,
		uintptr_t& self_op_end_in_in
	) {
		if (!(self_self_begin_in <= self_op_begin_in && self_op_end_in_in <= self_self_end_in)) {
			__CODING_ERROR__(bmms_m::err::coding_err_agent_ptr_self_op_out_of_range)
		}
	};
};