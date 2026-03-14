export module MES;

#include "MES_dp.h"

export namespace mes{
#define __EXIT__ std::exit(EXIT_FAILURE);
#define EXIT_FAILURE 1
	struct a_mes {
		a_mes() = default;//默认构造函数

		a_mes(byte_8 mode_code_in, byte_8 line, str message_in) {
			mode_code = mode_code_in;
			code = line;
			#if defined(DEBUG_MODE) && defined(SHOW_STATIC_STR_MESSAGE)
			message = message_in;
			#endif
		};

		a_mes& operator=(const a_mes& other) {
			// 检查自赋值
			if (this != &other) {
				mode_code = other.mode_code;
				code = other.code;
				#if defined(DEBUG_MODE) && defined(SHOW_STATIC_STR_MESSAGE)
				message = other.message;
				#endif
			}
			// 返回引用以支持链式赋值
			return *this;
		};

		// 重载比较等于运算符
		bool operator==(const a_mes& other) const {
			return ( mode_code == other.mode_code && code == other.code );
		};
		// 重载不等于运算符
		bool operator!=(const a_mes& other) const {
			return ( mode_code!= other.mode_code || code!= other.code );
		};
		// 重载大于运算符
		bool operator>(const a_mes& other) const {
			// 不同mode_code的消息类型不能比较
			if (mode_code != other.mode_code) {
				output << "MODC:";
				output << MOD_CODE_MES;
				output << " MESC:";
				output << 0;
				output << "coding error compare mes::a_mes with different mode_code by greater-than" << endl;
				__EXIT__
			}
			return (code > other.code);
		};
		// 重载小于运算符
		bool operator<(const a_mes& other) const {
			// 不同mode_code的消息类型不能比较
			if (mode_code != other.mode_code) {
				output << "MODC:";
				output << MOD_CODE_MES;
				output << " MESC:";
				output << 1;
				output << "coding error compare mes::a_mes with different mode_code by less-than" << endl;
				__EXIT__
			}

			return (code < other.code);
		};
		// 重载大于等于运算符
		bool operator>=(const a_mes& other) const {
			// 不同mode_code的消息类型不能比较
			if (mode_code != other.mode_code) {
				output << "MODC:";
				output << MOD_CODE_MES;
				output << " MESC:";
				output << 0;
				output << "coding error compare mes::a_mes with different mode_code by greater-than" << endl;
				__EXIT__
			}
			return (code >= other.code);
		};
		// 重载小于等于运算符
		bool operator<=(const a_mes& other) const {
			// 不同mode_code的消息类型不能比较
			if (mode_code != other.mode_code) {
				output << "MODC:";
				output << MOD_CODE_MES;
				output << " MESC:";
				output << 1;
				output << "coding error compare mes::a_mes with different mode_code by less-than" << endl;
				__EXIT__
			}

			return (code <= other.code);
		};

		void out() {
			output << "MODC:" << mode_code << " MESC:" << code;
			#if defined(DEBUG_MODE) && defined(SHOW_STATIC_STR_MESSAGE)
			output << " " << message;
			#endif
			output << endl;
		}

		byte_8 mode_code = 0;
		byte_8 code      = 0;
		#if defined(DEBUG_MODE) && defined(SHOW_STATIC_STR_MESSAGE)
		str message = "";
		#endif
	};

	namespace err {
		// 从INIT.h导入自动协调好的模块代码宏 MOD_CODE_MES
		inline a_mes no_sug() { return a_mes(MOD_CODE_MES, __LINE__, "no_suggestion_in_res"); };
		inline a_mes no_war() { return a_mes(MOD_CODE_MES, __LINE__, "no_warning_in_res"); };
		inline a_mes no_err() { return a_mes(MOD_CODE_MES, __LINE__, "no_error_in_res"); };
		inline a_mes no_dyn_mes() { return a_mes(MOD_CODE_MES, __LINE__, "no_dynamic_message_in_res"); };
		inline a_mes code_error_no_chk_sug_and_try_get_res() { return a_mes(MOD_CODE_MES, __LINE__, "code_error_no_suggestion_and_try_to_get_result"); };
		inline a_mes code_error_no_chk_war_and_try_get_res() { return a_mes(MOD_CODE_MES, __LINE__, "code_error_no_warning_and_try_to_get_result"); };
		inline a_mes code_error_no_chk_err_and_try_get_res() { return a_mes(MOD_CODE_MES, __LINE__, "code_error_no_error_and_try_to_get_result"); };
		inline a_mes code_error_no_chk_dyn_mes_and_try_get_res() { return a_mes(MOD_CODE_MES, __LINE__, "code_error_no_dynamic_message_and_try_to_get_result"); };
		inline a_mes code_error_freeze_res_again() { return a_mes(MOD_CODE_MES, __LINE__, "code_error_freeze_result_again"); };
		inline a_mes code_error_unfreeze_res_again() { return a_mes(MOD_CODE_MES, __LINE__, "code_error_unfreeze_result_again"); };
		inline a_mes code_error_try_set_info_when_res_is_freeze() { return a_mes(MOD_CODE_MES, __LINE__, "code_error_try_to_set_info_when_result_is_freeze"); };
	};

	template<typename T>
	requires std::movable<T> || std::copyable<T>
	class res {
	public:
		// 构造函数
		res() = default;
		// 拷贝构造
		res(const T& result_in) : result(result_in) {}
		// 初始构造
		res(T& result_in, bool move_data) {
			if (move_data) {
				result = move(result_in);
			}
			else {
				result = result_in;
			}
		};
		// 移动构造
		res(res&& other) noexcept
			: time_or_temp(std::exchange(other.time_or_temp, 0))
			, info(std::exchange(other.info, 0))
			, maes{ move(other.maes[0]),
				   move(other.maes[1]),
				   move(other.maes[2]) }
			#if defined(DEBUG_MODE) && defined(SHOW_DYNAMIC_STR_MESSAGE)
			, dynamic_mes(move(other.dynamic_mes))
			#endif
			#if defined(DEBUG_MODE) && defined(SHOW_CALL_CHAIN_MESSAGE)
			, call_chain(move(other.call_chain))
			#endif
			, result(move(other.result))
		{
			// 移动后，源对象失效
			other.clear();
		}
		// 禁止复制
		res(const res&) = delete;

		// 清空函数 便于内存复用，减少内存分配时间
		void clear() {
			time_or_temp = 0;
			info.clear();
			maes[0] = a_mes{};
			maes[1] = a_mes{};
			maes[2] = a_mes{};
			#if defined(DEBUG_MODE) && defined(SHOW_DYNAMIC_STR_MESSAGE)
			dynamic_mes.clear();
			#endif
			#if defined(DEBUG_MODE) && defined(SHOW_CALL_CHAIN_MESSAGE)
			call_chain.clear();
			#endif
			result = T{};
		};

		// 设置信息
		void set_time_or_temp(byte_8 time_or_temp_in) {	
			if (has_freeze()) {
				err::code_error_try_set_info_when_res_is_freeze().out();
				__EXIT__
			}
			else {
				time_or_temp = time_or_temp_in;
			}
		};
		void set_sug(const a_mes& mes_in, bool must_chk) {
			if (has_freeze()) {
				err::code_error_try_set_info_when_res_is_freeze().out();
				__EXIT__
			}
			else {
				info.set(0, 1);
				info.set(1, must_chk);
				maes[0] = mes_in;
			}
		};
		void set_war(const a_mes& war_in, bool must_chk) {
			if (has_freeze()) {
				err::code_error_try_set_info_when_res_is_freeze().out();
				__EXIT__
			}
			else {
				info.set(2, 1);
				info.set(3, must_chk);
				maes[1] = war_in;
			}
		};
		void set_err(const a_mes& err_in, bool must_chk) {
			if (has_freeze()) {
				err::code_error_try_set_info_when_res_is_freeze().out();
				__EXIT__
			}
			else {
				info.set(4, 1);
				info.set(5, must_chk);
				maes[2] = err_in;
			}
		};
		void set_dynamic_mes(const str& mes_in, bool must_chk) {
			if (has_freeze()) {
				err::code_error_try_set_info_when_res_is_freeze().out();
				__EXIT__
			}
			else {
				#if defined(DEBUG_MODE) && defined(SHOW_DYNAMIC_STR_MESSAGE)
				info.set(6, 1);
				info.set(7, must_chk);
				dynamic_mes = mes_in;
				#endif
			}
		};
		//使用引用，这样不开启这个功能的时候也不会有拷贝开销，上面也是
		void set_call_chain(
			const str& file_in, 
			const str& function_name_in, 
			const str& line_in
		) {
			if (has_freeze()) {
				err::code_error_try_set_info_when_res_is_freeze().out();
				__EXIT__
			}
			else {
				#if defined(DEBUG_MODE) && defined(SHOW_CALL_CHAIN_MESSAGE)
				info.set(8, 1);
				//info.set(9, 1);//占位，没什么用 注释掉以提高性能
				call_chain.push_back(file_in);
				call_chain.push_back(function_name_in);
				call_chain.push_back(line_in);
				#endif
			}
		};
		//冻结后不可再设置信息
		void freeze() {
			if (info[15]) { 
				err::code_error_freeze_res_again().out();
				__EXIT__
			}
			else {
				info.set(15, 1);
			}
		}
		void unfreeze() {
			if (!info[15]) {
				err::code_error_unfreeze_res_again().out();
				__EXIT__
			}
			else {
				info.set(15, 0);
			}
		}

		// 获取状态
		bool has_freeze() {
			return info[15];
		};
		bool has_sug() {
			return info[0];
		};
		bool has_war() {
			return info[2];
		};
		bool has_err() {
			return info[4];
		};
		bool has_dynamic_mes() {
			#if defined(DEBUG_MODE) && defined(SHOW_DYNAMIC_STR_MESSAGE)
			return info[6];
			#else
			return false;
			#endif
		};
		bool has_call_chain() {
			#if defined(DEBUG_MODE) && defined(SHOW_CALL_CHAIN_MESSAGE)
			return info[8];
			#else
			return false;
			#endif
		};

		bool get_sug_must_chk() {
			return info[1];
		};
		bool get_war_must_chk() {
			return info[3];
		};
		bool get_err_must_chk() {
			return info[5];
		};
		bool get_dynamic_mes_must_chk() {
			#if defined(DEBUG_MODE) && defined(SHOW_DYNAMIC_STR_MESSAGE)
			return info[7];
			#else
			return false;
			#endif
		};
		//调用链不做检查要求，因为调用链式附加信息

		// 获取信息
		byte_8 get_time_or_temp() {
			return time_or_temp;
		};
		a_mes get_sug() {
			if (! info[0]) {
				return err::no_sug();
			}
			else {
				info.set(1, 0);
				return maes[0];
			}
		};
		a_mes get_war() {
			if (! info[2]) {
				return err::no_war();
			}
			else {
				info.set(3, 0);
				return maes[1];
			}
		};
		a_mes get_err() {
			if (! info[4]) {
				return err::no_err();
			}
			else {
				info.set(5, 0);
				return maes[2];
			}
		};
		str get_dynamic_mes() {
			#if defined(DEBUG_MODE) && defined(SHOW_DYNAMIC_STR_MESSAGE)
			if (! info[6]) {
				return err::no_dyn_mes().message;
			}
			else {
				info.set(7, 0);
				return dynamic_mes;
			}
			#else
			return err::no_dyn_mes().message;
			#endif
		};

		size_t get_call_chain_layer_depth() {
			#if defined(DEBUG_MODE) && defined(SHOW_CALL_CHAIN_MESSAGE)
			if (! info[8]) {
				return 0;
			}
			else {
				return call_chain.size() / 3;
			}
			#else
			return 0;
			#endif
		};
		
		tuple<str, str, str> get_call_chain(size_t layer_depth) {
			#if defined(DEBUG_MODE) && defined(SHOW_CALL_CHAIN_MESSAGE)
			if (! info[8]) {
				return {"","",""};
			}
			else {
				info.set(9, 0);
				if (layer_depth >= call_chain.size() / 3) {
					return { "","","" };
				}
				else {
					return { 
						call_chain[layer_depth * 3     ],
						call_chain[layer_depth * 3 + 1 ],
						call_chain[layer_depth * 3 + 2 ]
					};
				}
			}
			#else
			return { "","","" };
			#endif
		};

		// 移动结果，操作后本包的结果将为空
		T move_result() {
			if (info[1]) {
				err::code_error_no_chk_sug_and_try_get_res().out();
				__EXIT__
			}
			if (info[3]) {
				err::code_error_no_chk_war_and_try_get_res().out();
				__EXIT__
			}
			if (info[5]) {
				err::code_error_no_chk_err_and_try_get_res().out();
				__EXIT__
			}

			return move(result);
		};

		// 获取结果，操作后本包保持结果
		T get_result() {
			if (info[1]) {
				err::code_error_no_chk_sug_and_try_get_res().out();
				__EXIT__
			}
			if (info[3]) {
				err::code_error_no_chk_war_and_try_get_res().out();
				__EXIT__
			}
			if (info[5]) {
				err::code_error_no_chk_err_and_try_get_res().out();
				__EXIT__
			}
			return result;
		};

	private:
		byte_8 time_or_temp = 0;
		bitset<16> info = 0;
		a_mes maes[3]{};
		#if defined(DEBUG_MODE) && defined(SHOW_DYNAMIC_STR_MESSAGE)
		str dynamic_mes = "";
		#endif

		#if defined(DEBUG_MODE) && defined(SHOW_CALL_CHAIN_MESSAGE)
		vector<str> call_chain = {};
		#endif
		T result;
	};
};