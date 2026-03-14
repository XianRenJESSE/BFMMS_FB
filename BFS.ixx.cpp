export module BFS;

#include "BFS_dp.h"

export namespace bfs_f {

	class bin_file {
	public:

		friend class bin_file;
		//============================= 初始化 =============================

		enum class init_type {
			open_existed,
			cover_and_create,
			create
		};

		bin_file() = default;

		// 从二进制文件创建文件对象
		bin_file(
			str       path_in, 
			size_t    size_in,
			init_type init_type_in,
			mes::a_mes& mes_out
		);

		~bin_file();

		//============================= 访问控制 =============================

		// 禁止拷贝
		bin_file(const bin_file&) = delete;
		bin_file& operator=(const bin_file&) = delete;

		// 支持移动
		bin_file(bin_file&& other)noexcept;
		bin_file& operator=(bin_file&& other) noexcept;

		//============================= 文件本身操作 =============================

		// 从已二进制有文件对象创建文件对象
		void open(
			str       path_in,
			size_t    size_in,
			init_type init_type_in,
			mes::a_mes& mes_out
		);

		// 立即刷新缓冲区，保存数据
		void flush();

		// 关闭
		void close(
			mes::a_mes& mes_out
		);

		// 重新设置大小
		void resize(
			size_t size_in,
			mes::a_mes& mes_out
		);

		// 清空文件
		void clear(
			mes::a_mes& mes_out
		);

		// 范围清空文件
		void clear_range(
			size_t begin_in,
			size_t size_in,
			mes::a_mes& mes_out
		);

		// 另存为
		void save_as(
			str path,
			mes::a_mes& mes_out
		);

		//============================= 数据交换 =============================

		// 从文件读取数据到自身
		void load(
			size_t      load_begin_in,
			size_t      load_size_in,
			bin_file&   save_bin_file_in,
			size_t      store_begin_in,
			bool        safe,
			bool        flush_instantly,
			mes::a_mes& mes_out
		);

		// 从自身数据写入文件
		void store(
			bin_file&   load_bin_file_in,
			size_t      load_begin_in,
			size_t      load_size_in,
			size_t      store_begin_in,
			bool        safe,
			bool        flush_instantly,
			mes::a_mes& mes_out
		);

		// 将自身数据读取到代理指针
		void load_to_agent_ptr(
			size_t           load_begin_in,
			size_t           load_size_in,
			bmms_f::agent_ptr& save_agent_ptr_in,
			size_t           store_begin_in,
			bool             safe,
			mes::a_mes&      mes_out
		);

		// 从代理指针读取数据写入到自身
		void store_from_agent_ptr(
			bmms_f::agent_ptr& load_agent_ptr_in,
			size_t           load_begin_in,
			size_t           load_size_in,
			size_t           store_begin_in,
			bool             safe,
			bool             flush_instantly,
			mes::a_mes&      mes_out
		);

		// 将自身数据读到void指针
		void load_to_void_ptr(
			size_t      load_begin_in,
			size_t      load_size_in,
			void*       store_ptr_in,
			bool        safe,
			mes::a_mes& mes_out
		);

		// 将void指针的数据写到自身
		void store_from_void_ptr(
			void*       load_ptr_in,
			size_t      load_size_in,
			size_t      store_begin_in,
			bool        safe,
			mes::a_mes& mes_out
		);

		// ============================ 信息获取函数 ============================

		// 检查文件操作区间有效性
		bool __check_file_range__(size_t begin, size_t size);

		// 检查路径是否合法以消除std::filesystem对于路径非法字符在各个平台上的UB
		// 请根据平台修改此函数的实现！！！
		// 如果需要性能可以继承然后定义为空实现
		// 内部的代码只是例子，建议根据项目实际需要调整
		bool __check_path_valid__(str& path_in);

		// 检查操作大小有效性
		inline bool __check_op_size__(size_t size);

		// 检查操作区间是否重叠
		inline bool __check_range_overlap__(
			size_t self_begin, size_t self_size,
			size_t other_begin, size_t other_size
		);

		// 检查两个文件是否为同一文件，如果是则结束进程
		inline void __check_not_same_file_and_exit__(bin_file& other);

		inline bool   __check_inited__();
		inline bool   __same_file__(bin_file& other);
		inline size_t __file_size__();
		inline size_t __self_begin__();
		inline size_t __self_end__();
		inline size_t __op_begin__(size_t begin_in);
		inline size_t __op_end__(size_t op_begin_in, size_t op_size_in);

		// ====================== 辅助函数 ======================

		// 进入未初始化状态（清理资源）
		void __enter_uninitialized_state__();

		// 尝试重新打开原文件，如果失败则进入未初始化状态
		void __try_reopen_and_enter_uninitialized_state__(mes::a_mes& mes_out);

		// 清理失败时的资源
		void __cleanup_on_failure__();

		// ============================ 定理检查函数 ============================
		// 前提检查
		inline bool no_overflow(size_t a, size_t b);

		// 文件到文件操作检查
		void check_basic_validity(
			size_t& self_op_begin,
			size_t& self_op_size,
			size_t& self_self_begin,
			size_t& self_self_end,
			size_t& other_op_begin,
			size_t& other_op_size,
			size_t& other_self_begin,
			size_t& other_self_end
		);

		void check_not_same_file(bin_file& other);

		void check_op_range_not_overlapping(
			size_t& self_op_begin,
			size_t& self_op_size,
			size_t& other_op_begin,
			size_t& other_op_size
		);

		// 文件与代理指针交互检查
		void check_file_to_agent_ptr_validity(
			size_t& file_op_begin,
			size_t& file_op_size,
			size_t& file_self_begin,
			size_t& file_self_end,
			bmms_f::agent_ptr& agent_ptr_in,
			size_t& agent_ptr_op_begin
		);

		void check_agent_ptr_to_file_validity(
			bmms_f::agent_ptr& agent_ptr_in,
			size_t& agent_ptr_op_begin,
			size_t& agent_ptr_op_size,
			size_t& file_op_begin,
			size_t& file_self_begin,
			size_t& file_self_end
		);

		// 文件与void指针交互检查
		void check_file_to_void_ptr_validity(
			size_t& file_op_begin,
			size_t& file_op_size,
			size_t& file_self_begin,
			size_t& file_self_end,
			void*& void_ptr_in
		);

		void check_void_ptr_to_file_validity(
			void*& void_ptr_in,
			size_t& void_ptr_op_size,
			size_t& file_op_begin,
			size_t& file_self_begin,
			size_t& file_self_end
		);

		#if defined(BFS_INCLUDE_TEST)
		void test();
		#endif

	private:
		bool inited = false;
		byte_1 fill[15];
		unique_ptr<std::fstream> filestream_ptr = nullptr;
		str  path = "";
	};

	class alignas(64) file_agent_ptr {
	public:
		friend class file_agent_ptr;

		//============================ 构造 ============================

		// 默认构造函数 得到空指针
		file_agent_ptr() = default;

		// 从bin_file 构造函数
		file_agent_ptr(
			const shared_ptr<bfs_f::bin_file>& bin_file_in,
			size_t           begin_in,
			size_t           size_in,
			bmms_f::agent_ptr& cache_agent_ptr_in,
			mes::a_mes& mes_out
		);

		~file_agent_ptr();

		// 禁用拷贝
		file_agent_ptr(const file_agent_ptr&) = delete;
		file_agent_ptr& operator=(const file_agent_ptr&) = delete;

		// 支持移动
		file_agent_ptr(file_agent_ptr&& other) noexcept;
		file_agent_ptr& operator=(file_agent_ptr&& other) noexcept;

		//============================ 数据控制 ============================

		// 清空数据
		void clear(
			mes::a_mes& mes_out
		);

		// 范围清空数据
		void clear_range(
			size_t begin_in,
			size_t size_in,
			mes::a_mes& mes_out
		);

		void push_cache_to_bin_file(
			mes::a_mes& mes_out
		);

		void pull_cache_from_bin_file(
			mes::a_mes& mes_out
		);

		//============================ 数据交换 ============================

		// 将自身数据读取到file_agent_ptr
		void load(
			size_t          load_begin_in,
			size_t          load_size_in,
			file_agent_ptr& save_file_agent_ptr_in,
			size_t          store_begin_in,
			bool            safe,
			mes::a_mes& mes_out
		);

		// 从file_agent_ptr读取数据写入到自身
		void store(
			file_agent_ptr& load__file_agent_ptr_in,
			size_t          load_begin_in,
			size_t          load_size_in,
			size_t          store_begin_in,
			bool            safe,
			mes::a_mes& mes_out
		);

		// 将自身数据读取到agent_ptr
		void load_to_agent_ptr(
			size_t           load_begin_in,
			size_t           load_size_in,
			bmms_f::agent_ptr& save_agent_ptr_in,
			size_t           store_begin_in,
			bool             safe,
			mes::a_mes& mes_out
		);

		// 从agent_ptr读取数据写入到自身
		void store_from_agent_ptr(
			bmms_f::agent_ptr& load_agent_ptr_in,
			size_t           load_begin_in,
			size_t           load_size_in,
			size_t           store_begin_in,
			bool             safe,
			mes::a_mes& mes_out
		);

		// 将自身数据读取到void指针
		void load_to_void_ptr(
			size_t      load_begin_in,
			size_t      load_size_in,
			void* store_ptr_in,
			bool        safe,
			mes::a_mes& mes_out
		);

		// 从void指针读数据写入自身
		void store_from_void_ptr(
			void* load_ptr_in,
			size_t      load_size_in,
			size_t      store_begin_in,
			bool        safe,
			mes::a_mes& mes_out
		);

		//============================ 测试 ============================

#if defined(BFS_INCLUDE_TEST)
		void test();
#endif

		//============================ 信息获取 ============================

		// 基本状态信息
		void get_is_inited(bool& is_inited_out);
		void get_cache_begin_about_bin_file(size_t& begin_out);
		void get_cache_agent_ptr(bmms_f::agent_ptr& cache_agent_ptr_out);
		void get_cache_raw_ptr(void*& raw_ptr_out);
		void get_cache_size(size_t& cache_size_out);
		void get_dirty_flag(bool& is_dirty_out);
		void get_associated_bin_file_shared_ptr(
			shared_ptr<bfs_f::bin_file>& bin_file_out
		);
		void check_associated_with_bin_file_shared_ptr(
			shared_ptr<bfs_f::bin_file> bin_file_in,
			bool& is_associated_out,
			mes::a_mes& mes_out
		);

		// 检查指定文件范围是否在当前管理范围内
		void check_file_range_in_managed(
			const shared_ptr<bfs_f::bin_file>& bin_file_in,
			size_t file_begin,
			size_t size_in,
			bool& is_in_managed_out,
			mes::a_mes& mes_out
		);

		void check_cache_range_in_managed(
			size_t cache_begin,
			size_t size_in,
			bool& is_in_managed_out,
			mes::a_mes& mes_out
		);

		// 大小信息
		void get_managed_file_size(
			size_t& managed_size_out,
			mes::a_mes& mes_out
		);

		void get_cache_used_size(
			size_t& used_size_out,
			mes::a_mes& mes_out
		);

		// 有效性检查
		void check_file_still_valid(
			bool& is_valid_out,
			mes::a_mes& mes_out
		);

		// 位置映射
		void calculate_cache_position(
			size_t file_absolute_pos,
			size_t& cache_pos_out,
			bool& is_mappable_out,
			mes::a_mes& mes_out
		);

		// 位置映射
		void calculate_file_position(
			size_t cache_pos,
			size_t& file_absolute_pos_out,
			bool& is_mappable_out,
			mes::a_mes& mes_out
		);

		//============================ 定理校验 ============================
		bool __check_self_valid__(
			mes::a_mes& mes_out
		);
		bool __check_file_range_valid__(
			bfs_f::bin_file& file_in,
			size_t& file_begin,
			size_t& size_in,
			mes::a_mes& mes_out
		);
		bool __check_cache_range_valid__(
			size_t& cache_begin,
			size_t& size_in,
			mes::a_mes& mes_out
		);
		bool __check_two_fap_compatible__(
			file_agent_ptr& other_fap,
			size_t& self_cache_begin,
			size_t& other_cache_begin,
			size_t& size_in,
			mes::a_mes& mes_out
		);

	private:

		bmms_f::agent_ptr cache_ptr = bmms_f::agent_ptr();
		shared_ptr<bfs_f::bin_file> bin_file_shared_ptr = nullptr;
		size_t cache_begin_about_bin_file = 0;
		bool   inited = false;
		bool   dirty = false;
		byte_1 fill[\
			64       \
			- sizeof(bmms_f::agent_ptr) \
			- sizeof(shared_ptr<bfs_f::bin_file>) \
			- sizeof(size_t) \
			- sizeof(bool) * 2 \
		];

		// 私有辅助函数
		void __mark_dirty__();
		void __clear_dirty__();
		void __cleanup_on_construct_failure__();
	};

	namespace dir_control {

		bool __check_path_valid__(str& path_in);

		void add_dir(
			str path,
			mes::a_mes& mes_out
		);

		void del(
			str path,
			mes::a_mes& mes_out
		);

		void get_dir_list(
			str path,
			vector<str>& dir_list_path_out,
			mes::a_mes& mes_out
		);

		void get_list_file(
			str path,
			vector<str>& file_list_path_out,
			mes::a_mes&  mes_out
		);
		
		void get_path_is_excited(
			str path,
			bool& is_excited_out,
			mes::a_mes& mes_out
		);

		void get_is_dir(
			str path,
			bool& is_dir_out,
			mes::a_mes& mes_out
		);

		void get_is_file(
			str path,
			bool& is_file_out,
			mes::a_mes& mes_out
		);

		void get_accessible(
			str path,
			bool& accessible_out,
			mes::a_mes& mes_out
		);

		void rename_file(
			str old_path,
			str new_path,
			mes::a_mes& mes_out,
			bool overwrite = false
		);

		#if defined(BFS_INCLUDE_TEST)
		void test();
		#endif
	};
};