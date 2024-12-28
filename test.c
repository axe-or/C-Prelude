#include "prelude.h"
#include <stdio.h>

#define MEM_SIZE (400ll)

int main(){
	bool ok = 0;

	ok = str_ends_with(str_lit("Some/Path.json"), str_lit(".json"));
	printf("%d\n", ok);
	ok = str_starts_with(str_lit(""), str_lit("Some/Path.json"));
	printf("%d\n", ok);
}

