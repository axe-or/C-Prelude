#include "arena_allocator.h"

#include "./assert.h"

static
uintptr arena_required_mem(uintptr cur, isize nbytes, isize align){
	debug_assert(mem_valid_alignment(align), "Alignment must be a power of 2");
	uintptr_t aligned  = align_forward_ptr(cur, align);
	uintptr_t padding  = (uintptr)(aligned - cur);
	uintptr_t required = padding + nbytes;
	return required;
}

static
void *arena_alloc(Mem_Arena* a, isize size, isize align){
	uintptr base = (uintptr)a->data;
	uintptr current = (uintptr)base + (uintptr)a->offset;

	uintptr available = (uintptr)a->capacity - (current - base);
	uintptr required = arena_required_mem(current, size, align);

	if(required > available){
		return NULL;
	}

	a->offset += required;
	void* allocation = &a->data[a->offset - size];
	return allocation;
}

static
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

		case Mem_Op_Resize: {} break;

		case Mem_Op_Free: {} break;

		case Mem_Op_Query: {
			*capabilities = Allocator_Alloc_Any | Allocator_Free_All | Allocator_Align_Any;
		} break;
	}

	return NULL;
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
	a->data = NULL;
}