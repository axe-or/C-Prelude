#include "prelude.h"
#include <stdio.h>

#define TARGET_OS_LINUX 1

//// Platform //////////////////////////////////////////////////////////////////
#if defined(TARGET_OS_WINDOWS)
#define TARGET_OS_NAME "Windows"
#elif defined(TARGET_OS_LINUX)
#define TARGET_OS_NAME "Linux"
#else
#error "Platform macro is not defined, this means you probably forgot to define it or this platform is not suported."
#endif

typedef struct Mem_Pool Mem_Pool;
typedef struct Mem_Pool_Node Mem_Pool_Node;

struct Mem_Pool {
	byte* data;
	isize capacity;
	isize node_size;
	Mem_Pool_Node* free_list;
};

struct Mem_Pool_Node {
	Mem_Pool_Node* next;
};

// Initialize pool allocator with nodes of a particular size and alignment.
// Returns success status
bool pool_init(Mem_Pool* pool, byte* data, isize len, isize node_size, isize node_alignment);

// Mark all the pool's allocations as freed
void pool_free_all(Mem_Pool* pool);

// Mark specified pointer returned by `pool_alloc` as free again
void pool_free(Mem_Pool* pool, void* ptr);

// Allocate one node from pool, returns null on failure
void* pool_alloc(Mem_Pool* pool);

// Get pool as a conforming instance to the allocator interface
Mem_Allocator pool_allocator(Mem_Pool* pool);

bool pool_init(Mem_Pool* pool, byte* data, isize len, isize node_size, isize node_alignment){
	uintptr unaligned_start = (uintptr)data;
	uintptr start = align_forward_ptr(unaligned_start, node_alignment);
	len -= (isize)(start - unaligned_start);

	node_size = align_forward_size(node_size, node_alignment);

	debug_assert(node_size >= (isize)sizeof(Mem_Pool_Node*), "Size of node is too small");
	debug_assert(len >= node_size, "Buffer length is too small");

	pool->data = (byte*)start; // or data?
	pool->capacity = len;
	pool->node_size = node_size;
	pool->free_list = null;
	pool_free_all(pool);

	return true;
}

void pool_free_all(Mem_Pool* pool){
	isize node_count = pool->capacity / pool->node_size;

	for(isize i = 0; i < node_count; i += 1){
		void* p = &pool->data[i * node_count];
		Mem_Pool_Node* node = (Mem_Pool_Node*)p;
		node->next = pool->free_list;
		pool->free_list = node;
	}
}

void* pool_alloc(Mem_Pool* pool){
	Mem_Pool_Node* node = pool->free_list;

	if(node == null){
		return null;
	}

	pool->free_list = pool->free_list->next;
	mem_set(node, 0, pool->node_size);
	return (void*)node;
}

static bool pool_owns_pointer(Mem_Pool* pool, void* ptr){
	uintptr begin = (uintptr)pool->data;
	uintptr end = (uintptr)(&pool->data[pool->capacity]);
	uintptr p = (uintptr)ptr;
	return p >= begin && p < end;
}

void pool_free(Mem_Pool* pool, void* ptr){
	if(ptr == null){ return; }

	debug_assert(pool_owns_pointer(pool, ptr), "Pointer is not owned by allocator");

	Mem_Pool_Node* node = (Mem_Pool_Node*)ptr;
	node->next = pool->free_list;
	pool->free_list = node;
}

static
void* pool_allocator_func (
	void * restrict impl,
	enum Allocator_Op op,
	void* old_ptr,
	isize size, isize align,
	i32* capabilities
){
	Mem_Pool* p = (Mem_Pool*) impl;
	(void)align; // Pool has fixed alignment

	switch (op) {
		case Mem_Op_Query:{
			*capabilities = Allocator_Free_All | Allocator_Free_Any | Allocator_Resize;
		} break;

		case Mem_Op_Alloc:
			return pool_alloc(p);

		case Mem_Op_Resize: {
			debug_assert(pool_owns_pointer(p, old_ptr), "Pointer is not owned by allocator");
			if(size <= p->node_size){
				return old_ptr;
			} else {
				return null;
			}
		} break;

		case Mem_Op_Free: {
			pool_free(p, old_ptr);
		} break;

		case Mem_Op_Free_All: {
			pool_free_all(p);
		} break;
	}
	return null;
}

Mem_Allocator pool_allocator(Mem_Pool* pool){
	return (Mem_Allocator){
		.data = pool,
		.func = pool_allocator_func,
	};
}

int main(){
	printf("Platform: %s\n", TARGET_OS_NAME);
}


