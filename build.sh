cc='gcc -std=c11'
cflags='-D_XOPEN_SOURCE=500 -fPIC -fno-strict-aliasing -Wall -Wextra'
ignoreflags='-Wno-unused-label'
ldflags=''

set -xe

$cc $cflags $ignoreflags test.c prelude.c -o test.bin

