#pragma once

//// Platform //////////////////////////////////////////////////////////////////
#if defined(TARGET_OS_LINUX)
	#define TARGET_OS_NAME "Linux"
	#define _XOPEN_SOURCE 800
#elif defined(TARGET_OS_WINDOWS)
	#define TARGET_OS_NAME "Windows"
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <stdlib.h>
#else
	#error "Platform macro `TARGET_OS_*` is not defined, this means you probably forgot to define it or this platform is not suported."
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdalign.h>
#include <tgmath.h>
#include <limits.h>
#include <float.h>

#include <atomic>
#include <new>

//// Essentials ////////////////////////////////////////////////////////////////
#define null NULL

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using uint = unsigned int;
using byte = uint8_t ;

using isize = ptrdiff_t;
using usize = size_t;

using uintptr = uintptr_t;

using f32 = float;
using f64 = double;

using cstring = char const *;

// Swap bytes around, useful for when dealing with endianess
template<typename T>
void swap_bytes(T* data){
	isize len = sizeof(T);
	for(isize i = 0; i < (len / 2); i += 1){
		byte temp = data[i];
		data[i] = data[len - (i + 1)];
		data[len - (i + 1)] = temp;
	}
}

template<typename T>
T abs(T x){
	return (x < static_cast<T>(0)) ? - x : x;
}

template<typename T>
T min(T a, T b){ return a < b ? a : b; }

template<typename T, typename ...Args>
T min(T a, T b, Args... rest){
	if(a < b)
		return min(a, rest...);
	else
		return min(b, rest...);
}

template<typename T>
T max(T a, T b){
	return a > b ? a : b;
}

template<typename T, typename ...Args>
T max(T a, T b, Args... rest){
	if(a > b)
		return max(a, rest...);
	else
		return max(b, rest...);
}

template<typename T>
T clamp(T lo, T x, T hi){
	return min(max(lo, x), hi);
}

static_assert(sizeof(f32) == 4 && sizeof(f64) == 8, "Bad float size");
static_assert(sizeof(isize) == sizeof(usize), "Mismatched (i/u)size");
static_assert(sizeof(void(*)(void)) == sizeof(void*), "Function pointers and data pointers must be of the same width");
static_assert(sizeof(void(*)(void)) == sizeof(uintptr), "Mismatched pointer types");
static_assert(CHAR_BIT == 8, "Invalid char size");

//// Source Location ///////////////////////////////////////////////////////////
typedef struct Source_Location Source_Location;

struct Source_Location {
    cstring filename;
    cstring caller_name;
    i32 line;
};

#define this_location() this_location_()

#define this_location_() (Source_Location){ \
    .filename = __FILE__, \
    .caller_name = __func__, \
    .line = __LINE__, \
}

//// Assert ////////////////////////////////////////////////////////////////////
// Crash if `pred` is false, this is disabled in non-debug builds
void debug_assert_ex(bool pred, cstring msg, Source_Location loc);

#if defined(NDEBUG) || defined(RELEASE_MODE)
#define debug_assert(Pred, Msg) ((void)0)
#else
#define debug_assert(Pred, Msg) debug_assert_ex(Pred, Msg, this_location())
#endif

// Crash if `pred` is false, this is always enabled
void panic_assert_ex(bool pred, cstring msg, Source_Location loc);

#define panic_assert(Pred, Msg) panic_assert_ex(Pred, Msg, this_location())

// Crash the program with a fatal error
[[noreturn]] void panic(cstring msg);

// Crash the program due to unimplemented code paths, this should *only* be used
// during development
[[noreturn]] void unimplemented();

//// Atomic ////////////////////////////////////////////////////////////////////
// Mostly just boilerplate around C++'s standard atomic stuff but enforcing more explicit handling of memory ordering
enum class Memory_Order : u32 {
    Relaxed = std::memory_order_relaxed,
	Consume = std::memory_order_consume,
	Acquire = std::memory_order_acquire,
	Release = std::memory_order_release,
	Acq_Rel = std::memory_order_acq_rel,
	Seq_Cst = std::memory_order_seq_cst,
};

template<typename T>
using Atomic = std::atomic<T>;

template<typename T>
T atomic_exchange(volatile Atomic<T> * ptr, T desired, Memory_Order order){
	return std::atomic_exchange_explicit<T>(ptr, desired, (std::memory_order)order);
}

template<typename T>
bool atomic_compare_exchange_strong(Atomic<T> * ptr, std::atomic<T> * expected, T desired, Memory_Order order, Memory_Order failure_order = Memory_Order::Seq_Cst){
	return std::atomic_compare_exchange_strong_explicit(ptr, expected, desired, (std::memory_order)order, (std::memory_order)failure_order);
}
template<typename T>
bool atomic_compare_exchange_strong(volatile Atomic<T> * ptr, std::atomic<T> * expected, T desired, Memory_Order order, Memory_Order failure_order = Memory_Order::Seq_Cst){
	return std::atomic_compare_exchange_strong_explicit(ptr, expected, desired, (std::memory_order)order, (std::memory_order)failure_order);
}

template<typename T>
void atomic_store(Atomic<T> * ptr, T desired, Memory_Order order){
	return std::atomic_store_explicit<T>(ptr, desired, (std::memory_order)order);
}
template<typename T>
void atomic_store(volatile Atomic<T> * ptr, T desired, Memory_Order order){
	return std::atomic_store_explicit<T>(ptr, desired, (std::memory_order)order);
}

template<typename T>
T atomic_load(Atomic<T> const * ptr, Memory_Order order){
	return std::atomic_load_explicit<T>(ptr, (std::memory_order)order);
}
template<typename T>
T atomic_load(volatile Atomic<T> const * ptr, Memory_Order order){
	return std::atomic_load_explicit<T>(ptr, (std::memory_order)order);
}


//// Spinlock //////////////////////////////////////////////////////////////////
constexpr int SPINLOCK_LOCKED = 1;
constexpr int SPINLOCK_UNLOCKED = 0;

// The zeroed state of a spinlock is unlocked, to be effective across threads
// it's important to keep the spinlock outside of the stack and never mark it as
// a thread_local struct.
struct Spinlock {
	Atomic<int> _state{0};

	// Enter a busy wait loop until spinlock is acquired(locked)
	void acquire();

	// Try to lock spinlock, if failed, just move on. Returns if lock was locked.
	bool try_acquire();

	// Release(unlock) the spinlock
	void release();
};

//// Memory ////////////////////////////////////////////////////////////////////
enum class Allocator_Op {
	Query    = 0, // Query allocator's capabilities
	Alloc    = 1, // Allocate a chunk of memory
	Resize   = 2, // Resize an allocation in-place
	Free     = 3, // Mark allocation as free
	Free_All = 4, // Mark allocations as free
};

enum class Allocator_Capability {
	Alloc_Any = 1 << 0, // Can alloc any size
	Free_Any  = 1 << 1, // Can free in any order
	Free_All  = 1 << 2, // Can free all allocations
	Resize    = 1 << 3, // Can resize in-place
	Align_Any = 1 << 4, // Can alloc aligned to any alignment
};

// Memory allocator method
using Mem_Allocator_Func = void* (*) (
	void* impl,
	byte op,
	void* old_ptr,
	isize size, isize align,
	i32* capabilities
);

// Memory allocator interface
struct Mem_Allocator {
	void* _impl{0};
	Mem_Allocator_Func func{0};

	// Get capabilities of allocator as a number, gets capability bit-set
	u32 query_capabilites();

	// Allocate fresh memory, filled with 0s. Returns NULL on failure.
	void* alloc(isize size, isize align);

	// Re-allocate memory in-place without changing the original pointer. Returns
	// NULL on failure.
	void* resize(void* ptr, isize new_size);

	// Free pointer to memory, includes alignment information, which is required for
	// some allocators, freeing NULL is a no-op
	void free_ex(void* p, isize size, isize align);

	// Free pointer to memory, freeing NULL is a no-op
	void free(void* p);

	// Free all pointers owned by allocator
	void free_all();

	// Re-allocate to new_size, first tries to resize in-place, then uses
	// alloc->copy->free to attempt reallocation, returns null on failure
	void* realloc(void* ptr, isize old_size, isize new_size, isize align);
};

// Set n bytes of p to value.
void mem_set(void* p, byte val, isize nbytes);

// Copy n bytes for source to destination, they may overlap.
void mem_copy(void* dest, void const * src, isize nbytes);

// Compare 2 buffers of memory, returns -1, 0, 1 depending on which buffer shows
// a bigger byte first, 0 meaning equality.
i32 mem_compare(void const * a, void const * b, isize nbytes);

// Copy n bytes for source to destination, they should not overlap, this tends
// to be faster then mem_copy
void mem_copy_no_overlap(void* dest, void const * src, isize nbytes);

// Align p to alignment a, this only works if a is a non-zero power of 2
uintptr align_forward_ptr(uintptr p, uintptr a);

// Align p to alignment a, this works for any positive non-zero alignment
uintptr align_forward_size(isize p, isize a);

