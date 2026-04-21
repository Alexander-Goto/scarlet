../../scarlet/scarlet -n main -m ../../std/std.mscar -s \
test.scar \
src/helpers/heap_data.scar \
src/helpers/other.scar \
src/gconsts.scar \
src/change_dir.scar \
src/current_dir.scar \
src/delete_empty_dir.scar \
src/delete_file.scar \
src/fs_path.scar \
src/list_dir.scar \
src/make_dir.scar \
src/file.scar \
src/is_root.scar \
src/run_app.scar \
-o test.c &&
clang -march=native -pthread -lm -std=c23 -DEXPORT_CORE_BASIC -DEXPORT_CORE_ERROR -DEXPORT_CORE_STRING -DFULL_DEBUG -fsanitize=address -fsanitize=undefined -g -O0 -Wno-empty-body ../../std/std.o test.c -o test &&
./test
