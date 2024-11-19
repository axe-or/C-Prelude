#include "prelude.h"

//// Assert ////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>

void debug_assert(bool pred, cstring msg){
	#ifdef NDEBUG
		(void)pred; (void)msg;
	#else
	if(!pred){
		fprintf(stderr, "Failed assert: %s\n", msg);
		abort();
	}
	#endif
}

void panic_assert(bool pred, cstring msg){
	if(!pred){
		fprintf(stderr, "Failed assert: %s\n", msg);
		abort();
	}
}

noreturn
void panic(char* const msg){
	fprintf(stderr, "Panic: %s\n", msg);
	abort();
}

noreturn
void unimplemented(){
	fprintf(stderr, "Unimplemented code\n");
	abort();
}

//// Spinlock //////////////////////////////////////////////////////////////////
void spinlock_acquire(Spinlock* l){
	for(;;){
		if(!atomic_exchange_explicit(&l->_state, SPINLOCK_LOCKED, memory_order_acquire)){
			break;
		}
		/* Busy wait while locked */
		while(atomic_load_explicit(&l->_state, memory_order_relaxed));
	}
}

bool spinlock_try_acquire(Spinlock* l){
    return !atomic_exchange_explicit(&l->_state, SPINLOCK_LOCKED, memory_order_acquire);
}

void spinlock_release(Spinlock* l){
	atomic_store(&l->_state, SPINLOCK_UNLOCKED);
}

//// Memory ////////////////////////////////////////////////////////////////////
static inline
bool mem_valid_alignment(isize align){
	return (align & (align - 1)) == 0 && (align != 0);
}

void mem_set(void* p, byte val, isize nbytes){
	__builtin_memset(p, val, nbytes);
}

void mem_copy(void* dest, void const * src, isize nbytes){
	__builtin_memmove(dest, src, nbytes);
}

void mem_copy_no_overlap(void* dest, void const * src, isize nbytes){
	__builtin_memcpy(dest, src, nbytes);
}

uintptr align_forward_ptr(uintptr p, uintptr a){
	debug_assert(mem_valid_alignment(a), "Invalid memory alignment");
	uintptr mod = p & (a - 1);
	if(mod > 0){
		p += (a - mod);
	}
	return p;
}

uintptr align_forward_size(isize p, isize a){
	debug_assert(a > 0, "Invalid size alignment");
	isize mod = p % a;
	if(mod > 0){
		p += (a - mod);
	}
	return p;
}

i32 allocator_query_capabilites(Mem_Allocator allocator, i32* capabilities){
	if(capabilities == null){ return 0; }
	allocator.func(allocator.data, Mem_Op_Query, null, 0, 0, capabilities);
	return *capabilities;
}

void* mem_alloc(Mem_Allocator allocator, isize size, isize align){
	void* ptr = allocator.func(allocator.data, Mem_Op_Alloc, null, size, align, null);
	if(ptr != null){
		mem_set(ptr, 0, size);
	}
	return ptr;
}

void* mem_resize(Mem_Allocator allocator, void* ptr, isize new_size){
	void* new_ptr = allocator.func(allocator.data, Mem_Op_Resize, ptr, new_size, 0, null);
	return new_ptr;
}

void mem_free_ex(Mem_Allocator allocator, void* p, isize size, isize align){
	if(p == null){ return; }
	allocator.func(allocator.data, Mem_Op_Free, p, size, align, null);
}

void mem_free(Mem_Allocator allocator, void* p){
	mem_free_ex(allocator, p, 0, 0);
}

void mem_free_all(Mem_Allocator allocator){
	allocator.func(allocator.data, Mem_Op_Free_All, null, 0, 0, null);
}

//// UTF-8 /////////////////////////////////////////////////////////////////////
#define UTF8_RANGE1 ((i32)0x7f)
#define UTF8_RANGE2 ((i32)0x7ff)
#define UTF8_RANGE3 ((i32)0xffff)
#define UTF8_RANGE4 ((i32)0x10ffff)

#define UTF16_SURROGATE1 ((i32)0xd800)
#define UTF16_SURROGATE2 ((i32)0xdfff)

#define UTF8_MASK2 (0x1f) /* 0001_1111 */
#define UTF8_MASK3 (0x0f) /* 0000_1111 */
#define UTF8_MASK4 (0x07) /* 0000_0111 */

#define UTF8_MASKX (0x3f) /* 0011_1111 */

#define UTF8_SIZE2 (0xc0) /* 110x_xxxx */
#define UTF8_SIZE3 (0xe0) /* 1110_xxxx */
#define UTF8_SIZE4 (0xf0) /* 1111_0xxx */

#define CONT (0x80)  /* 10xx_xxxx */

static inline
bool is_continuation_byte(rune c){
	static const rune CONTINUATION1 = 0x80;
	static const rune CONTINUATION2 = 0xbf;
	return (c >= CONTINUATION1) && (c <= CONTINUATION2);
}

UTF8_Encode_Result utf8_encode(rune c){
	UTF8_Encode_Result res = {0};

	if(is_continuation_byte(c) ||
	   (c >= UTF16_SURROGATE1 && c <= UTF16_SURROGATE2) ||
	   (c > UTF8_RANGE4))
	{
		return UTF8_ERROR_ENCODED;
	}

	if(c <= UTF8_RANGE1){
		res.len = 1;
		res.bytes[0] = c;
	}
	else if(c <= UTF8_RANGE2){
		res.len = 2;
		res.bytes[0] = UTF8_SIZE2 | ((c >> 6) & UTF8_MASK2);
		res.bytes[1] = CONT  | ((c >> 0) & UTF8_MASKX);
	}
	else if(c <= UTF8_RANGE3){
		res.len = 3;
		res.bytes[0] = UTF8_SIZE3 | ((c >> 12) & UTF8_MASK3);
		res.bytes[1] = CONT  | ((c >> 6) & UTF8_MASKX);
		res.bytes[2] = CONT  | ((c >> 0) & UTF8_MASKX);
	}
	else if(c <= UTF8_RANGE4){
		res.len = 4;
		res.bytes[0] = UTF8_SIZE4 | ((c >> 18) & UTF8_MASK4);
		res.bytes[1] = CONT  | ((c >> 12) & UTF8_MASKX);
		res.bytes[2] = CONT  | ((c >> 6)  & UTF8_MASKX);
		res.bytes[3] = CONT  | ((c >> 0)  & UTF8_MASKX);
	}
	return res;
}

static const UTF8_Decode_Result DECODE_ERROR = { .codepoint = UTF8_ERROR, .len = 0 };

UTF8_Decode_Result utf8_decode(byte const* buf, isize len){
	UTF8_Decode_Result res = {0};
	if(buf == null || len <= 0){ return DECODE_ERROR; }

	u8 first = buf[0];

	if((first & CONT) == 0){
		res.len = 1;
		res.codepoint |= first;
	}
	else if ((first & ~UTF8_MASK2) == UTF8_SIZE2 && len >= 2){
		res.len = 2;
		res.codepoint |= (buf[0] & UTF8_MASK2) << 6;
		res.codepoint |= (buf[1] & UTF8_MASKX) << 0;
	}
	else if ((first & ~UTF8_MASK3) == UTF8_SIZE3 && len >= 3){
		res.len = 3;
		res.codepoint |= (buf[0] & UTF8_MASK3) << 12;
		res.codepoint |= (buf[1] & UTF8_MASKX) << 6;
		res.codepoint |= (buf[2] & UTF8_MASKX) << 0;
	}
	else if ((first & ~UTF8_MASK4) == UTF8_SIZE4 && len >= 4){
		res.len = 4;
		res.codepoint |= (buf[0] & UTF8_MASK4) << 18;
		res.codepoint |= (buf[1] & UTF8_MASKX) << 12;
		res.codepoint |= (buf[2] & UTF8_MASKX) << 6;
		res.codepoint |= (buf[3] & UTF8_MASKX) << 0;
	}
	else {
		return DECODE_ERROR;
	}

	// Validation step
	if(res.codepoint >= UTF16_SURROGATE1 && res.codepoint <= UTF16_SURROGATE2){
		return DECODE_ERROR;
	}
	if(res.len > 1 && !is_continuation_byte(buf[1])){
		return DECODE_ERROR;
	}
	if(res.len > 2 && !is_continuation_byte(buf[2])){
		return DECODE_ERROR;
	}
	if(res.len > 3 && !is_continuation_byte(buf[3])){
		return DECODE_ERROR;
	}

	return res;
}

// static inline
// bool is_continuation_byte

// Steps iterator forward and puts rune and Length advanced into pointers,
// returns false when finished.
bool utf8_iter_next(UTF8_Iterator* iter, rune* r, i8* len){
	if(iter->current >= iter->data_length){ return 0; }

	UTF8_Decode_Result res = utf8_decode(&iter->data[iter->current], iter->data_length);
	*r = res.codepoint;
	*len = res.len;

	if(res.codepoint == DECODE_ERROR.codepoint){
		*len = res.len + 1;
	}

	iter->current += res.len;

	return 1;
}

// Steps iterator backward and puts rune and its length into pointers,
// returns false when finished.
bool utf8_iter_prev(UTF8_Iterator* iter, rune* r, i8* len){
	if(iter->current <= 0){ return false; }

	iter->current -= 1;
	while(is_continuation_byte(iter->data[iter->current])){
		iter->current -= 1;
	}

	UTF8_Decode_Result res = utf8_decode(&iter->data[iter->current], iter->data_length - iter->current);
	*r = res.codepoint;
	*len = res.len;
	return true;
}

#undef CONT


//// Strings ///////////////////////////////////////////////////////////////////
bool str_empty(String s){
	return s.len == 0 || s.data == null;
}

String str_from(cstring data){
	String s = {
		.data = (byte const *)data,
		.len = cstring_len(data),
	};
	return s;
}

String str_from_bytes(byte const* data, isize length){
	String s = {
		.data = (byte const *)data,
		.len = length,
	};
	return s;
}

String str_concat(String a, String b, Mem_Allocator allocator){
	byte* data = mem_new(byte, a.len + b.len, allocator);
	String s = {0};
	if(data != null){
		mem_copy(&data[0], a.data, a.len);
		mem_copy(&data[a.len], b.data, b.len);
		s.data = data;
		s.len = a.len + b.len;
	}
	return s;
}

String str_from_range(cstring data, isize start, isize length){
	String s = {
		.data = (byte const *)&data[start],
		.len = length,
	};
	return s;
}

isize str_codepoint_count(String s){
	UTF8_Iterator it = str_iterator(s);

	isize count = 0;
	rune c; i8 len;
	while(utf8_iter_next(&it, &c, &len)){
		count += 1;
	}
	return count;
}


isize str_codepoint_offset(String s, isize n){
	UTF8_Iterator it = str_iterator(s);
	
	isize acc = 0;

	rune c; i8 len;
	do {
		if(acc == n){ break; }
		acc += 1;
	} while(utf8_iter_next(&it, &c, &len));

	return it.current;
}

// TODO: Handle length in codepoint count
String str_sub(String s, isize start, isize byte_count){
	if(start < 0 || byte_count < 0 || (start + byte_count) > s.len){ return (String){0}; }

	String sub = {
		.data = &s.data[start],
		.len = byte_count,
	};

	return sub;
}

String str_clone(String s, Mem_Allocator allocator){
	char* mem = mem_new(char, s.len, allocator);
	if(mem == null){ return (String){0}; }
	return (String){
		.data = (byte const *)mem,
		.len = s.len,
	};
}

bool str_eq(String a, String b){
	if(a.len != b.len){ return false; }

	for(isize i = 0; i < a.len; i += 1){
		if(a.data[i] != b.data[i]){ return false; }
	}

	return true;
}

UTF8_Iterator str_iterator(String s){
	return (UTF8_Iterator){
		.current = 0,
		.data_length = s.len,
		.data = s.data,
	};
}

UTF8_Iterator str_iterator_reversed(String s){
	return (UTF8_Iterator){
		.current = s.len,
		.data_length = s.len,
		.data = s.data,
	};
}

void str_destroy(String s, Mem_Allocator allocator){
	mem_free(allocator, (void*)s.data);
}

#define MAX_CUTSET_LEN 64

String str_trim(String s, String cutset){
	String st = str_trim_leading(str_trim_trailing(s, cutset), cutset);
	return st;
}

String str_trim_leading(String s, String cutset){
	debug_assert(cutset.len <= MAX_CUTSET_LEN, "Cutset string exceeds MAX_CUTSET_LEN");

	rune set[MAX_CUTSET_LEN] = {0};
	isize set_len = 0;
	isize cut_after = 0;

	decode_cutset: {
		rune c; i8 n;
		UTF8_Iterator iter = str_iterator(cutset);

		isize i = 0;
		while(utf8_iter_next(&iter, &c, &n) && i < MAX_CUTSET_LEN){
			set[i] = c;
			i += 1;
		}
		set_len = i;
	}

	strip_cutset: {
		rune c; i8 n;
		UTF8_Iterator iter = str_iterator(s);

		while(utf8_iter_next(&iter, &c, &n)){
			bool to_be_cut = false;
			for(isize i = 0; i < set_len; i += 1){
				if(set[i] == c){
					to_be_cut = true;
					break;
				}
			}

			if(to_be_cut){
				cut_after += n;
			}
			else {
				break; // Reached first rune that isn't in cutset
			}

		}
	}

	return str_sub(s, cut_after, s.len - cut_after);
}

String str_trim_trailing(String s, String cutset){
	debug_assert(cutset.len <= MAX_CUTSET_LEN, "Cutset string exceeds MAX_CUTSET_LEN");

	rune set[MAX_CUTSET_LEN] = {0};
	isize set_len = 0;
	isize cut_until = s.len;

	decode_cutset: {
		rune c; i8 n;
		UTF8_Iterator iter = str_iterator(cutset);

		isize i = 0;
		while(utf8_iter_next(&iter, &c, &n) && i < MAX_CUTSET_LEN){
			set[i] = c;
			i += 1;
		}
		set_len = i;
	}

	strip_cutset: {
		rune c; i8 n;
		UTF8_Iterator iter = str_iterator_reversed(s);

		while(utf8_iter_prev(&iter, &c, &n)){
			bool to_be_cut = false;
			for(isize i = 0; i < set_len; i += 1){
				if(set[i] == c){
					to_be_cut = true;
					break;
				}
			}

			if(to_be_cut){
				cut_until -= n;
			}
			else {
				break; // Reached first rune that isn't in cutset
			}

		}
	}

	return str_sub(s, 0, cut_until);
}
//// Logger ////////////////////////////////////////////////////////////////////
i32 log_ex_str(Logger l, String message, Source_Location loc, u8 level_n){
    i32 n = l.log_func(l.impl, message, loc, level_n);
	if(level_n == Log_Fatal){
		panic("Fatal error");
	}
	return n;
}

i32 log_ex_cstr(Logger l, cstring message, Source_Location loc, u8 level_n){
    return log_ex_str(l, str_from(message), loc, level_n);
}

static i32 _console_logger_func(void* impl, String message, Source_Location location, u8 level_n){
    (void)impl;
    enum Log_Level level = level;
    cstring header = log_level_map[level_n];
    i32 n = printf("[%-5s - %.*s:%d %.*s] %.*s\n", header, fmt_bytes(location.filename), location.line, fmt_bytes(location.caller_name), fmt_bytes(message));
    return n;
}

Logger log_create_console_logger(){
    return (Logger){
        .impl = null,
        .log_func = _console_logger_func,
    };
}
//// Arena Allocator ///////////////////////////////////////////////////////////
static
uintptr arena_required_mem(uintptr cur, isize nbytes, isize align){
	debug_assert(mem_valid_alignment(align), "Alignment must be a power of 2");
	uintptr_t aligned  = align_forward_ptr(cur, align);
	uintptr_t padding  = (uintptr)(aligned - cur);
	uintptr_t required = padding + nbytes;
	return required;
}

void *arena_alloc(Mem_Arena* a, isize size, isize align){
	uintptr base = (uintptr)a->data;
	uintptr current = (uintptr)base + (uintptr)a->offset;

	uintptr available = (uintptr)a->capacity - (current - base);
	uintptr required = arena_required_mem(current, size, align);

	if(required > available){
		return null;
	}

	a->offset += required;
	void* allocation = &a->data[a->offset - size];
	a->last_allocation = (uintptr)allocation;
	return allocation;
}


void arena_free_all(Mem_Arena* a){
	a->offset = 0;
}

static
void* arena_allocator_func(
	void* impl,
	enum Allocator_Op op,
	void* old_ptr,
	isize size,
	isize align,
	i32* capabilities)
{
	Mem_Arena* a = impl;
	(void)old_ptr;

	switch(op){
		case Mem_Op_Alloc: {
			return arena_alloc(a, size, align);
		} break;

		case Mem_Op_Free_All: {
			arena_free_all(a);
		} break;

		case Mem_Op_Resize: {
			return arena_resize(a, old_ptr, size);
		} break;

		case Mem_Op_Free: {} break;

		case Mem_Op_Query: {
			*capabilities = Allocator_Alloc_Any | Allocator_Free_All | Allocator_Align_Any | Allocator_Resize;
		} break;
	}

	return null;
}

void* arena_resize(Mem_Arena* a, void* ptr, isize new_size){
	if((uintptr)ptr == a->last_allocation){
		uintptr base = (uintptr)a->data;
		uintptr current = base + (uintptr)a->offset;
		uintptr limit = base + (uintptr)a->capacity;
		isize last_allocation_size = current - a->last_allocation;
		
		if((current - last_allocation_size + new_size) > limit){
			return null; /* No space left*/
		}

		a->offset += new_size - last_allocation_size;
		return ptr;
	}

	return null;
}

Mem_Allocator arena_allocator(Mem_Arena* a){
	Mem_Allocator allocator = {
		.data = a,
		.func = arena_allocator_func,
	};
	return allocator;
}

void arena_init(Mem_Arena* a, byte* data, isize len){
	a->capacity = len;
	a->data = data;
	a->offset = 0;
}

void arena_destroy(Mem_Arena* a){
	arena_free_all(a);
	a->capacity = 0;
	a->data = null;
}
