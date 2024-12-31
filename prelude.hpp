#pragma once

//// Platform //////////////////////////////////////////////////////////////////
#if defined(TARGET_OS_WINDOWS)
	#define TARGET_OS_NAME "Windows"
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <stdlib.h>
	#undef min
	#undef max

#elif defined(TARGET_OS_LINUX)
	#define TARGET_OS_NAME "Linux"
	#define _XOPEN_SOURCE 800
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
	usize len = sizeof(T);
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

//// Atomic ////////////////////////////////////////////////////////////////////
// Mostly just boilerplate around C++'s standard atomic stuff
enum class Memory_Order : u32 {
    Relaxed = std::memory_order_relaxed,
	Consume = std::memory_order_consume,
	Acquire = std::memory_order_acquire,
	Release = std::memory_order_release,
	Acq_Rel = std::memory_order_acq_rel,
	Seq_Cst = std::memory_order_seq_cst,
};

template<typename T>
T atomic_exchange(std::atomic<T> * ptr, T desired, Memory_Order order){
	return std::atomic_exchange_explicit<T>(ptr, desired, (std::memory_order)order);
}
template<typename T>
T atomic_exchange(volatile std::atomic<T> * ptr, T desired, Memory_Order order){
	return std::atomic_exchange_explicit<T>(ptr, desired, (std::memory_order)order);
}

template<typename T>
void atomic_store(std::atomic<T> * ptr, T desired, Memory_Order order){
	return std::atomic_store_explicit<T>(ptr, desired, (std::memory_order)order);
}
template<typename T>
void atomic_store(volatile std::atomic<T> * ptr, T desired, Memory_Order order){
	return std::atomic_store_explicit<T>(ptr, desired, (std::memory_order)order);
}

template<typename T>
T atomic_load(std::atomic<T> const * ptr, Memory_Order order){
	return std::atomic_load_explicit<T>(ptr, (std::memory_order)order);
}
template<typename T>
T atomic_load(volatile std::atomic<T> const * ptr, Memory_Order order){
	return std::atomic_load_explicit<T>(ptr, (std::memory_order)order);
}


//// Spinlock //////////////////////////////////////////////////////////////////
constexpr int SPINLOCK_LOCKED = 1;
constexpr int SPINLOCK_UNLOCKED = 0;

// The zeroed state of a spinlock is unlocked, to be effective across threads
// it's important to keep the spinlock outside of the stack and never mark it as
// a thread_local struct.
struct Spinlock {
	std::atomic_int _state{0};

	// Enter a busy wait loop until spinlock is acquired(locked)
	void acquire();

	// Try to lock spinlock, if failed, just move on. Returns if lock was locked.
	bool try_acquire();

	// Release(unlock) the spinlock
	void release();
};

//// IO Interface //////////////////////////////////////////////////////////////
enum class IO_Op {
	Query = 0,
	Read  = 1,
	Write = 2,
};

enum class IO_Error {
	None          = 0,
	End_Of_Stream = -1,
	Unsupported   = -2,
	Memory_Error  = -3,
	Broken_Handle = -4,
};

enum class IO_Capability {
	Read  = 1 << 0,
	Write = 1 << 1,
};

// Function that represents a read or write operation. Returns number of bytes
// read into buf, returns a negative number indicating error if any.
typedef i64 (*IO_Func)(
	void* impl,
	byte op,
	byte* buf,
	isize buflen);

// Stream interface for IO operations.
struct IO_Stream {
	virtual i64 read(byte* buf, isize buflen) = 0;

	virtual i64 write(byte* buf, isize buflen) = 0;

	virtual u8 capabilities() = 0;
};

