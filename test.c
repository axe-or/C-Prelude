#include "prelude.h"
#include <stdio.h>

#define MEM_SIZE (400ll)

typedef struct IO_String_Reader IO_String_Reader;

struct IO_String_Reader {
	String source;
	isize  current;
};

static
i64 io_string_func(void* impl, byte op, byte* buf, isize buflen){
	if(buflen <= 0){ return 0; }

	struct IO_String_Reader* reader = impl;
	enum IO_Op operation = op;

	switch(operation){
		case IO_Query: {
			return IO_Stream_Read;
		} break;

		case IO_Read: {
			isize remaining = reader->source.len - reader->current;
			if(remaining <= 0){
				return IO_Err_End_Of_Stream;
			}
			UTF8_Decode_Result res = utf8_decode(&reader->source.data[reader->current], remaining);
			if(res.codepoint != UTF8_ERROR){
				isize to_copy = min(buflen, res.len);
				mem_copy(buf, &reader->source.data[reader->current], res.len);
				reader->current += res.len;
				return to_copy;
			}
			else {
				UTF8_Encode_Result error = UTF8_ERROR_ENCODED;
				isize to_copy = min(buflen, error.len);
				mem_copy(buf, error.bytes, to_copy);
				return to_copy;
			}
		} break;

		case IO_Write: {
			return IO_Err_Unsupported;
		};

		default: panic("Bad enum access");
	}

	return IO_Err_Unsupported;
}

IO_String_Reader io_string_reader_make(String s);

IO_String_Reader io_string_reader_make(String s){
	return (IO_String_Reader){
		.source = s,
		.current = 0,
	};
}

IO_Stream io_string_reader_stream(IO_String_Reader* r){
	return (IO_Stream){
		.data = r,
		.func = io_string_func,
	};
}

int main(){
	static byte memory[MEM_SIZE];
	printf("zamn..");
}

