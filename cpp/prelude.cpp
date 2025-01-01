#include "prelude.hpp"

//// Spinlock //////////////////////////////////////////////////////////////////
void Spinlock::acquire(){
	for(;;){
		if(!atomic_exchange(&_state, SPINLOCK_LOCKED, Memory_Order::Acquire)){
			break;
		}
		/* Busy wait while locked */
		while(atomic_load(&_state, Memory_Order::Relaxed));
	}
}

bool Spinlock::try_acquire(){
    return !atomic_exchange(&_state, SPINLOCK_LOCKED, Memory_Order::Acquire);
}

void Spinlock::release(){
	atomic_store(&_state, SPINLOCK_UNLOCKED);
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

static inline
bool mem_valid_alignment(isize align){
	return (align & (align - 1)) == 0 && (align != 0);
}

void mem_set(void* p, byte val, isize nbytes){
	mem_set_impl(p, val, nbytes);
}

void mem_copy(void* dest, void const * src, isize nbytes){
	mem_copy_impl(dest, src, nbytes);
}

void mem_copy_no_overlap(void* dest, void const * src, isize nbytes){
	mem_copy_no_overlap_impl(dest, src, nbytes);
}

i32 mem_compare(void const * a, void const * b, isize nbytes){
	return mem_compare_impl(a, b, nbytes);
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

u32 Mem_Allocator::query_capabilites(){
	u32 n = 0;
	_func(_impl, Allocator_Op::Query, nullptr, 0, 0, &n);
	return n;
}

void* Mem_Allocator::alloc(isize size, isize align){
	return _func(_impl, Allocator_Op::Alloc, nullptr, size, align, nullptr);
}

void* Mem_Allocator::resize(void* ptr, isize new_size){
	return _func(_impl, Allocator_Op::Resize, ptr, new_size, 0, nullptr);
}

void Mem_Allocator::free_ex(void* ptr, isize size, isize align){
	return _func(_impl, Allocator_Op::Free, ptr, size, align, nullptr);
}

void Mem_Allocator::free(void* p){
	return _func(_impl, Allocator_Op::Free, ptr, 0, 0, nullptr);
}

void Mem_Allocator::free_all(){
	return _func(_impl, Allocator_Op::Free_All, nullptr, 0, 0, nullptr);
}

void* Mem_Allocator::realloc(void* ptr, isize old_size, isize new_size, isize align){
	if(ptr == nullptr){ return nullptr; }

	void* resized_p = this->resize(ptr, new_size);
	if(resized_p != nullptr){
		return resized_p;
	}

	resized_p = this->alloc(new_size, align);
	if(resized_p == nullptr){ // Out of memory
		return nullptr;
	}

	mem_copy(resized_p, ptr, old_size);
	this->free(ptr);
	return resized_p;
}

#undef mem_set_impl
#undef mem_copy_impl
#undef mem_copy_no_overlap_impl
#undef mem_compare_impl
