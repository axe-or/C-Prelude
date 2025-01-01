#pragma once
// TODO: futex

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
using byte = uint8_t;
using rune = i32;

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

void bounds_check_assert_ex(bool pred, cstring msg, Source_Location loc);

#if defined(NDEBUG) || defined(RELEASE_MODE)
#define debug_assert(Pred, Msg) ((void)0)
#else
#define debug_assert(Pred, Msg) debug_assert_ex(Pred, Msg, this_location())
#endif

#if defined(DISABLE_BOUNDS_CHECK)
#define bounds_check_assert(Pred, Msg) ((void)0)
#else
#define bounds_check_assert(Pred, Msg) bounds_check_assert_ex(Pred, Msg, this_location())
#endif

// Crash if `pred` is false, this is always enabled
void panic_assert_ex(bool pred, cstring msg, Source_Location loc);

#define panic_assert(Pred, Msg) panic_assert_ex(Pred, Msg, this_location())

// Crash the program with a fatal error
[[noreturn]] void panic(cstring msg);

// Crash the program due to unimplemented code paths, this should *only* be used
// during development
[[noreturn]] void unimplemented();

//// Slices ////////////////////////////////////////////////////////////////////
template<typename T>
struct Slice {
	T* _data;
	isize _length;

	isize size() const { return _length; }

	T* raw_data() const { return _data; }

	bool empty() const { return _length == 0 || _data == nullptr; }

	T& operator[](isize idx) noexcept {
		bounds_check_assert(idx >= 0 && idx < _length, "Index to slice is out of bounds");
		return _data[idx];
	}

	T const& operator[](isize idx) const noexcept {
		bounds_check_assert(idx >= 0 && idx < _length, "Index to slice is out of bounds");
		return _data[idx];
	}

	// Get a sub-slice in the interval a..slice.size()
	Slice<T> slice(isize from){
		bounds_check_assert(from >= 0 && from < _length, "Index to sub-slice is out of bounds");
		Slice<T> s;
		s._length = _length - from;
		s._data = &_data[from];
		return s;
	}

	// Get a sub-slice in the interval a..b (end exclusive)
	Slice<T> slice(isize from, isize to){
		bounds_check_assert(from <= to, "Improper slicing range");
		bounds_check_assert(from >= 0 && from < _length, "Index to sub-slice is out of bounds");
		bounds_check_assert(to >= 0 && to <= _length, "Index to sub-slice is out of bounds");

		Slice<T> s;
		s._length = to - from;
		s._data = &_data[from];
		return s;
	}

	Slice() : _data{nullptr}, _length{0} {}

	static Slice<T> from_pointer(T* ptr, isize len){
		Slice<T> s;
		s._data = ptr;
		s._length = len;
		return s;
	}
};

//// Atomic ////////////////////////////////////////////////////////////////////
// Mostly just boilerplate around C++'s standard atomic stuff but enforcing more explicit handling of memory ordering
namespace atomic {
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
T exchange(volatile Atomic<T> * ptr, T desired, Memory_Order order = Memory_Order::Seq_Cst){
	return std::atomic_exchange_explicit<T>(ptr, desired, (std::memory_order)order);
}

template <typename T>
bool compare_exchange_strong(Atomic<T> *ptr, std::atomic<T> *expected, T desired, Memory_Order order = Memory_Order::Seq_Cst, Memory_Order failure_order = Memory_Order::Seq_Cst) {
  return std::atomic_compare_exchange_strong_explicit(ptr, expected, desired, (std::memory_order)order, (std::memory_order)failure_order);
}
template<typename T>
bool compare_exchange_strong(volatile Atomic<T> * ptr, std::atomic<T> * expected, T desired, Memory_Order order = Memory_Order::Seq_Cst, Memory_Order failure_order = Memory_Order::Seq_Cst){
	return std::atomic_compare_exchange_strong_explicit(ptr, expected, desired, (std::memory_order)order, (std::memory_order)failure_order);
}

template<typename T>
void store(Atomic<T> * ptr, T desired, Memory_Order order = Memory_Order::Seq_Cst){
	return std::atomic_store_explicit<T>(ptr, desired, (std::memory_order)order);
}
template<typename T>
void store(volatile Atomic<T> * ptr, T desired, Memory_Order order = Memory_Order::Seq_Cst){
	return std::atomic_store_explicit<T>(ptr, desired, (std::memory_order)order);
}

template<typename T>
T load(Atomic<T> const * ptr, Memory_Order order = Memory_Order::Seq_Cst){
	return std::atomic_load_explicit<T>(ptr, (std::memory_order)order);
}
template<typename T>
T load(volatile Atomic<T> const * ptr, Memory_Order order = Memory_Order::Seq_Cst){
	return std::atomic_load_explicit<T>(ptr, (std::memory_order)order);
}
}

//// Sync //////////////////////////////////////////////////////////////////////
namespace sync {
constexpr int SPINLOCK_UNLOCKED = 0;
constexpr int SPINLOCK_LOCKED = 1;

// The zeroed state of a spinlock is unlocked, to be effective across threads
// it's important to keep the spinlock outside of the stack and never mark it as
// a thread_local struct.
struct Spinlock {
	atomic::Atomic<int> _state{0};

	// Enter a busy wait loop until spinlock is acquired(locked)
	void acquire();

	// Try to lock spinlock, if failed, just move on. Returns if lock was locked.
	bool try_acquire();

	// Release(unlock) the spinlock
	void release();
};

}

//// Memory ////////////////////////////////////////////////////////////////////
enum class Allocator_Op : byte {
	Query    = 0, // Query allocator's capabilities
	Alloc    = 1, // Allocate a chunk of memory
	Resize   = 2, // Resize an allocation in-place
	Free     = 3, // Mark allocation as free
	Free_All = 4, // Mark allocations as free
};

enum class Allocator_Capability : u32 {
	Alloc_Any = 1 << 0, // Can alloc any size
	Free_Any  = 1 << 1, // Can free in any order
	Free_All  = 1 << 2, // Can free all allocations
	Resize    = 1 << 3, // Can resize in-place
	Align_Any = 1 << 4, // Can alloc aligned to any alignment
};

// Memory allocator method
using Mem_Allocator_Func = void* (*) (
	void* impl,
	Allocator_Op op,
	void* old_ptr,
	isize size, isize align,
	u32* capabilities
);

// Memory allocator interface
struct Mem_Allocator {
	void* _impl{0};
	Mem_Allocator_Func _func{0};

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

//// Make & Destroy ////////////////////////////////////////////////////////////
// Allocate one of object of a type using allocator
template<typename T>
T* make(Mem_Allocator al){
	T* p = al.alloc(sizeof(T), alignof(T));
	if(p != nullptr){
		new (&p) T();
	}
	return p;
}

// Allocate slice of a type using allocator
template<typename T>
Slice<T> make(isize count, Mem_Allocator al){
	T* p = al.alloc(sizeof(T) * count, alignof(T) * count);
	if(p != nullptr){
		for(isize i = 0; i < count; i ++){
			new (&p[i]) T();
		}
	}
	return Slice<T>::from_pointer(p, count);
}

// Deallocate object from allocator
template<typename T>
void destroy(T* ptr, Mem_Allocator al){
	ptr->~T();
	al.free(ptr);
}

// Deallocate slice from allocator
template<typename T>
void destroy(Slice<T> s, Mem_Allocator al){
	isize n = s.size();
	T* ptr = s.raw_data();
	for(isize i = 0; i < n; i ++){
		ptr[i]->~T();
	}
	al.free(ptr);
}

//// UTF-8 /////////////////////////////////////////////////////////////////////
namespace utf8 {

// UTF-8 encoding result, a len = 0 means an error.
struct Encode_Result {
	byte bytes[4];
	i8 len;
};

// UTF-8 encoding result, a len = 0 means an error.
struct Decode_Result {
	rune codepoint;
	i8 len;
};

// The error rune
constexpr rune ERROR = 0xfffd;

// The error rune, byte encoded
constexpr Encode_Result ERROR_ENCODED = {
	.bytes = {0xef, 0xbf, 0xbd},
	.len = 0,
};

// Encode a unicode codepoint
Encode_Result encode(rune c);

// Decode a codepoint from a UTF8 buffer of bytes
Decode_Result utf8_decode(Slice<byte> buf);

// Allows to iterate a stream of bytes as a sequence of runes
struct Iterator {
	Slice<byte> data;
	isize current;

	// Steps iterator forward and puts rune and length advanced into pointers,
	// returns false when finished.
	bool next(rune* r, i8* len);

	// Steps iterator backward and puts rune and its length into pointers,
	// returns false when finished.
	bool prev(rune* r, i8* len);
};

} /* Namespace utf8 */

//// Strings ///////////////////////////////////////////////////////////////////
#if 0
static inline isize cstring_len(cstring cstr);

struct String {
	byte const * _data;
	isize _length;

	// Size (in bytes)
	isize size() const;

	// Size (in codepoints)
	isize rune_count() const;

	// Create a substring
	String sub(isize start, isize length);

	// Create string from C-style string
	static String from_cstr(cstring data);

	// Create string from a piece of a C-style string
	static String from_cstr(cstring data, isize start, isize length);

	// Create string from a raw byte pointer
	static String from_pointer(byte const* data, isize length);

	// Check if 2 strings are equal
	bool operator==(String lhs) const {
		if(lhs._length != _length){ return false; }
		return mem_compare(_data, lhs._data, _length) == 0;
	}
};

static inline
isize cstring_len(cstring cstr){
	constexpr isize CSTR_MAX_LENGTH = (~(u32)0) >> 1;
	isize size = 0;
	for(isize i = 0; i < CSTR_MAX_LENGTH && cstr[i] != 0; i += 1){
		size += 1;
	}
	return size;
}

// Get the byte offset of the n-th codepoint
isize str_codepoint_offset(String s, isize n);

// Clone a string
String str_clone(String s, Mem_Allocator allocator);

// Destroy a string
void str_destroy(String s, Mem_Allocator allocator);

// Concatenate 2 strings
String str_concat(String a, String b, Mem_Allocator allocator);

// Trim leading codepoints that belong to the cutset
String str_trim_leading(String s, String cutset);

// Trim trailing codepoints that belong to the cutset
String str_trim_trailing(String s, String cutset);

// Trim leading and trailing codepoints
String str_trim(String s, String cutset);

// Check if string starts with a prefix
bool str_starts_with(String s, String prefix);

// Check if string ends with a suffix
bool str_ends_with(String s, String suffix);

// Get an utf8 iterator from string
// UTF8_Iterator str_iterator(String s);

// Get an utf8 iterator from string, already at the end, to be used for reverse iteration
// UTF8_Iterator str_iterator_reversed(String s);
#endif

