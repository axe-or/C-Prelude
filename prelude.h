/* Essential definitions. */
#pragma once

#define BASE_C_VERSION "{{BaseCVersion}}"

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

