// LFMMS_logical_volume_static_cache.cpp
module LFMMS;

#include "LFMMS_dp.h"

//priority_matrix
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
    #define LDCP logical_volume::data::static_cache::priority_matrix

    // 辅助：计算位图大小（字节数）
    size_t LDCP::bitmap_size() const {
        return (row_count * column_count + 7) / 8;
    }

    // 辅助：计算位图中的位置
    void LDCP::get_bit_position(size_t row, size_t col, size_t& byte_idx, size_t& bit_idx) const {
        size_t index = row * column_count + col;
        byte_idx = index / 8;
        bit_idx = index % 8;
    }

    // 辅助：读写位
    bool LDCP::test_bit(size_t byte_idx, size_t bit_idx) const {
        return (priority_matrix_info[byte_idx] >> bit_idx) & 1;
    }

    void LDCP::set_bit(size_t byte_idx, size_t bit_idx, bool value) {
        if (value) {
            priority_matrix_info[byte_idx] |= (1 << bit_idx);
        }
        else {
            priority_matrix_info[byte_idx] &= ~(1 << bit_idx);
        }
    }

    // 标记普通优先级项
    void LDCP::mark_normal_priority(
        op          op_in,
        size_t      row_in,
        size_t& column_io,  // set时输出，clean时输入
        mes::a_mes  mes_out
    ) {
        // 1. 参数校验
        if (row_in >= row_count) {
            mes_out = lfmms_m::err::service_err_priority_matrix_row_out_of_range();
            return;
        }

        // 2. 常驻行检查（普通项不能标记在常驻行）
        if (row_in < permanent_row_end) {
            mes_out = lfmms_m::err::service_err_priority_matrix_row_is_permanent();
            return;
        }

        if (op_in == op::set) {
            // 3. SET：寻找空闲列
            size_t start_col = 0;
            size_t found_col = SIZE_MAX;

            // 在当前行查找空闲位置
            for (size_t col = start_col; col < column_count; col++) {
                size_t byte_idx, bit_idx;
                get_bit_position(row_in, col, byte_idx, bit_idx);

                if (!test_bit(byte_idx, bit_idx)) {
                    found_col = col;
                    break;
                }
            }

            if (found_col == SIZE_MAX) {
                mes_out = lfmms_m::err::service_err_priority_matrix_row_full();
                return;
            }

            // 标记占用
            size_t byte_idx, bit_idx;
            get_bit_position(row_in, found_col, byte_idx, bit_idx);
            set_bit(byte_idx, bit_idx, true);

            // 输出列索引
            column_io = found_col;
        }
        else {  // op::clean
            // 4. CLEAN：清除指定列
            if (column_io >= column_count) {
                mes_out = lfmms_m::err::service_err_priority_matrix_column_out_of_range();
                return;
            }

            size_t byte_idx, bit_idx;
            get_bit_position(row_in, column_io, byte_idx, bit_idx);

            if (!test_bit(byte_idx, bit_idx)) {
                mes_out = lfmms_m::err::service_err_priority_matrix_column_not_occupied();
                return;
            }

            // 清除标记
            set_bit(byte_idx, bit_idx, false);
        }

        mes_out = mes::a_mes();
    }

    // 从末尾释放普通优先级项（用于淘汰）
    void LDCP::free_normal_priority_from_end(
        size_t      free_count,
        mes::a_mes  mes_out
    ) {
        if (free_count == 0) {
            mes_out = mes::a_mes();
            return;
        }

        size_t freed = 0;

        // 从最高行（最低优先级）开始，从后往前扫描
        for (size_t row = row_count - 1; row >= permanent_row_end; row--) {
            for (size_t col = column_count - 1; col < column_count; col--) {  // 无符号循环技巧
                size_t byte_idx, bit_idx;
                get_bit_position(row, col, byte_idx, bit_idx);

                if (test_bit(byte_idx, bit_idx)) {
                    // 释放这个位置
                    set_bit(byte_idx, bit_idx, false);
                    freed++;

                    if (freed >= free_count) {
                        mes_out = mes::a_mes();
                        return;
                    }
                }

                if (col == 0) break;  // 防止无符号下溢
            }

            if (row == permanent_row_end) break;  // 到达常驻行边界
        }

        if (freed < free_count) {
            mes_out = lfmms_m::err::service_err_priority_matrix_not_enough_freeable();
        }
    }

    // 获取是否有普通项被释放（用于唤醒等待）
    void LDCP::get_normal_priority_freed(bool& freed_out) {
        // 这个函数需要结合具体使用场景，比如记录上次释放的位置
        // 简化版本：检查是否有任何普通项被标记
        freed_out = false;

        for (size_t row = permanent_row_end; row < row_count; row++) {
            for (size_t col = 0; col < column_count; col++) {
                size_t byte_idx, bit_idx;
                get_bit_position(row, col, byte_idx, bit_idx);

                if (!test_bit(byte_idx, bit_idx)) {
                    freed_out = true;
                    return;
                }
            }
        }
    }

    // 标记常驻优先级项
    void LDCP::mark_permanent_priority(
        op          op_in,
        size_t& row_io,     // set时输出，clean时输入
        size_t& column_io,  // set时输出，clean时输入
        mes::a_mes  mes_out
    ) {
        // 1. 检查是否启用了常驻优先级
        if (permanent_row_end == 0) {
            mes_out = lfmms_m::err::service_err_priority_matrix_permanent_not_enabled();
            return;
        }

        if (op_in == op::set) {
            // 2. SET：在常驻行中寻找空闲位置

            // 优先从最低的常驻行开始分配（靠近普通区域的行）
            for (size_t row = permanent_row_end - 1; row < permanent_row_end; row--) {
                for (size_t col = 0; col < column_count; col++) {
                    size_t byte_idx, bit_idx;
                    get_bit_position(row, col, byte_idx, bit_idx);

                    if (!test_bit(byte_idx, bit_idx)) {
                        // 找到空闲位置
                        set_bit(byte_idx, bit_idx, true);
                        row_io = row;
                        column_io = col;
                        mes_out = mes::a_mes();
                        return;
                    }
                }

                if (row == 0) break;
            }

            mes_out = lfmms_m::err::service_err_priority_matrix_permanent_full();
        }
        else {  // op::clean
            // 3. CLEAN：清除指定常驻项
            if (row_io >= permanent_row_end) {
                mes_out = lfmms_m::err::service_err_priority_matrix_row_not_permanent();
                return;
            }

            if (column_io >= column_count) {
                mes_out = lfmms_m::err::service_err_priority_matrix_column_out_of_range();
                return;
            }

            size_t byte_idx, bit_idx;
            get_bit_position(row_io, column_io, byte_idx, bit_idx);

            if (!test_bit(byte_idx, bit_idx)) {
                mes_out = lfmms_m::err::service_err_priority_matrix_column_not_occupied();
                return;
            }

            set_bit(byte_idx, bit_idx, false);
            mes_out = mes::a_mes();
        }
    }

    // 获取行优先级使用信息
    void LDCP::get_row_priority_using_info(
        size_t      row_in,
        size_t& using_count_out,
        size_t& free_count_out,
        mes::a_mes  mes_out
    ) {
        if (row_in >= row_count) {
            mes_out = lfmms_m::err::service_err_priority_matrix_row_out_of_range();
            return;
        }

        using_count_out = 0;

        for (size_t col = 0; col < column_count; col++) {
            size_t byte_idx, bit_idx;
            get_bit_position(row_in, col, byte_idx, bit_idx);

            if (test_bit(byte_idx, bit_idx)) {
                using_count_out++;
            }
        }

        free_count_out = column_count - using_count_out;
        mes_out = mes::a_mes();
    }

    // 获取常驻项使用信息
    void LDCP::get_permanent_using_info(
        size_t& using_count_out,
        size_t& free_count_out,
        mes::a_mes  mes_out
    ) {
        if (permanent_row_end == 0) {
            mes_out = lfmms_m::err::service_err_priority_matrix_permanent_not_enabled();
            return;
        }

        using_count_out = 0;
        size_t total_permanent = permanent_row_end * column_count;

        for (size_t row = 0; row < permanent_row_end; row++) {
            for (size_t col = 0; col < column_count; col++) {
                size_t byte_idx, bit_idx;
                get_bit_position(row, col, byte_idx, bit_idx);

                if (test_bit(byte_idx, bit_idx)) {
                    using_count_out++;
                }
            }
        }

        free_count_out = total_permanent - using_count_out;
        mes_out = mes::a_mes();
    }

    // 获取项的优先级（行号越低优先级越高）
    size_t LDCP::get_priority(size_t row, size_t col) const {
        if (row >= row_count || col >= column_count) return SIZE_MAX;

        size_t byte_idx, bit_idx;
        get_bit_position(row, col, byte_idx, bit_idx);

        if (!test_bit(byte_idx, bit_idx)) {
            return SIZE_MAX;  // 未占用
        }

        return row;  // 行号即优先级（0最高）
    }

    // 判断项是否常驻
    bool LDCP::is_permanent(size_t row, size_t col) const {
        if (row >= row_count || col >= column_count) return false;

        size_t byte_idx, bit_idx;
        get_bit_position(row, col, byte_idx, bit_idx);

        if (!test_bit(byte_idx, bit_idx)) {
            return false;  // 未占用
        }

        return row < permanent_row_end;
    }

    // 初始化矩阵
    void LDCP::init(size_t rows, size_t cols, size_t permanent_rows, mes::a_mes& mes_out) {
        if (rows == 0 || cols == 0) {
            mes_out = lfmms_m::err::service_err_priority_matrix_invalid_size();
            return;
        }

        if (permanent_rows > rows) {
            mes_out = lfmms_m::err::service_err_priority_matrix_permanent_rows_exceed_total();
            return;
        }

        row_count         = rows;
        column_count      = cols;
        permanent_row_end = permanent_rows;

        // 分配位图内存
        try {
            priority_matrix_info = make_unique<byte_1[]>(bitmap_size());
        }
        catch (...) {
            mes_out = lfmms_m::err::service_err_priority_matrix_memory_alloc_failed();
            return;
        }

        // 全部初始化为0（未占用）
        memset(priority_matrix_info.get(), 0, bitmap_size());

        mes_out = mes::a_mes();
    }
};

//static_cache_part
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
    #define LDSC logical_volume::data::static_cache::static_cache_part

    LDSC::result LDSC::load_with_status(
        size_t           load_begin_in,
        size_t           load_size_in,
        bmms_f::agent_ptr& save_agent_ptr_in,
        size_t           store_begin_in,
        bool             safe,
        mes::a_mes& mes_out
    ) {
        // 1 初始化校验
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        if (load_size_in == 0) {
            mes_out = mes::a_mes();
            return result::HIT;  // 空操作视为成功
        }

        // 2 范围校验
        check_range(load_begin_in, load_size_in);

        // 3 获取范围信息
        size_t idx_begin, idx_count, idx_end, off_first, size_end;
        bool over;
        get_range_info(load_begin_in, load_size_in,
            idx_begin, idx_count, idx_end,
            off_first, size_end, over);

        // 4 检查范围内是否有空洞
        bool has_hole = false;
        size_t blk_size = get_block_size();

        for (size_t i = idx_begin; i <= idx_end; i = nf_add(i, 1)) {
            void* block_ptr = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                nf_mul(i, 8), 8, &block_ptr, true);

            if (block_ptr == nullptr) {
                has_hole = true;
                break;
            }
        }

        // 5 如果有空洞，返回 MISS
        if (has_hole) {
            mes_out = mes::a_mes();
            return result::MISS;
        }

        // 6 没有空洞，执行实际的 load 操作
        size_t current_store = store_begin_in;
        size_t remaining = load_size_in;

        for (size_t i = idx_begin; i <= idx_end && remaining > 0; i = nf_add(i, 1)) {
            void* block_ptr = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                nf_mul(i, 8), 8, &block_ptr, true);

            size_t block_off = (i == idx_begin) ? off_first : 0;
            size_t bytes_this = blk_size - block_off;
            if (i == idx_end && over) {
                bytes_this = size_end;
            }
            bytes_this = std::min(bytes_this, remaining);

            bmms_f::agent_ptr block_agent(block_ptr, blk_size);
            block_agent.load(block_off, bytes_this,
                save_agent_ptr_in, current_store, safe);

            current_store = nf_add(current_store, bytes_this);
            remaining = nf_sub(remaining, bytes_this);
        }

        mes_out = mes::a_mes();
        return result::HIT;
    }

    // 重写 store 方法 - 不会自动分配，未命中返回 MISS
    LDSC::result LDSC::store_with_status(
        bmms_f::agent_ptr& load_agent_ptr_in,
        size_t             load_begin_in,
        size_t             load_size_in,
        size_t             store_begin_in,
        bool               safe,
        mes::a_mes& mes_out
    ) {
        // 1 初始化校验
        if (!is_init) {
            __CODING_ERROR__(bmms_m::err::coding_err_static_volume_part_not_inited)
        }
        if (load_size_in == 0) {
            mes_out = mes::a_mes();
            return result::HIT;  // 空操作视为成功
        }

        // 2 范围校验
        check_range(store_begin_in, load_size_in);

        // 3 获取范围信息
        size_t idx_begin, idx_count, idx_end, off_first, size_end;
        bool over;
        get_range_info(store_begin_in, load_size_in,
            idx_begin, idx_count, idx_end,
            off_first, size_end, over);

        // 4 检查范围内是否有空洞
        bool has_hole = false;
        size_t blk_size = get_block_size();

        for (size_t i = idx_begin; i <= idx_end; i = nf_add(i, 1)) {
            void* block_ptr = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                nf_mul(i, 8), 8, &block_ptr, true);

            if (block_ptr == nullptr) {
                has_hole = true;
                break;
            }
        }

        // 5 如果有空洞，返回 MISS
        if (has_hole) {
            mes_out = mes::a_mes();
            return result::MISS;
        }

        // 6 没有空洞，执行实际的 store 操作
        size_t current_load = load_begin_in;
        size_t remaining = load_size_in;

        for (size_t i = idx_begin; i <= idx_end && remaining > 0; i = nf_add(i, 1)) {
            void* block_ptr = nullptr;
            block_ptr_ram_space_agent_ptr.load_to_void_ptr(
                nf_mul(i, 8), 8, &block_ptr, true);

            size_t block_off = (i == idx_begin) ? off_first : 0;
            size_t bytes_this = blk_size - block_off;
            if (i == idx_end && over) {
                bytes_this = size_end;
            }
            bytes_this = std::min(bytes_this, remaining);

            bmms_f::agent_ptr block_agent(block_ptr, blk_size);
            block_agent.store(load_agent_ptr_in,
                current_load, bytes_this,
                block_off, safe);

            current_load = nf_add(current_load, bytes_this);
            remaining = nf_sub(remaining, bytes_this);
        }

        mes_out = mes::a_mes();
        return result::HIT;
    }
};

//block_cache_info_vector
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
    #define LDSB logical_volume::data::static_cache::block_cache_info_vector

    // 初始化
    void LDSB::init(size_t count, mes::a_mes& mes_out) {
        entry_count = count;
        size_t byte_count = (count * 4 + 7) / 8;  // 每个条目4bit，向上取整到字节

        try {
            data = make_unique<byte_1[]>(byte_count);
        }
        catch (...) {
            mes_out = lfmms_m::err::service_err_block_cache_vector_memory_alloc_failed();
            return;
        }

        // 全部初始化为0
        memset(data.get(), 0, byte_count);
        mes_out = mes::a_mes();
    }

    // 辅助：获取4bit在字节中的位置
    void LDSB::get_position(size_t index, size_t& byte_idx, size_t& nibble_offset) {
        size_t bit_pos = index * 4;           // 每个条目4bit
        byte_idx = bit_pos / 8;                // 所在字节
        nibble_offset = (bit_pos % 8) / 4;     // 0或1（低4位/高4位）
    }

    // 读取4bit值
    uint8_t LDSB::read_nibble(size_t index) {
        size_t byte_idx, nibble_offset;
        get_position(index, byte_idx, nibble_offset);

        uint8_t byte = data[byte_idx];
        if (nibble_offset == 0) {
            return byte & 0x0F;        // 低4位
        }
        else {
            return (byte >> 4) & 0x0F;  // 高4位
        }
    }

    // 写入4bit值
    void LDSB::write_nibble(size_t index, uint8_t value) {
        size_t byte_idx, nibble_offset;
        get_position(index, byte_idx, nibble_offset);

        uint8_t& byte = data[byte_idx];
        if (nibble_offset == 0) {
            byte = (byte & 0xF0) | (value & 0x0F);        // 保留高4位，更新低4位
        }
        else {
            byte = (byte & 0x0F) | ((value & 0x0F) << 4); // 保留低4位，更新高4位
        }
    }

    // 锁定操作
    void LDSB::lock(op op_in, size_t index, bool& lock_io) {
        if (index >= entry_count) return;

        uint8_t nibble = read_nibble(index);

        if (op_in == op::get) {
            lock_io = (nibble & LOCK_BIT) != 0;
        }
        else {  // set
            if (lock_io) {
                nibble |= LOCK_BIT;
            }
            else {
                nibble &= ~LOCK_BIT;
            }
            write_nibble(index, nibble);
        }
    }

    // 脏标记操作
    void LDSB::dirty(op op_in, size_t index, bool& dirty_io) {
        if (index >= entry_count) return;

        uint8_t nibble = read_nibble(index);

        if (op_in == op::get) {
            dirty_io = (nibble & DIRTY_BIT) != 0;
        }
        else {  // set
            if (dirty_io) {
                nibble |= DIRTY_BIT;
            }
            else {
                nibble &= ~DIRTY_BIT;
            }
            write_nibble(index, nibble);
        }
    }

    // 有效标记操作
    void LDSB::valid(op op_in, size_t index, bool& valid_io) {
        if (index >= entry_count) return;

        uint8_t nibble = read_nibble(index);

        if (op_in == op::get) {
            valid_io = (nibble & VALID_BIT) != 0;
        }
        else {  // set
            if (valid_io) {
                nibble |= VALID_BIT;
            }
            else {
                nibble &= ~VALID_BIT;
            }
            write_nibble(index, nibble);
        }
    }

    // 预留位操作（供未来扩展）
    void LDSB::reserved(op op_in, size_t index, bool& reserved_io) {
        if (index >= entry_count) return;

        uint8_t nibble = read_nibble(index);

        if (op_in == op::get) {
            reserved_io = (nibble & RESERVED) != 0;
        }
        else {  // set
            if (reserved_io) {
                nibble |= RESERVED;
            }
            else {
                nibble &= ~RESERVED;
            }
            write_nibble(index, nibble);
        }
    }

    // 批量获取状态
    void LDSB::get_all(size_t index, bool& locked, bool& dirty, bool& valid, bool& reserved) {
        if (index >= entry_count) {
            locked = dirty = valid = reserved = false;
            return;
        }

        uint8_t nibble = read_nibble(index);
        locked = (nibble & LOCK_BIT) != 0;
        dirty = (nibble & DIRTY_BIT) != 0;
        valid = (nibble & VALID_BIT) != 0;
        reserved = (nibble & RESERVED) != 0;
    }

    // 批量设置状态
    void LDSB::set_all(size_t index, bool locked, bool dirty, bool valid, bool reserved) {
        if (index >= entry_count) return;

        uint8_t nibble = 0;
        if (locked)  nibble |= LOCK_BIT;
        if (dirty)   nibble |= DIRTY_BIT;
        if (valid)   nibble |= VALID_BIT;
        if (reserved)nibble |= RESERVED;

        write_nibble(index, nibble);
    }

    // 清空单个条目
    void LDSB::clear(size_t index) {
        if (index >= entry_count) return;
        write_nibble(index, 0);
    }

    // 清空所有条目
    void LDSB::clear_all() {
        size_t byte_count = (entry_count * 4 + 7) / 8;
        memset(data.get(), 0, byte_count);
    }

    // 获取统计信息
    void LDSB::get_stats(size_t& used_count, size_t& dirty_count, size_t& locked_count) {
        used_count = 0;
        dirty_count = 0;
        locked_count = 0;

        for (size_t i = 0; i < entry_count; i++) {
            uint8_t nibble = read_nibble(i);

            if (nibble & VALID_BIT) used_count++;
            if (nibble & DIRTY_BIT) dirty_count++;
            if (nibble & LOCK_BIT)  locked_count++;
        }
    }

};

//static_cache
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
    #define LDSS logical_volume::data::static_cache::static_cache

};