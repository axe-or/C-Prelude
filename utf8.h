#pragma once

#include "./prelude.h"

#define UTF8_RANGE1 ((i32)0x7f)
#define UTF8_RANGE2 ((i32)0x7ff)
#define UTF8_RANGE3 ((i32)0xffff)
#define UTF8_RANGE4 ((i32)0x10ffff)

typedef i32 Codepoint;

typedef struct {
	byte bytes[4];
	i8 len;
} UTF8_Encode_Result;

typedef struct {
	Codepoint codepoint;
	i8 len;
} UTF8_Decode_Result;

static const Codepoint UTF8_ERROR = 0xfffd;

static const UTF8_Encode_Result UTF8_ERROR_ENCODED = {
	.bytes = {0xef, 0xbf, 0xbd},
	.len = 3,
};

// Encode a unicode Codepoint
UTF8_Encode_Result utf8_encode(Codepoint c);

// Decode a codepoint from a UTF8 buffer of bytes
UTF8_Decode_Result utf8_decode(byte const* data, isize len);

typedef struct {
	byte const* data;
	isize data_length;
	isize current;
} UTF8_Iterator;

// Steps iterator forward and puts Codepoint and Length advanced into pointers,
// returns false when finished.
bool utf8_iter_next(UTF8_Iterator* iter, Codepoint* r, i8* len);

// Steps iterator backward and puts Codepoint and its length into pointers,
// returns false when finished.
bool utf8_iter_prev(UTF8_Iterator* iter, Codepoint* r, i8* len);
