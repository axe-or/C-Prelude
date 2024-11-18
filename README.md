# A prelude for C

A *prelude* for a language is essentially a set of definitions that are always included. This header and its accompanying implementation have what I consider to be essentials to writing C code. It is a very opinionated set of features, and is probably not to everyone's taste.

Main features:
- Saner type definitions
- Strings with basic UTF-8 support
- Custom memory allocators that can be swapped out
- Spinlock for quick locking

