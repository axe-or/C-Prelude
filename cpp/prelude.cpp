#include "prelude.hpp"

//// Sync //////////////////////////////////////////////////////////////////////
namespace sync {
using atomic::Memory_Order;

void Spinlock::acquire(){
	for(;;){
		if(!atomic::exchange(&_state, SPINLOCK_LOCKED, Memory_Order::Acquire)){
			break;
		}
		/* Busy wait while locked */
		while(atomic::load(&_state, Memory_Order::Relaxed));
	}
}

bool Spinlock::try_acquire(){
    return !atomic::load(&_state, Memory_Order::Relaxed) &&
		!atomic::exchange(&_state, SPINLOCK_LOCKED, Memory_Order::Acquire);
}

void Spinlock::release(){
	atomic::store(&_state, SPINLOCK_UNLOCKED, Memory_Order::Release);
}
}

//// Assert ////////////////////////////////////////////////////////////////////
void debug_assert_ex(bool pred, cstring msg, Source_Location loc){
	#if defined(NDEBUG) || defined(RELEASE_MODE)
		(void)pred; (void)msg;
	#else
	if(!pred){
		fprintf(stderr, "(%s:%d) Failed assert: %s\n", loc.filename, loc.line, msg);
		abort();
	}
	#endif
}

void panic_assert_ex(bool pred, cstring msg, Source_Location loc){
	if(!pred){
		fprintf(stderr, "[%s:%d %s()] Failed assert: %s\n", loc.filename, loc.line, loc.caller_name, msg);
		abort();
	}
}

void bounds_check_assert_ex(bool pred, cstring msg, Source_Location loc){
	#if defined(DISABLE_BOUNDS_CHECK)
		(void)pred; (void)msg;
	#else
	if(!pred){
		fprintf(stderr, "(%s:%d) Failed bounds check: %s\n", loc.filename, loc.line, msg);
		abort();
	}
	#endif
}

[[noreturn]]
void panic(char* const msg){
	fprintf(stderr, "Panic: %s\n", msg);
	abort();
}

[[noreturn]]
void unimplemented(){
	fprintf(stderr, "Unimplemented code\n");
	abort();
}

//// Memory ////////////////////////////////////////////////////////////////////
#if !defined(__clang__) && !defined(__GNUC__)
#include <string.h>
#define mem_set_impl             memset
#define mem_copy_impl            memmove
#define mem_copy_no_overlap_impl memcpy
#define mem_compare_impl         memcmp
#else
#define mem_set_impl             __builtin_memset
#define mem_copy_impl            __builtin_memmove
#define mem_copy_no_overlap_impl __builtin_memcpy
#define mem_compare_impl         __builtin_memcmp
#endif

namespace mem {
static inline
bool valid_alignment(isize align){
	return (align & (align - 1)) == 0 && (align != 0);
}

void set(void* p, byte val, isize nbytes){
	mem_set_impl(p, val, nbytes);
}

void copy(void* dest, void const * src, isize nbytes){
	mem_copy_impl(dest, src, nbytes);
}

void copy_no_overlap(void* dest, void const * src, isize nbytes){
	mem_copy_no_overlap_impl(dest, src, nbytes);
}

i32 compare(void const * a, void const * b, isize nbytes){
	return mem_compare_impl(a, b, nbytes);
}

uintptr align_forward_ptr(uintptr p, uintptr a){
	debug_assert(valid_alignment(a), "Invalid memory alignment");
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

u32 Allocator::query_capabilites(){
	u32 n = 0;
	_func(_impl, Allocator_Op::Query, nullptr, 0, 0, &n);
	return n;
}

void* Allocator::alloc(isize size, isize align){
	return _func(_impl, Allocator_Op::Alloc, nullptr, size, align, nullptr);
}

void* Allocator::resize(void* ptr, isize new_size){
	return _func(_impl, Allocator_Op::Resize, ptr, new_size, 0, nullptr);
}

void Allocator::free_ex(void* ptr, isize size, isize align){
	_func(_impl, Allocator_Op::Free, ptr, size, align, nullptr);
}

void Allocator::free(void* ptr){
	_func(_impl, Allocator_Op::Free, ptr, 0, 0, nullptr);
}

void Allocator::free_all(){
	_func(_impl, Allocator_Op::Free_All, nullptr, 0, 0, nullptr);
}

void* Allocator::realloc(void* ptr, isize old_size, isize new_size, isize align){
	if(ptr == nullptr){ return nullptr; }

	void* resized_p = this->resize(ptr, new_size);
	if(resized_p != nullptr){
		return resized_p;
	}

	resized_p = this->alloc(new_size, align);
	if(resized_p == nullptr){ // Out of memory
		return nullptr;
	}

	copy(resized_p, ptr, old_size);
	this->free(ptr);
	return resized_p;
}
} /* Namespace mem */

#undef mem_set_impl
#undef mem_copy_impl
#undef mem_copy_no_overlap_impl
#undef mem_compare_impl

//// IO Interface //////////////////////////////////////////////////////////////
namespace io {
i64 Stream::read(Slice<byte> buf){
	return _func(_data, Stream_Op::Read, buf);
}

i64 Stream::write(Slice<byte> buf){
	return _func(_data, Stream_Op::Write, buf);
}

u8 Stream::capabilities(Stream s){
	u8 c = _func(_data, Stream_Op::Query, Slice<byte>());
	return c;
}
} /* Namespace io */

//// UTF-8 /////////////////////////////////////////////////////////////////////
namespace utf8 {
constexpr i32 RANGE1 = 0x7f;
constexpr i32 RANGE2 = 0x7ff;
constexpr i32 RANGE3 = 0xffff;
constexpr i32 RANGE4 = 0x10ffff;

constexpr i32 UTF16_SURROGATE1 = 0xd800;
constexpr i32 UTF16_SURROGATE2 = 0xdfff;

constexpr i32 MASK2 = 0x1f; /* 0001_1111 */
constexpr i32 MASK3 = 0x0f; /* 0000_1111 */
constexpr i32 MASK4 = 0x07; /* 0000_0111 */

constexpr i32 MASKX = 0x3f; /* 0011_1111 */

constexpr i32 SIZE2 = 0xc0; /* 110x_xxxx */
constexpr i32 SIZE3 = 0xe0; /* 1110_xxxx */
constexpr i32 SIZE4 = 0xf0; /* 1111_0xxx */

constexpr i32 CONT = 0x80; /* 10xx_xxxx */

static inline
bool is_continuation_byte(rune c){
	static const rune CONTINUATION1 = 0x80;
	static const rune CONTINUATION2 = 0xbf;
	return (c >= CONTINUATION1) && (c <= CONTINUATION2);
}

Encode_Result encode(rune c){
	Encode_Result res = {0};

	if(is_continuation_byte(c) ||
	   (c >= UTF16_SURROGATE1 && c <= UTF16_SURROGATE2) ||
	   (c > RANGE4))
	{
		return ERROR_ENCODED;
	}

	if(c <= RANGE1){
		res.len = 1;
		res.bytes[0] = c;
	}
	else if(c <= RANGE2){
		res.len = 2;
		res.bytes[0] = SIZE2 | ((c >> 6) & MASK2);
		res.bytes[1] = CONT  | ((c >> 0) & MASKX);
	}
	else if(c <= RANGE3){
		res.len = 3;
		res.bytes[0] = SIZE3 | ((c >> 12) & MASK3);
		res.bytes[1] = CONT  | ((c >> 6) & MASKX);
		res.bytes[2] = CONT  | ((c >> 0) & MASKX);
	}
	else if(c <= RANGE4){
		res.len = 4;
		res.bytes[0] = SIZE4 | ((c >> 18) & MASK4);
		res.bytes[1] = CONT  | ((c >> 12) & MASKX);
		res.bytes[2] = CONT  | ((c >> 6)  & MASKX);
		res.bytes[3] = CONT  | ((c >> 0)  & MASKX);
	}
	return res;
}

constexpr Decode_Result DECODE_ERROR = { .codepoint = ERROR, .len = 0 };

Decode_Result decode(Slice<byte> s){
	Decode_Result res = {0};
	byte* buf = s.raw_data();
	isize len = s.size();

	if(s.empty()){ return DECODE_ERROR; }

	u8 first = buf[0];

	if((first & CONT) == 0){
		res.len = 1;
		res.codepoint |= first;
	}
	else if ((first & ~MASK2) == SIZE2 && len >= 2){
		res.len = 2;
		res.codepoint |= (buf[0] & MASK2) << 6;
		res.codepoint |= (buf[1] & MASKX) << 0;
	}
	else if ((first & ~MASK3) == SIZE3 && len >= 3){
		res.len = 3;
		res.codepoint |= (buf[0] & MASK3) << 12;
		res.codepoint |= (buf[1] & MASKX) << 6;
		res.codepoint |= (buf[2] & MASKX) << 0;
	}
	else if ((first & ~MASK4) == SIZE4 && len >= 4){
		res.len = 4;
		res.codepoint |= (buf[0] & MASK4) << 18;
		res.codepoint |= (buf[1] & MASKX) << 12;
		res.codepoint |= (buf[2] & MASKX) << 6;
		res.codepoint |= (buf[3] & MASKX) << 0;
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

// Steps iterator forward and puts rune and Length advanced into pointers,
// returns false when finished.
bool Iterator::next(rune* r, i8* len){
	if(this->current >= this->data.size()){ return 0; }

	Decode_Result res = decode(this->data.sub(current));
	*r = res.codepoint;
	*len = res.len;

	if(res.codepoint == DECODE_ERROR.codepoint){
		*len = res.len + 1;
	}

	this->current += res.len;

	return 1;
}

// Steps iterator backward and puts rune and its length into pointers,
// returns false when finished.
bool Iterator::prev(rune* r, i8* len){
	if(this->current <= 0){ return false; }

	this->current -= 1;
	while(is_continuation_byte(this->data[this->current])){
		this->current -= 1;
	}

	Decode_Result res = decode(this->data.sub(current));
	*r = res.codepoint;
	*len = res.len;
	return true;
}
} /* Namespace utf8 */

//// Strings ///////////////////////////////////////////////////////////////////
isize String::size() const {
	return _length;
}

// UTF8_Iterator str_iterator(String s){
// }
//
// UTF8_Iterator str_iterator_reversed(String s){
// 	return (UTF8_Iterator){
// 		.current = s.len,
// 		.data_length = s.len,
// 		.data = s.data,
// 	};
// }

utf8::Iterator String::iterator(){
	return {
		.data = Slice<byte>::from_pointer((byte*)_data, _length),
		.current = 0,
	};
}

utf8::Iterator String::iterator_reversed(){
	return {
		.data = Slice<byte>::from_pointer((byte*)_data, _length),
		.current = _length,
	};
}

utf8::Iterator str_iterator_reversed(String s);

isize String::rune_count() const {
}

String String::sub(isize start, isize length);

String String::from_cstr(cstring data){}

String String::from_cstr(cstring data, isize start, isize length);

String String::from_pointer(byte const* data, isize length);
