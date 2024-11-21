#include "prelude.h"

#define ARENA_MEM_SIZE (4096ll * 4096ll)
static byte ARENA_MEMORY[ARENA_MEM_SIZE];

typedef struct Mem_Pool_Allocator Mem_Pool_Allocator;
typedef struct Mem_Pool_Node Mem_Pool_Node;

struct Mem_Pool_Node {
	Mem_Pool_Node* next;
};

struct Mem_Pool_Allocator {
	void* data;
	isize capacity;
	isize pool_size;
	isize pool_align;
	Mem_Pool_Node* free_list;
};

void pool_init(Mem_Pool_Allocator* a, byte* buffer, isize buflen, isize pool_size, isize alignment){
	isize capacity = buflen;
	uintptr original_start = (uintptr)buffer;
	uintptr aligned_start = align_forward_ptr(original_start, alignment);
	capacity -= (isize)(aligned_start - original_start);

	isize align_pool_size = align_forward_size(pool_size, alignment);
	panic_assert((usize)pool_size >= sizeof(Mem_Pool_Node), "Pool size is too small");
	panic_assert(capacity >= pool_size, "Buffer is too small to hold at least one pool");
}

// void* pool_alloc(Mem_Pool_Allocator* a);

// void* pool_free(Mem_Pool_Allocator* a, void* ptr);

// void* pool_free_all(Mem_Pool_Allocator* a, void* ptr);

int main(){
	Mem_Arena arena;
	arena_init(&arena, ARENA_MEMORY, ARENA_MEM_SIZE);
	Mem_Allocator allocator = arena_allocator(&arena);

    Console_Logger* cl = log_create_console_logger(allocator);
	Logger logger = log_console_logger(cl, 0);

	for(int i = 0; i < 20; i++){
		time_sleep(250 * time_millisecond);
		log_info(logger, "yes");
	}

}

