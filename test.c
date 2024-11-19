#include "prelude.h"
#include <stdio.h>

static byte memory[4096];

int main(){
    Mem_Arena arena;
    arena_init(&arena, memory, 4096);
    Mem_Allocator allocator = arena_allocator(&arena);
    
    i32* num0 = arena_alloc(&arena, 20, alignof(i32));
    i32* num1 = arena_alloc(&arena, 20, alignof(i32));
    
    debug_assert(arena_resize(&arena, num0, 40) == null, "");
    debug_assert(arena_resize(&arena, num1, 40) != null, "");
}

