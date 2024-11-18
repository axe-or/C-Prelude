#pragma once

#include "prelude.h"
#include "memory.h"
#include "utf8.h"

typedef struct {
	isize len;
	byte const * data;
} String;

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
