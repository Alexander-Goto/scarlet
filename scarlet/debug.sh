export SCAR__FAKE_RND=1

find ../std/src/scar -iname "*.scar" -exec scarlet -mt 1 -n std -o std.mscar -s {} + &&
clang -march=native -pthread -std=c23 -DDEBUG -O1 -Wno-empty-body -c ../std/src/c/std.c -o std.o &&
find src -iname "*.scar" -exec scarlet -mt 1 -n main -m std.mscar -o scarlet.c -s {} + &&
clang -march=native -pthread -lm -std=c23 -DEXPORT_CORE_BASIC -DEXPORT_CORE_ERROR -DEXPORT_CORE_STRING -DDEBUG -O1 -Wno-empty-body scarlet.c std.o -o scarlet