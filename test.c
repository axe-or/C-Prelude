#include "prelude.h"

// typedef struct Mem_Pool_Allocator Mem_Pool_Allocator;
// typedef struct Mem_Pool_Node Mem_Pool_Node;
//
// struct Mem_Pool_Node {
// 	Mem_Pool_Node* next;
// };
//
// struct Mem_Pool_Allocator {
// 	void* data;
// 	isize capacity;
// 	isize pool_size;
// 	isize pool_align;
// 	Mem_Pool_Node* free_list;
// };
//
// void pool_init(Mem_Pool_Allocator* a, byte* buffer, isize buflen, isize pool_size, isize alignment){
// 	isize capacity = buflen;
// 	uintptr original_start = (uintptr)buffer;
// 	uintptr aligned_start = align_forward_ptr(original_start, alignment);
// 	capacity -= (isize)(aligned_start - original_start);
//
// 	isize align_pool_size = align_forward_size(pool_size, alignment);
// 	panic_assert((usize)pool_size >= sizeof(Mem_Pool_Node), "Pool size is too small");
// 	panic_assert(capacity >= pool_size, "Buffer is too small to hold at least one pool");
// }
//
// // void* pool_alloc(Mem_Pool_Allocator* a);
//
// // void* pool_free(Mem_Pool_Allocator* a, void* ptr);
//
// // void* pool_free_all(Mem_Pool_Allocator* a, void* ptr);


#define ARENA_MEM_SIZE (4096ll * 4096ll)
static byte ARENA_MEMORY[ARENA_MEM_SIZE];
#include <stdio.h>

//// Dynamic Loading ///////////////////////////////////////////////////////////
// #if PRELUDE_DYN_LOAD
// #else
// #endif

typedef struct Lib_Handle Lib_Handle;
#include <dlfcn.h>

struct Lib_Handle {
	uintptr _data;
};

static inline
bool lib_ok(Lib_Handle h){
	return h._data != 0;
}

#define MAX_LIB_SYMBOL_SIZE 4096

Lib_Handle lib_open(String path){
	char pathbuf[MAX_LIB_SYMBOL_SIZE] = {0};
	mem_copy_no_overlap(pathbuf, path.data, min(MAX_LIB_SYMBOL_SIZE - 1, path.len));
	void* res = dlopen(pathbuf, RTLD_LOCAL | RTLD_LAZY);
	return (Lib_Handle){
		._data = (uintptr)res,
	};
}

void lib_close(Lib_Handle h){
	if(h._data == 0){ return; }
	dlclose((void*)h._data);
}

void* lib_load_symbol(Lib_Handle h, String sym){
	char namebuf[MAX_LIB_SYMBOL_SIZE] = {0};
	mem_copy_no_overlap(namebuf, sym.data, min(MAX_LIB_SYMBOL_SIZE - 1, sym.len));

	void* res = dlsym((void*)h._data, namebuf);
	return res;
}

int main(){
	Mem_Arena arena = {0};
	arena_init(&arena, ARENA_MEMORY, ARENA_MEM_SIZE);
	// Mem_Allocator allocator = arena_allocator(&arena);
	Mem_Allocator allocator = libc_allocator();

	String_Builder builder = {0};
	panic_assert(sb_init(&builder, allocator, 32), "Failed to init builder");
	String s = sb_build(&builder);
	printf(">> '%.*s'", fmt_bytes(s));
}


