#include "./memory.h"

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
	if(capabilities == NULL){ return 0; }
	allocator.func(allocator.data, Mem_Op_Query, NULL, 0, 0, capabilities);
	return *capabilities;
}

void* mem_alloc(Mem_Allocator allocator, isize size, isize align){
	void* ptr = allocator.func(allocator.data, Mem_Op_Alloc, NULL, size, align, NULL);
	if(ptr != NULL){
		mem_set(ptr, 0, size);
	}
	return ptr;
}

void* mem_resize(Mem_Allocator allocator, void* ptr, isize new_size){
	void* new_ptr = allocator.func(allocator.data, Mem_Op_Resize, ptr, new_size, 0, NULL);
	return new_ptr;
}

void mem_free_ex(Mem_Allocator allocator, void* p, isize align){
	if(p == NULL){ return; }
	allocator.func(allocator.data, Mem_Op_Free, p, 0, align, NULL);
}

void mem_free(Mem_Allocator allocator, void* p){
	mem_free_ex(allocator, p, 0);
}

void mem_free_all(Mem_Allocator allocator){
	allocator.func(allocator.data, Mem_Op_Free_All, NULL, 0, 0, NULL);
}
