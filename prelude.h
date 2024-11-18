#pragma once
//// Essentials ////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <limits.h>

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef unsigned int uint;
typedef uint8_t byte;

typedef ptrdiff_t isize;
typedef size_t    usize;

typedef uintptr_t uintptr;

typedef float f32;
typedef double f64;

typedef char const * cstring;

// Swap bytes around, useful for when dealing with endianess
static inline
void swap_bytes_raw(byte* data, isize len){
	for(isize i = 0; i < (len / 2); i += 1){
		byte temp = data[i];
		data[i] = data[len - (i + 1)];
		data[len - (i + 1)] = temp;
	}
}

#define swap_bytes(Ptr) swap_bytes_raw((byte*)(Ptr), sizeof(*(Ptr)))

_Static_assert(sizeof(f32) == 4 && sizeof(f64) == 8, "Bad float size");
_Static_assert(sizeof(isize) == sizeof(usize), "Mismatched (i/u)size");
_Static_assert(CHAR_BIT == 8, "Invalid char size");

#define min(A, B) ((A) < (B) ? (A) : (B))
#define max(A, B) ((A) > (B) ? (A) : (B))
#define clamp(Lo, X, Hi) min(max(Lo, X), Hi)

#define container_of(Ptr, Type, Member) \
	((Type *)(((void *)(Ptr)) - offsetof(Type, Member)))

#ifndef __cplusplus
#undef bool
typedef _Bool bool;
#endif

//// Assert ////////////////////////////////////////////////////////////////////
// Crash if `pred` is false, this is disabled in non-debug builds
void debug_assert(bool pred, cstring msg);

// Crash if `pred` is false, this is always enabled
void panic_assert(bool pred, cstring msg);

// Crash the program with a fatal error
noreturn void panic(char * const msg);

// Crash the program due to unimplemented code paths, this should *only* be used
// during development
noreturn void unimplemented();

//// Memory ////////////////////////////////////////////////////////////////////
typedef struct Bytes Bytes;
typedef struct Mem_Allocator Mem_Allocator;

#define mem_new(T_, N_, Al_) mem_alloc((Al_), sizeof(T_) * (N_), alignof(T_))

// Helper to use with printf "%.*s"
#define fmt_bytes(buf) (int)((buf).len), (buf).data

struct Bytes {
	byte* data;
	isize len;
};

enum Allocator_Op {
	Mem_Op_Query    = 0, // Query allocator's capabilities
	Mem_Op_Alloc    = 1, // Allocate a chunk of memory
	Mem_Op_Resize   = 2, // Resize an allocation in-place
	Mem_Op_Free     = 3, // Mark allocation as free
	Mem_Op_Free_All = 4, // Mark allocations as free
};

enum Allocator_Capability {
	Allocator_Alloc_Any = 1 << 0, // Can alloc any size
	Allocator_Free_Any  = 1 << 1, // Can free in any order
	Allocator_Free_All  = 1 << 2, // Can free all allocations
	Allocator_Resize    = 1 << 3, // Can resize in-place
	Allocator_Align_Any = 1 << 4, // Can alloc aligned to any alignment
};

// Memory allocator method
typedef void* (*Mem_Allocator_Func) (
	void * restrict impl,
	enum Allocator_Op op,
	void* old_ptr,
	isize size, isize align,
	i32* capabilities
);

// Memory allocator interface
struct Mem_Allocator {
	Mem_Allocator_Func func;
	void* data;
};

// Set n bytes of p to value.
void mem_set(void* p, byte val, isize nbytes);

// Copy n bytes for source to destination, they may overlap.
void mem_copy(void* dest, void const * src, isize nbytes);

// Copy n bytes for source to destination, they should not overlap, this tends
// to be faster then mem_copy
void mem_copy_no_overlap(void* dest, void const * src, isize nbytes);

// Align p to alignment a, this only works if a is a non-zero power of 2
uintptr align_forward_ptr(uintptr p, uintptr a);

// Align p to alignment a, this works for any positive non-zero alignment
uintptr align_forward_size(isize p, isize a);

// Get capabilities of allocator as a number, gets capability bit-set
u32 mem_query_capabilites(Mem_Allocator allocator);

// Allocate fresh memory, filled with 0s. Returns NULL on failure.
void* mem_alloc(Mem_Allocator allocator, isize size, isize align);

// Re-allocate memory in-place without changing the original pointer. Returns
// NULL on failure.
void* mem_resize(Mem_Allocator allocator, void* ptr, isize new_size);

// Free pointer to memory, includes alignment information, which is required for
// some allocators, freeing NULL is a no-op
void mem_free_ex(Mem_Allocator allocator, void* p, isize size, isize align);

// Free pointer to memory, freeing NULL is a no-op
void mem_free(Mem_Allocator allocator, void* p);

// Free all pointers owned by allocator
void mem_free_all(Mem_Allocator allocator);

//// UTF-8 /////////////////////////////////////////////////////////////////////
typedef i32 rune;
typedef struct UTF8_Encode_Result UTF8_Encode_Result;
typedef struct UTF8_Decode_Result UTF8_Decode_Result;
typedef struct UTF8_Iterator UTF8_Iterator;

// UTF-8 encoding result, a len = 0 means an error.
struct UTF8_Encode_Result {
	byte bytes[4];
	i8 len;
};

// UTF-8 encoding result, a len = 0 means an error.
struct UTF8_Decode_Result {
	rune codepoint;
	i8 len;
};

// The error rune
static const rune UTF8_ERROR = 0xfffd;

// The error rune, byte encoded
static const UTF8_Encode_Result UTF8_ERROR_ENCODED = {
	.bytes = {0xef, 0xbf, 0xbd},
	.len = 0,
};

// Encode a unicode rune
UTF8_Encode_Result utf8_encode(rune c);

// Decode a rune from a UTF8 buffer of bytes
UTF8_Decode_Result utf8_decode(byte const* data, isize len);

// Allows to iterate a stream of bytes as a sequence of runes
struct UTF8_Iterator {
	byte const* data;
	isize data_length;
	isize current;
};

// Steps iterator forward and puts rune and Length advanced into pointers,
// returns false when finished.
bool utf8_iter_next(UTF8_Iterator* iter, rune* r, i8* len);

// Steps iterator backward and puts rune and its length into pointers,
// returns false when finished.
bool utf8_iter_prev(UTF8_Iterator* iter, rune* r, i8* len);

//// Strings ///////////////////////////////////////////////////////////////////
typedef struct String String;

struct String {
	byte const * data;
	isize len;
};

static inline
isize cstring_len(cstring cstr){
	static const isize CSTR_MAX_LENGTH = (~(u32)0) >> 1;
	isize size = 0;
	for(isize i = 0; i < CSTR_MAX_LENGTH && cstr[i] != 0; i += 1){
		size += 1;
	}
	return size;
}

// Create substring from a cstring
String str_from(cstring data);

// Create substring from a piece of a cstring
String str_from_range(cstring data, isize start, isize length);

// Create substring from a raw slice of bytes
String str_from_bytes(byte const* data, isize length);

// Get a sub string, starting at `start` with `length`
String str_sub(String s, isize start, isize length);

// Get how many codeponits are in a string
isize str_codepoint_count(String s);

// Get the byte offset of the n-th codepoint
isize str_codepoint_offset(String s, isize n);

// Clone a string
String str_clone(String s, Mem_Allocator allocator);

// Destroy a string
void str_destroy(String s, Mem_Allocator allocator);

// Concatenate 2 strings
String str_concat(String a, String b, Mem_Allocator allocator);

// Check if 2 strings are equal
bool str_eq(String a, String b);

// Trim leading codepoints that belong to the cutset
String str_trim_leading(String s, String cutset);

// Trim trailing codepoints that belong to the cutset
String str_trim_trailing(String s, String cutset);

// Trim leading and trailing codepoints
String str_trim(String s, String cutset);

// Get an utf8 iterator from string
UTF8_Iterator str_iterator(String s);

// Get an utf8 iterator from string, already at the end, to be used for reverse iteration
UTF8_Iterator str_iterator_reversed(String s);

// Is string empty?
bool str_empty(String s);

//// Arena Allocator ///////////////////////////////////////////////////////////
typedef struct Mem_Arena Mem_Arena;

struct Mem_Arena {
	isize offset;
	isize capacity;
	byte* data;
};

// Initialize a memory arena with a buffer
void arena_init(Mem_Arena* a, byte* data, isize len);

// Deinit the arena
void arena_destroy(Mem_Arena *a);

// Get arena as a conforming instance to the allocator interface
Mem_Allocator arena_allocator(Mem_Arena* a);
