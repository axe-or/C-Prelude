#define _XOPEN_SOURCE 500
#include "prelude.h"
#define ARENA_MEM_SIZE (4096ll * 4096ll)
static byte ARENA_MEMORY[ARENA_MEM_SIZE];

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

typedef struct String_Builder String_Builder;

struct String_Builder {
	byte* data;
	isize len;
	isize cap;
	Mem_Allocator allocator;
};

bool sb_init(String_Builder* sb, Mem_Allocator allocator, isize initial_cap){
	sb->allocator = allocator;
	sb->len = 0;
	sb->data = mem_new(byte, initial_cap, allocator);
	sb->cap = initial_cap;
	return sb->data != null;
}

void sb_destroy(String_Builder* sb){
	mem_free_ex(sb->allocator, sb->data, sb->cap, alignof(byte));
}

void* mem_realloc(Mem_Allocator allocator, void* ptr, isize old_size, isize new_size, isize align){
	if(mem_resize(allocator, ptr, new_size) != null){
		return ptr; /* In-place resize successful */
	}

	void* new_data = mem_alloc(allocator, new_size, align);
	if(new_data != null){
		mem_copy_no_overlap(new_data, ptr, min(old_size, new_size));
		mem_free_ex(allocator, ptr, old_size, align);
	}
	return new_data;
}

// Append buffer of bytes to the end of builder. Returns < 0 if an error occours
// and number of bytes added otherwhise
isize sb_append_bytes(String_Builder* sb, byte const* buf, isize nbytes){
	if((sb->len + nbytes) > sb->cap){
		byte* new_data = mem_realloc(sb->allocator, sb->data, sb->cap, max(16, (sb->cap * 7) / 4), alignof(byte));
		if(new_data == null){ return -1; }
	}
	mem_copy(&sb->data[sb->len], buf, nbytes);
	sb->len += nbytes;
	return nbytes;
}

isize sb_append_str(String_Builder* sb, String s){
	return sb_append_bytes(sb, s.data, s.len);
}

isize sb_append_rune(String_Builder* sb, rune r){
	UTF8_Encode_Result enc = utf8_encode(r);
	return sb_append_bytes(sb, enc.bytes, enc.len);
}

#define MAX_INT_CHAR_LEN (sizeof("-9223372036854775808") + 1)
isize sb_append_integer(String_Builder* sb, i64 num, bool force_sign){
	byte buf[MAX_INT_CHAR_LEN] = {0};
	isize pos = 0;

	if(num < 0ll){
		buf[pos] = '-';
		pos += 1;
	} else if(force_sign){
		buf[pos] = '+';
		pos += 1;
	}

	i64 n = abs(num);
	while(n > 0ll){
		char d = '0' + (n % 10ll);
		buf[pos] = d;
		pos += 1;
		n /= 10ll;
	}

	return sb_append_bytes(sb, buf, pos);
}

int main(){
}

