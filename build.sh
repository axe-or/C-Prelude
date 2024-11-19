cc='gcc -std=c11'
cflags='-fPIC -g -fno-strict-aliasing -Wall -Wextra'
ignoreflags='-Wno-unused-label'
ldflags=''

set -xe

$cc $cflags $ignoreflags test.c prelude.c -o test.bin

