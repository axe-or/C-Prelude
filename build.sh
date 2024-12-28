cc='gcc -std=c11'
cflags='-DTARGET_OS_LINUX -fPIC -fno-strict-aliasing -Wall -Wextra'
ignoreflags='-Wno-unused-label'
ldflags=''

set -xe

$cc $cflags $ignoreflags test.c prelude.c -o test.bin

