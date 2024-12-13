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

int main(){
	printf("Platform: %s\n", TARGET_OS_NAME);
}


