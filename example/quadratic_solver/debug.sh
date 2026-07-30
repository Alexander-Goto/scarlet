find ../../std/src/scar -iname "*.scar" -exec scarlet -mt 1 -n std -o std.mscar -s {} + &&
clang -march=native -pthread -std=c23 -DDEBUG -O1 -Wno-empty-body -c ../../std/src/c/std.c -o std.o &&
scarlet -n main -m std.mscar -s solver.scar -o solver.c &&
clang -march=native -lm -pthread -DDEBUG -DEXPORT_CORE_BASIC -DEXPORT_CORE_ERROR -DEXPORT_CORE_STRING -std=c23 -O1 -Wno-empty-body std.o solver.c -o solver