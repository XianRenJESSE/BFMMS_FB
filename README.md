# 📦 File Memory Management System Family Bucket

## 📋 Feature Overview

This is a **C++20 modular system that unifies memory and file operation semantics**, providing a complete solution from memory pool allocation to file cache management.

### Core Features
- **Unified Operation Interface**: Memory operations and file operations use exactly the same function signatures
- **Zero-Fragmentation Memory Pools**: Two allocation strategies - static volumes (fixed size) and dynamic volumes (variable size)
- **File Cache Proxy**: Maps file regions as memory views, enabling zero-copy access
- **Forced Error Consumption**: Unchecked errors/warnings prevent result retrieval
- **Region Views**: Logical sub-views of physical resources, supporting local mapping of memory and files

---

## ✨ Advantages

### 1. Unified Semantics, Reduced Mental Load
```cpp
// Memory operation
agent_ptr.load(0, 1024, another_ptr, 0, true);

// File operation (completely identical syntax)
file_agent_ptr.load(0, 1024, another_fap, 0, true, mes);
```
**Learn one, know them all.**

### 2. Zero External Fragmentation Memory Pools
- **static_volume**: Fixed block size, immediate reuse upon release, fragmentation impossible
- **free_volume**: Dynamic allocation, supports returning physical memory to the system
- **static_volume_part**: Multiple logical partitions sharing the same physical memory pool

### 3. Zero-Copy File Caching
File caches are directly managed by the memory pool and can be borrowed by the business layer:
```cpp
// File cache IS an agent_ptr
bmms_f::agent_ptr& cache = file_agent_ptr.get_cache_agent_ptr();
// Direct memory operations, no second copy needed
```

### 4. Explicit Control, No Black-Box Behavior
- Explicit dirty page marking: `__mark_dirty__()`
- Explicit flush to disk: `push_cache_to_bin_file()`
- Explicit pull: `pull_cache_from_bin_file()`

### 5. Forced Error Consumption Ensuring Business Safety
```cpp
res.set_err(error, true);  // Mark as "must check"
// ... forgot to check ...
auto result = res.move_result();  // Triggers coding error, process terminates
```

### 6. Regions as Views, Physical-Logical Separation
- **static_volume_part**: Logical sub-view of memory regions
- **file_agent_ptr**: Logical sub-view of file regions
- Multiple independent logical regions can be carved from the same physical resource

---

## 🚀 Quick Start

### Environment Requirements
- C++20 Modules support (MSVC 2022 17.5+ / Clang 17+ / GCC 14+)
- Standard library headers: `<fstream>`, `<memory>`, `<bitset>`, etc.

# 📦 File Memory Management System Family Bucket —— Use Case Collection

## Case 1: Type-Safe Data Persistence (Simplest Mode)

```cpp
import MES;
import BMMS;
import BFS;

mes::a_mes mes;

// ===== 1. Open file =====
auto file = std::make_shared<bfs_f::bin_file>(
    "config.bin", 1024,
    bfs_f::bin_file::init_type::open_existed,
    mes
);

// ===== 2. Define typed data =====
struct Config {
    int version = 1;
    double scale = 1.5;
    char name[32] = "default";
    
    void upgrade() { version++; }
};

Config cfg;

// ===== 3. Create agent pointer wrapping typed data =====
bmms_f::agent_ptr cfg_agent(&cfg, sizeof(Config));

// ===== 4. Load from file to typed data =====
file->load_to_agent_ptr(0, sizeof(Config), cfg_agent, 0, true, mes);

// ===== 5. Business logic: directly operate on type =====
cfg.upgrade();
cfg.scale = 2.0;

// ===== 6. Save back to file =====
file->store_from_agent_ptr(cfg_agent, 0, sizeof(Config), 0, true, true, mes);
```

**Semantics**: The `Config` object directly serves as the data carrier; the agent pointer is just a "view," and business code operates on the type itself.

---

## Case 2: Memory Pool + File Cache (Zero Copy)

```cpp
import MES;
import BMMS;
import BFS;

mes::a_mes mes;

// ===== 1. Create memory pool =====
auto vol = std::make_shared<bmms_f::static_volume>(
    4096, 100,  // 4KB blocks, 100 blocks
    bmms_f::static_volume::init_type::no_init_all_size,
    bmms_f::static_volume::alignment_type::byte_8,
    mes
);

// ===== 2. Allocate a block from the pool =====
bmms_f::static_volume_ptr block(vol, mes);

// ===== 3. Open file =====
auto file = std::make_shared<bfs_f::bin_file>(
    "data.bin", 1024 * 1024,
    bfs_f::bin_file::init_type::open_existed,
    mes
);

// ===== 4. Bind the memory block as a file cache region =====
bfs_f::file_agent_ptr cache(
    file,           // File
    8192,           // Start from file offset 8192
    4096,           // Cache size 4KB
    block.get_agent_ptr(),  // Use memory allocated from the pool
    mes
);

// ===== 5. Define business data structure =====
struct Packet {
    uint64_t id;
    uint32_t type;
    uint8_t data[4064];  // Remaining space
};

Packet pkt;

// ===== 6. Load from file to business object =====
bmms_f::agent_ptr pkt_agent(&pkt, sizeof(Packet));
cache.load_to_agent_ptr(0, sizeof(Packet), pkt_agent, 0, true, mes);

// ===== 7. Business processing =====
pkt.id++;
pkt.type = 2;

// ===== 8. Write back to cache (automatically marks dirty pages) =====
cache.store_from_agent_ptr(pkt_agent, 0, sizeof(Packet), 0, true, mes);

// ===== 9. Flush back to disk =====
cache.push_cache_to_bin_file(mes);
```

**Semantics**: Memory pool, file cache, and business objects are separated, connected through agent pointers, with clear data flow.

---

## Case 3: `static_volume_part` Lightweight Logical Partitioning

```cpp
import MES;
import BMMS;

mes::a_mes mes;

// ===== 1. Create physical memory pool =====
auto phys_vol = std::make_shared<bmms_f::static_volume>(
    64, 1000,  // 64-byte blocks, 1000 blocks
    bmms_f::static_volume::init_type::init_all_size,
    bmms_f::static_volume::alignment_type::byte_8,
    mes
);

// ===== 2. Create three logical partitions sharing the same physical pool =====
bmms_f::static_volume_part session_pool(
    phys_vol, 200,  // Session pool with 200 blocks
    bmms_f::static_volume_part::init_type::no_init_all_size,
    bmms_f::static_volume_part::control_type::not_allow_over_use,
    mes
);

bmms_f::static_volume_part cache_pool(
    phys_vol, 500,  // Cache pool with 500 blocks
    bmms_f::static_volume_part::init_type::no_init_all_size,
    bmms_f::static_volume_part::control_type::not_allow_over_use,
    mes
);

bmms_f::static_volume_part temp_pool(
    phys_vol, 300,  // Temporary pool with 300 blocks
    bmms_f::static_volume_part::init_type::no_init_all_size,
    bmms_f::static_volume_part::control_type::allow_over_use,  // Allows overuse!
    mes
);

// ===== 3. Define business data =====
struct UserSession {
    uint64_t uid;
    char name[32];
    time_t login_time;
};

UserSession session;

// ===== 4. Allocate space in session pool =====
bmms_f::agent_ptr session_agent(&session, sizeof(UserSession));
session_pool.store_from_void_ptr(&session, sizeof(UserSession), 0, true, mes);

// ===== 5. Release block after use (creates hole) =====
session_pool.free_block(0);  // Block index 0 is released

// ===== 6. Temporary pool overuse example =====
struct BigData {
    char data[1024];
};

BigData big;

// The temporary pool allows overuse. Although only 100 blocks remain in the 
// physical pool (since session+cache used 700), you can logically allocate 
// 200 blocks. Physical space is only checked when actually writing.
for(int i = 0; i < 150; i++) {
    temp_pool.store_from_void_ptr(&big, sizeof(BigData), i * 1024, true, mes);
    // First 100 succeed; from 101 onward, service errors are triggered
}
```

**Semantics**: Physical memory is shared, logical views are isolated. `allow_over_use` lets logical space exceed physical space, suitable for temporary buffers.

---

## Case 4: Hole Utilization and Dynamic Resizing

```cpp
import MES;
import BMMS;

mes::a_mes mes;

auto vol = std::make_shared<bmms_f::static_volume>(
    1024, 100,
    bmms_f::static_volume::init_type::init_all_size,
    bmms_f::static_volume::alignment_type::byte_8,
    mes
);

bmms_f::static_volume_part part(
    vol, 50,
    bmms_f::static_volume_part::init_type::no_init_all_size,
    bmms_f::static_volume_part::control_type::not_allow_over_use,
    mes
);

// ===== 1. Allocate some blocks =====
struct Record {
    int id;
    double value;
};

Record rec;

for(int i = 0; i < 30; i++) {
    part.store_from_void_ptr(&rec, sizeof(Record), i * sizeof(Record), true, mes);
}

// ===== 2. Release a middle block (creates a hole) =====
part.free_block(15);  // Block 15 is released

// ===== 3. The hole is automatically reused =====
Record new_rec;
part.store_from_void_ptr(&new_rec, sizeof(Record), 15 * sizeof(Record), true, mes);
// Block 15 is automatically reallocated from the physical pool

// ===== 4. Resize partition (shrink) =====
part.resize(40, mes);  // Last 10 blocks are returned to the physical pool

// ===== 5. Resize partition (expand) =====
part.resize(60, mes);  // Newly added 20 blocks are initially unallocated
```

**Semantics**: Hole creation and filling are transparent to the upper layer; `resize` dynamically adjusts the size of logical space.

---

## Case 5: Complete Pipeline with File + Memory Pool + Business Objects

```cpp
import MES;
import BMMS;
import BFS;
import <vector>;

mes::a_mes mes;

// ===== 1. Infrastructure =====
auto file = std::make_shared<bfs_f::bin_file>(
    "database.bin", 10 * 1024 * 1024,
    bfs_f::bin_file::init_type::open_existed,
    mes
);

auto pool = std::make_shared<bmms_f::static_volume>(
    512, 1000,
    bmms_f::static_volume::init_type::no_init_all_size,
    bmms_f::static_volume::alignment_type::byte_8,
    mes
);

// ===== 2. Business data definition =====
struct Entry {
    uint64_t key;
    uint32_t flags;
    uint8_t data[500];
    
    void update() { flags |= 0x01; }
};

// ===== 3. Process a batch of data =====
std::vector<Entry> entries(10);

for(size_t i = 0; i < entries.size(); i++) {
    // 3.1 Allocate block from pool
    bmms_f::static_volume_ptr block(pool, mes);
    
    // 3.2 Bind block as file region
    bfs_f::file_agent_ptr region(
        file, i * 1024, 512,
        block.get_agent_ptr(),
        mes
    );
    
    // 3.3 Load from file to block
    region.pull_cache_from_bin_file(mes);
    
    // 3.4 Load from block to business object
    bmms_f::agent_ptr entry_agent(&entries[i], sizeof(Entry));
    block.get_agent_ptr().load_to_agent_ptr(
        0, sizeof(Entry), entry_agent, 0, true, mes
    );
    
    // 3.5 Business processing
    entries[i].update();
    
    // 3.6 Write back to block
    block.get_agent_ptr().store_from_agent_ptr(
        entry_agent, 0, sizeof(Entry), 0, true, mes
    );
    
    // 3.7 Flush back to file
    region.push_cache_to_bin_file(mes);
    
    // 3.8 Block automatically returned (static_volume_ptr destructor)
}
```

**Semantics**: Each `Entry` is processed independently, resources are managed automatically, and the data flow path is clear and traceable.

---

## Summary of Key Design Points

1. **Type Safety**: Business code operates on `T`; agent pointers are just bridges
2. **Unified Semantics**: `load/store` family of functions have completely identical signatures
3. **Explicit Control**: No implicit flushing, no implicit allocation
4. **Zero Copy**: File caches directly use pool memory
5. **Logical Isolation**: `part` provides lightweight views, supports holes and overuse
6. **Forced Error Consumption**: `mes::res` ensures errors are handled

```cpp
// Typical data flow pattern
T data;
bmms_f::agent_ptr agent(&data, sizeof(T));

// File → agent → data
file.load_to_agent_ptr(offset, sizeof(T), agent, 0, true, mes);
data.modify();
file.store_from_agent_ptr(agent, 0, sizeof(T), offset, true, true, mes);

// Or
cache.load_to_agent_ptr(offset, sizeof(T), agent, 0, true, mes);
data.modify();
cache.store_from_agent_ptr(agent, 0, sizeof(T), offset, true, mes);
cache.push_cache_to_bin_file(mes);
```

### Example 3: Result Package with Error Checking
```cpp
import MES;

mes::res<std::vector<int>> res({1, 2, 3, 4, 5});

// Set an error that must be checked
mes::a_mes error = /* some error */;
res.set_err(error, true);  // true = must check

// Correct approach: check error first
if (res.has_err()) {
    mes::a_mes e = res.get_err();  // Consume the error
    e.out();
}

// Now it's safe to get the result
auto vec = res.move_result();
```
# The "Feel-Good" Points

## 🎯 Point 1: Type Safety Preserved, Semantics Unified

```cpp
// Ordinary C++ object, use it normally
Config cfg;
cfg.version = 2;  // Direct member access

// Agent pointer is just a "view," doesn't interfere with business logic
bmms_f::agent_ptr cfg_agent(&cfg, sizeof(Config));

// File read/write, semantics completely identical
file.load_to_agent_ptr(0, sizeof(Config), cfg_agent, 0, true, mes);
// Business code as usual
cfg.upgrade();
file.store_from_agent_ptr(cfg_agent, 0, sizeof(Config), 0, true, true, mes);
```

**Comparison with traditional solutions**:
- ❌ mmap: Need pointer arithmetic, type unsafe
- ❌ Serialization: Need to write to_bytes/from_bytes functions
- ❌ fread/fwrite: Need to calculate offsets and sizes manually
- ✅ This solution: Objects remain objects, file operations are "bonus"

---

## 🎯 Point 2: Memory Pool and File Cache, Identical Syntax

```cpp
// Memory pool allocation
bmms_f::static_volume_ptr block(vol, mes);

// File region view
bfs_f::file_agent_ptr cache(file, offset, size, block.get_agent_ptr(), mes);

// Load data to object
cache.load_to_agent_ptr(0, sizeof(T), obj_agent, 0, true, mes);

// Modify object
obj.update();

// Write back to cache
cache.store_from_agent_ptr(obj_agent, 0, sizeof(T), offset, true, mes);

// Flush to disk
cache.push_cache_to_bin_file(mes);
```

**Why it feels good?** — **No mental context switching**. Whether file or memory, the code looks exactly the same.

---

## 🎯 Point 3: `part` Makes Logical Space Seemingly Infinite

```cpp
// Physically only 1000 blocks
auto phys = std::make_shared<bmms_f::static_volume>(64, 1000, ...);

// But you can have three "virtual views"
bmms_f::static_volume_part session(phys, 2000, allow_over_use, ...);
bmms_f::static_volume_part cache(phys, 5000, allow_over_use, ...);
bmms_f::static_volume_part temp(phys, 3000, allow_over_use, ...);

// Writing code, you don't think about physical memory at all
for(int i = 0; i < 2000; i++) {
    session.store_from_void_ptr(&data, 64, i*64, true, mes);
    // First 1000 succeed, from 1001 onward returns "insufficient physical memory" error
}
```

**Why it feels good?** — **Physical and logical decoupling**. You can design the logical architecture first; physical insufficiency will naturally report errors, instead of calculating everything upfront.

---

## 🎯 Point 4: Automatic Hole Management, Transparent to Upper Layer

```cpp
// Allocate a bunch
for(int i = 0; i < 100; i++) {
    part.store_from_void_ptr(&data, 64, i*64, true, mes);
}

// Release some in the middle
part.free_block(30);
part.free_block(31);
part.free_block(32);

// Later ones automatically reuse the holes
part.store_from_void_ptr(&new_data, 64, 30*64, true, mes);  // Automatically reallocated

// No need to maintain a free list at all
```

**Why it feels good?** — **Memory management is infrastructure's job**; business code only needs `store` and `free_block`.

---

## 🎯 Point 5: Errors Must Be Consumed, Otherwise No Result

```cpp
mes::res<std::vector<int>> res({1,2,3});

res.set_err(some_error, true);  // Marked as must-check

// Forgot to check?
auto vec = res.move_result();  // Process terminates! MODC:6 MESC:...

// Correct approach
if(res.has_err()) {
    auto e = res.get_err();  // Consume error
    e.out();
}
auto vec = res.move_result();  // Safe
```

**Why it feels good?** — **Errors don't slip through unnoticed**. Either handle them explicitly, or the program dies.

---

## 🎯 Point 6: Automatic Resource Lifetime Management

```cpp
{
    auto vol = std::make_shared<bmms_f::static_volume>(...);
    
    {
        bmms_f::static_volume_ptr block(vol, mes);  // Allocate block
        bfs_f::file_agent_ptr cache(file, offset, size, block.get_agent_ptr(), mes);
        
        // ... do work ...
        
    }  // block destructor → block automatically returned to pool
      // cache destructor → automatically unbinds (dirty pages? auto-flush, but doesn't wait for result)
    
}  // vol's last shared_ptr destructor → physical memory released
```

**Why it feels good?** — **The power of RAII**. Allocation, release, binding, unbinding, all automatic.

---

## 🎯 Point 7: Data Flow Path is Crystal Clear

```cpp
// Complete path of data from disk to register, each line is clear

// 1. File → Cache
cache.pull_cache_from_bin_file(mes);

// 2. Cache → Business Object
cache.load_to_agent_ptr(offset, sizeof(T), obj_agent, 0, true, mes);

// 3. Business Object → Register
obj_agent.load_to_void_ptr(field_offset, sizeof(field), &reg, true, mes);

// 4. Modify Register
reg++;

// 5. Register → Business Object
obj_agent.store_from_void_ptr(&reg, sizeof(field), field_offset, true, mes);

// 6. Business Object → Cache
cache.store_from_agent_ptr(obj_agent, 0, sizeof(T), offset, true, mes);

// 7. Cache → File
cache.push_cache_to_bin_file(mes);
```

**Why it feels good?** — **No black boxes**. How data flows, where it flows, who modified it — all visible.

---

## 🎯 Point 8: Template Freedom, Compile-Time Polymorphism

```cpp
template<typename T>
void process_object(bfs_f::file_agent_ptr& cache, size_t offset, T& obj) {
    bmms_f::agent_ptr obj_agent(&obj, sizeof(T));
    
    cache.load_to_agent_ptr(offset, sizeof(T), obj_agent, 0, true, mes);
    obj.process();  // T's own methods
    cache.store_from_agent_ptr(obj_agent, 0, sizeof(T), offset, true, mes);
}

// Any type can be used
Config cfg;
Packet pkt;
Record rec;

process_object(cache, 0, cfg);
process_object(cache, 1024, pkt);
process_object(cache, 2048, rec);
```

**Why it feels good?** — **True generic programming**. One set of code serves all types.

---

## Summary

The "feel-good" points of this system are:

1. **Type Safety**: Business code operates on real objects, not byte arrays
2. **Unified Semantics**: Memory and file operations look identical, reducing cognitive load
3. **Logical Isolation**: `part` lets you design at the logical level; physical insufficiency reports errors
4. **Automatic Management**: Holes, lifetimes, error consumption — all infrastructure's job
5. **Clear Path**: Every step of data flow is explicitly written, no implicit magic
6. **Forced Safety**: Errors must be consumed, otherwise no results

**The feeling of writing code with this system**: Like writing ordinary C++, but with "bonus" features like memory pools, file caching, zero copy, and error checking. Where objects should be, they are objects; where files should be, they are files — but the methods to operate them are **exactly the same**.

**It feels great!**

---

## 📚 Public Method Overview

### BMMS Module

#### `agent_ptr` Class
| Method | Description |
|------|------|
| `agent_ptr(void* ptr, size_t size)` | Construct agent pointer from raw pointer and size |
| `load(load_begin, load_size, save_agent, store_begin, safe)` | Read data from this region into another agent pointer |
| `store(load_agent, load_begin, load_size, store_begin, safe)` | Read data from another agent pointer into this region |
| `load_to_void_ptr(load_begin, load_size, dest_ptr, safe)` | Read data from this region into raw pointer |
| `store_from_void_ptr(src_ptr, load_size, store_begin, safe)` | Read data from raw pointer into this region |
| `clear()` | Clear entire region |
| `clear_range(begin, size)` | Clear specified range |
| `get_void_ptr(ptr_out)` | Get raw pointer |
| `get_is_null(is_null_out)` | Check if null |
| `get_size(size_out)` | Get region size |
| `get_begin(begin_out)` | Get start address |
| `get_end(end_out)` | Get end address |
| `get_check_range_bool(begin, size, is_valid_out)` | Check if range is valid |

#### `static_volume` Class
| Method | Description |
|------|------|
| `static_volume(block_size, block_count, init_type, alignment, mes_out)` | Construct static memory volume |
| `new_block(agent_ptr_out, mes_out)` | Allocate a new block |
| `delete_block(agent_ptr_in)` | Return block to recycle stack |
| `get_is_init()` | Check if initialized |
| `get_block_count_max()` | Get maximum number of blocks |
| `get_free_block_count()` | Get number of free blocks |
| `get_block_size()` | Get block size |

#### `static_volume_ptr` Class (RAII Block Pointer)
| Method | Description |
|------|------|
| `static_volume_ptr(volume_shared_ptr, mes_out)` | Construct from static volume, automatically allocates a block |
| `clear()` | Clear entire block |
| `clear_range(begin, size)` | Clear specified range within block |
| `load(...)` | Same semantics as agent_ptr |
| `store(...)` | Same semantics as agent_ptr |
| `load_to_void_ptr(...)` | Same semantics as agent_ptr |
| `store_from_void_ptr(...)` | Same semantics as agent_ptr |
| `get_agent_ptr()` | Get underlying agent pointer |

#### `static_volume_part` Class (Static Volume Logical Partition)
| Method | Description |
|------|------|
| `static_volume_part(volume, block_count, init_type, control_type, mes_out)` | Create logical partition of static volume |
| `clear()` | Clear entire partition |
| `clear_range(begin, size)` | Clear specified range within partition |
| `resize(block_count, mes_out)` | Resize partition |
| `free_block(block_index)` | Release specified block (creates hole) |
| `free_range(begin, size)` | Release range of blocks |
| `load(...)` | Same semantics as agent_ptr |
| `store(...)` | Same semantics as agent_ptr |
| `load_to_void_ptr(...)` | Same semantics as agent_ptr |
| `store_from_void_ptr(...)` | Same semantics as agent_ptr |
| `get_total_size()` | Get total partition size (bytes) |
| `get_block_count_max()` | Get maximum number of blocks |
| `get_block_size()` | Get block size |
| `get_block_count_used()` | Get number of used blocks |
| `get_block_ptr(index, agent_out, is_null_out)` | Get agent pointer for specified block |

#### `free_volume` Class (Dynamic Memory Volume)
| Method | Description |
|------|------|
| `free_volume(block_size, block_count, init_type, alignment, mes_out)` | Construct dynamic memory volume |
| `new_block(agent_ptr_out, index_out, mes_out)` | Allocate a new block |
| `delete_block(agent_ptr_in, index_in)` | Release block |
| `free_unused_block(index)` | Release physical memory of free block |
| `free_range(begin, size)` | Release physical memory of all free blocks in range |
| `free_all_unused_blocks()` | Release physical memory of all free blocks |
| `get_is_init()` | Check if initialized |
| `get_block_count_max()` | Get maximum number of blocks |
| `get_block_count_used()` | Get number of used blocks |
| `get_free_block_count()` | Get number of free blocks |
| `get_block_size()` | Get block size |

#### `free_volume_ptr` Class (Dynamic Volume RAII Pointer)
| Method | Description |
|------|------|
| `free_volume_ptr(volume_shared_ptr, mes_out)` | Construct from dynamic volume, automatically allocates a block |
| `clear()` | Clear entire block |
| `clear_range(begin, size)` | Clear specified range within block |
| `load(...)` | Same semantics as agent_ptr |
| `store(...)` | Same semantics as agent_ptr |
| `load_to_void_ptr(...)` | Same semantics as agent_ptr |
| `store_from_void_ptr(...)` | Same semantics as agent_ptr |
| `get_index()` | Get block index |
| `get_agent_ptr()` | Get underlying agent pointer |

#### `free_part` Class (Free Block Logical Partition)
| Method | Description |
|------|------|
| `free_part(block_size, block_count, init_type, mes_out)` | Create free block logical partition |
| `clear()` | Clear entire partition |
| `clear_range(begin, size)` | Clear specified range within partition |
| `resize(block_count, mes_out)` | Resize partition |
| `free_block(index)` | Release specified block |
| `free_range(begin, size)` | Release range of blocks |
| `load(...)` | Same semantics as agent_ptr |
| `store(...)` | Same semantics as agent_ptr |
| `load_to_void_ptr(...)` | Same semantics as agent_ptr |
| `store_from_void_ptr(...)` | Same semantics as agent_ptr |
| `get_total_size()` | Get total partition size |
| `get_block_count_max()` | Get maximum number of blocks |
| `get_block_size()` | Get block size |
| `get_block_count_used()` | Get number of used blocks |
| `get_block_ptr(index, agent_out, is_null_out)` | Get agent pointer for specified block |

---

### BFS Module

#### `bin_file` Class (Binary File)
| Method | Description |
|------|------|
| `bin_file(path, size, init_type, mes_out)` | Construct binary file object |
| `open(path, size, init_type, mes_out)` | Open file |
| `flush()` | Flush buffer |
| `close(mes_out)` | Close file |
| `resize(size, mes_out)` | Resize file |
| `clear(mes_out)` | Clear entire file |
| `clear_range(begin, size, mes_out)` | Clear specified range in file |
| `save_as(path, mes_out)` | Save as |
| `load(load_begin, load_size, dest_file, store_begin, safe, flush, mes_out)` | Read from this file and write to another file |
| `store(src_file, load_begin, load_size, store_begin, safe, flush, mes_out)` | Read from another file and write to this file |
| `load_to_agent_ptr(load_begin, load_size, dest_agent, store_begin, safe, mes_out)` | Read from file to agent pointer |
| `store_from_agent_ptr(src_agent, load_begin, load_size, store_begin, safe, flush, mes_out)` | Read from agent pointer and write to file |
| `load_to_void_ptr(load_begin, load_size, dest_ptr, safe, mes_out)` | Read from file to raw pointer |
| `store_from_void_ptr(src_ptr, load_size, store_begin, safe, mes_out)` | Read from raw pointer and write to file |
| `__file_size__()` | Get file size |
| `__self_begin__()` | Get file start offset (0) |
| `__self_end__()` | Get file end offset |

#### `file_agent_ptr` Class (File Cache Proxy)
| Method | Description |
|------|------|
| `file_agent_ptr(bin_file, begin, size, cache_agent, mes_out)` | Construct file region view |
| `clear(mes_out)` | Clear entire cache |
| `clear_range(begin, size, mes_out)` | Clear specified range in cache |
| `push_cache_to_bin_file(mes_out)` | Flush dirty cache back to file |
| `pull_cache_from_bin_file(mes_out)` | Load data from file to cache |
| `load(load_begin, load_size, dest_fap, store_begin, safe, mes_out)` | Read from this cache to another file agent |
| `store(src_fap, load_begin, load_size, store_begin, safe, mes_out)` | Read from another file agent to this cache |
| `load_to_agent_ptr(load_begin, load_size, dest_agent, store_begin, safe, mes_out)` | Read from cache to agent pointer |
| `store_from_agent_ptr(src_agent, load_begin, load_size, store_begin, safe, mes_out)` | Read from agent pointer to cache |
| `load_to_void_ptr(load_begin, load_size, dest_ptr, safe, mes_out)` | Read from cache to raw pointer |
| `store_from_void_ptr(src_ptr, load_size, store_begin, safe, mes_out)` | Read from raw pointer to cache |
| `get_is_inited(is_inited_out)` | Get initialization status |
| `get_cache_begin_about_bin_file(begin_out)` | Get cache's start offset relative to file |
| `get_cache_agent_ptr(agent_out)` | Get underlying cache agent pointer |
| `get_cache_raw_ptr(ptr_out)` | Get cache raw pointer |
| `get_cache_size(size_out)` | Get cache size |
| `get_dirty_flag(is_dirty_out)` | Get dirty page flag |
| `get_associated_bin_file_shared_ptr(file_out)` | Get associated file object |
| `get_managed_file_size(size_out, mes_out)` | Get managed file region size |
| `get_cache_used_size(size_out, mes_out)` | Get used cache size |
| `check_file_still_valid(is_valid_out, mes_out)` | Check if associated file is still valid |
| `calculate_cache_position(file_pos, cache_pos_out, is_mappable_out, mes_out)` | Map file position to cache position |
| `calculate_file_position(cache_pos, file_pos_out, is_mappable_out, mes_out)` | Map cache position back to file position |

#### Directory Control (`dir_control` namespace)
| Method | Description |
|------|------|
| `add_dir(path, mes_out)` | Create directory |
| `del(path, mes_out)` | Delete file or directory |
| `get_dir_list(path, dir_list_out, mes_out)` | Get subdirectory list |
| `get_list_file(path, file_list_out, mes_out)` | Get file list |
| `get_path_is_excited(path, is_excited_out, mes_out)` | Check if path exists |
| `get_is_dir(path, is_dir_out, mes_out)` | Check if directory |
| `get_is_file(path, is_file_out, mes_out)` | Check if file |
| `get_accessible(path, accessible_out, mes_out)` | Check if path is accessible |
| `rename(old_path, new_path, mes_out, overwrite)` | Rename file or path |

---

### MES Module

#### `a_mes` Class (Message)
| Method | Description |
|------|------|
| `a_mes(mode_code, line, message)` | Construct message |
| `out()` | Output message |

#### `res<T>` Class (Result Package with Messages)
| Method | Description |
|------|------|
| `res(const T& result)` | Construct from result |
| `res(T& result, bool move_data)` | Construct from result (optional move) |
| `clear()` | Clear package content |
| `set_time_or_temp(time)` | Set time/temporary flag |
| `set_sug(mes, must_chk)` | Set suggestion (must-check flag) |
| `set_war(war, must_chk)` | Set warning |
| `set_err(err, must_chk)` | Set error |
| `set_dynamic_mes(mes, must_chk)` | Set dynamic message |
| `set_call_chain(file, func, line)` | Set call chain |
| `freeze()` | Freeze (prevent further message setting) |
| `unfreeze()` | Unfreeze |
| `has_freeze()` | Check if frozen |
| `has_sug()` | Check if has suggestion |
| `has_war()` | Check if has warning |
| `has_err()` | Check if has error |
| `has_dynamic_mes()` | Check if has dynamic message |
| `has_call_chain()` | Check if has call chain |
| `get_sug_must_chk()` | Get suggestion must-check flag |
| `get_war_must_chk()` | Get warning must-check flag |
| `get_err_must_chk()` | Get error must-check flag |
| `get_dynamic_mes_must_chk()` | Get dynamic message must-check flag |
| `get_time_or_temp()` | Get time/temporary flag |
| `get_sug()` | Get suggestion (consumes must_chk) |
| `get_war()` | Get warning |
| `get_err()` | Get error |
| `get_dynamic_mes()` | Get dynamic message |
| `get_call_chain_layer_depth()` | Get call chain depth |
| `get_call_chain(layer)` | Get call chain for specified layer |
| `move_result()` | Move result (must consume all must_chk) |
| `get_result()` | Get copy of result |
