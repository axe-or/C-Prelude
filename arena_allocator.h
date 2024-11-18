#pragma once

#include "./prelude.h"
#include "./memory.h"

typedef struct {
	isize offset;
	isize capacity;
	byte* data;
} Mem_Arena;

// Initialize a memory arena with a buffer
void arena_init(Mem_Arena* a, byte* data, isize len);

// Deinit the arena
void arena_destroy(Mem_Arena *a);

// Get arena as a conforming instance to the allocator interface
Mem_Allocator arena_allocator(Mem_Arena* a);
