#include "./string.h"

static const String EMPTY = {0};

bool str_empty(String s){
	return s.len == 0 || s.data == NULL;
}

String str_from(cstring data){
	String s = {
		.data = (byte const *)data,
		.len = cstring_len(data),
	};
	return s;
}

String str_from_bytes(byte const* data, isize length){
	String s = {
		.data = (byte const *)data,
		.len = length,
	};
	return s;
}

String str_concat(String a, String b, Mem_Allocator allocator){
	byte* data = mem_new(byte, a.len + b.len, allocator);
	String s = {0};
	if(data != NULL){
		mem_copy(&data[0], a.data, a.len);
		mem_copy(&data[a.len], b.data, b.len);
		s.data = data;
		s.len = a.len + b.len;
	}
	return s;
}

String str_from_range(cstring data, isize start, isize length){
	String s = {
		.data = (byte const *)&data[start],
		.len = length,
	};
	return s;
}

isize str_codepoint_count(String s){
	UTF8_Iterator it = str_iterator(s);

	isize count = 0;
	Codepoint c; i8 len;
	while(utf8_iter_next(&it, &c, &len)){
		count += 1;
	}
	return count;
}


isize str_codepoint_offset(String s, isize n){
	UTF8_Iterator it = str_iterator(s);
	
	isize acc = 0;

	Codepoint c; i8 len;
	do {
		if(acc == n){ break; }
		acc += 1;
	} while(utf8_iter_next(&it, &c, &len));

	return it.current;
}

// TODO: Handle length in codepoint count
String str_sub(String s, isize start, isize byte_count){
	if(start < 0 || byte_count < 0 || (start + byte_count) > s.len){ return EMPTY; }

	String sub = {
		.data = &s.data[start],
		.len = byte_count,
	};

	return sub;
}

String str_clone(String s, Mem_Allocator allocator){
	char* mem = mem_new(char, s.len, allocator);
	if(mem == NULL){ return EMPTY; }
	return (String){
		.data = (byte const *)mem,
		.len = s.len,
	};
}

bool str_eq(String a, String b){
	if(a.len != b.len){ return false; }

	for(isize i = 0; i < a.len; i += 1){
		if(a.data[i] != b.data[i]){ return false; }
	}

	return true;
}

UTF8_Iterator str_iterator(String s){
	return (UTF8_Iterator){
		.current = 0,
		.data_length = s.len,
		.data = s.data,
	};
}

UTF8_Iterator str_iterator_reversed(String s){
	return (UTF8_Iterator){
		.current = s.len,
		.data_length = s.len,
		.data = s.data,
	};
}

void str_destroy(String s, Mem_Allocator allocator){
	mem_free(allocator, (void*)s.data);
}

#define MAX_CUTSET_LEN 64

String str_trim(String s, String cutset){
	String st = str_trim_leading(str_trim_trailing(s, cutset), cutset);
	return st;
}

String str_trim_leading(String s, String cutset){
	debug_assert(cutset.len <= MAX_CUTSET_LEN, "Cutset string exceeds MAX_CUTSET_LEN");

	Codepoint set[MAX_CUTSET_LEN] = {0};
	isize set_len = 0;
	isize cut_after = 0;

	decode_cutset: {
		Codepoint c; i8 n;
		UTF8_Iterator iter = str_iterator(cutset);

		isize i = 0;
		while(utf8_iter_next(&iter, &c, &n) && i < MAX_CUTSET_LEN){
			set[i] = c;
			i += 1;
		}
		set_len = i;
	}

	strip_cutset: {
		Codepoint c; i8 n;
		UTF8_Iterator iter = str_iterator(s);

		while(utf8_iter_next(&iter, &c, &n)){
			bool to_be_cut = false;
			for(isize i = 0; i < set_len; i += 1){
				if(set[i] == c){
					to_be_cut = true;
					break;
				}
			}

			if(to_be_cut){
				cut_after += n;
			}
			else {
				break; // Reached first Codepoint that isn't in cutset
			}

		}
	}

	return str_sub(s, cut_after, s.len - cut_after);
}

String str_trim_trailing(String s, String cutset){
	debug_assert(cutset.len <= MAX_CUTSET_LEN, "Cutset string exceeds MAX_CUTSET_LEN");

	Codepoint set[MAX_CUTSET_LEN] = {0};
	isize set_len = 0;
	isize cut_until = s.len;

	decode_cutset: {
		Codepoint c; i8 n;
		UTF8_Iterator iter = str_iterator(cutset);

		isize i = 0;
		while(utf8_iter_next(&iter, &c, &n) && i < MAX_CUTSET_LEN){
			set[i] = c;
			i += 1;
		}
		set_len = i;
	}

	strip_cutset: {
		Codepoint c; i8 n;
		UTF8_Iterator iter = str_iterator_reversed(s);

		while(utf8_iter_prev(&iter, &c, &n)){
			bool to_be_cut = false;
			for(isize i = 0; i < set_len; i += 1){
				if(set[i] == c){
					to_be_cut = true;
					break;
				}
			}

			if(to_be_cut){
				cut_until -= n;
			}
			else {
				break; // Reached first Codepoint that isn't in cutset
			}

		}
	}

	return str_sub(s, 0, cut_until);
}