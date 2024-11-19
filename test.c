#include "prelude.h"
#include <stdio.h>

#define ARENA_MEM_SIZE 4096ll
static byte memory[ARENA_MEM_SIZE];

int main(){
    Mem_Arena arena;
    arena_init(&arena, memory, ARENA_MEM_SIZE);
    Mem_Allocator allocator = arena_allocator(&arena);
    
    i32* num0 = arena_alloc(&arena, 20, alignof(i32));
    i32* num1 = arena_alloc(&arena, 20, alignof(i32));
    
    debug_assert(arena_resize(&arena, num0, 40) == null, "");
    debug_assert(arena_resize(&arena, num1, 40) != null, "");
}

