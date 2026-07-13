export SCAR__FAKE_RND=1

find src -iname "*.scar" -exec scarlet -mt 1 -n main -m ../std/std.mscar -o scarlet.c -s {} + &&
clang -march=native -pthread -lm -std=c23 -DEXPORT_CORE_BASIC -DEXPORT_CORE_ERROR -DEXPORT_CORE_STRING -DDEBUG -O1 -Wno-empty-body scarlet.c ../std/std.o -o scarlet