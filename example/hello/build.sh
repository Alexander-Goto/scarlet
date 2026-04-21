scarlet -n main -s hello.scar -o hello.c &&
clang -march=native -pthread -lm -std=c23 -O3 -Wno-empty-body hello.c -o hello