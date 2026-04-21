find src/scar -iname "*.scar" -exec scarlet -n std -o std.mscar -s {} + &&
clang -march=native -pthread -std=c23 -O3 -Wno-empty-body -c src/c/std.c -o std.o