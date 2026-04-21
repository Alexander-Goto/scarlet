#pragma once

#include "bool_vectors.h"
#include "const.h"
#include "include.h"
#include "macro.h"
#include "type.h"

[[gnu::cold, noreturn]]
static void fail(const char* const msg) {
     fprintf(stderr, "%s\n", msg);
     exit(EXIT_FAILURE);
}

static inline void* safe_realloc(void* const pointer, u64 const size) {
     assume(size != 0);

     void* const result = realloc(pointer, size);
     if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");
     return result;
}

static inline void* safe_aligned_realloc(u64 const align, void* const pointer, u64 const size) {
     assume(__builtin_popcountll(align) == 1 && align % sizeof(void*) == 0);
     assume(size != 0);
     assume_aligned(pointer, align);

     if (pointer != nullptr) {
          void* const result = realloc(pointer, size);
          if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");

          if (result == pointer || (u64)result % align == 0) return result;

          void* const new_mem = aligned_alloc(align, size);

          memcpy(new_mem, result, size);
          free(result);
          return new_mem;
     }

     void* const result = aligned_alloc(align, size);
     if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");

     return result;
}

static inline void* safe_malloc(u64 const size) {
     assume(size != 0);

     void* const result = malloc(size);
     if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");
     return result;
}

static inline void* safe_aligned_alloc(u64 const align, u64 const size) {
     assume(__builtin_popcountll(align) == 1 && align % sizeof(void*) == 0);
     assume(size != 0);

     void* const result = aligned_alloc(align, size);
     if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");

     assume_aligned(result, align);

     return result;
}

static inline char* safe_strdup(const char* const string) {
     assume(string != nullptr);

     char* const result = __builtin_strdup(string);
     if (result == nullptr) [[clang::unlikely]] fail("Not enough memory.");
     return result;
}

static inline void safe_mtx_init(mtx_t* const mtx, int const type) {
     if (mtx_init(mtx, type) != thrd_success) [[clang::unlikely]] fail("Failed to create mutex.");
}

static inline void safe_cnd_init(cnd_t* const cnd) {
     if (cnd_init(cnd) != thrd_success) [[clang::unlikely]] fail("Failed to create condition variable.");
}

static inline void safe_cnd_broadcast(cnd_t* const cnd) {
     if (cnd_broadcast(cnd) != thrd_success) [[clang::unlikely]] fail("Can't unblock a condition variable.");
}

static inline void safe_cnd_signal(cnd_t* const cnd) {
     if (cnd_signal(cnd) != thrd_success) [[clang::unlikely]] fail("Can't unblock a condition variable.");
}

static inline void safe_cnd_wait(cnd_t* const cnd, mtx_t* const mtx) {
     if (cnd_wait(cnd, mtx) != thrd_success) [[clang::unlikely]] fail("Can't block a condition variable.");
}

static inline void safe_thrd_create(thrd_t* const thrd, thrd_start_t const fn, void* const arg) {
     if (thrd_create(thrd, fn, arg) != thrd_success) [[clang::unlikely]] fail("Failed to create thread.");
}

static inline int nointr_fclose(FILE* const stream) {
     #pragma nounroll
     while (true) {
          int const result = fclose(stream);
          if (result == 0 || errno != EINTR) return result;
          clearerr(stream);
     }
}

static inline int nointr_fflush(FILE* const stream) {
     #pragma nounroll
     while (true) {
          int const result = fflush(stream);
          if (result == 0 || errno != EINTR) return result;
          clearerr(stream);
     }
}

static inline FILE* nointr_fopen(const char* const pathname, const char* const mode) {
     #pragma nounroll
     while (true) {
          FILE* const result = fopen(pathname, mode);
          if (result != nullptr || errno != EINTR) return result;
     }
}

static inline size_t nointr_fread(void* const ptr, size_t const size, size_t const nitems, FILE* const stream) {
     size_t already_read_len = 0;

     #pragma nounroll
     while (true) {
          size_t const one_take_readed_len = fread(&((u8*)ptr)[already_read_len * size], size, nitems - already_read_len, stream);
          already_read_len                += one_take_readed_len;
          if (already_read_len == nitems || feof(stream) || errno != EINTR) return already_read_len;
          clearerr(stream);
     }
}

static inline int nointr_fseek(FILE* const stream, long const offset, int const whence) {
     #pragma nounroll
     while (true) {
          int const result = fseek(stream, offset, whence);
          if (result == 0 || errno != EINTR) return result;
          clearerr(stream);
     }
}

static inline size_t nointr_fwrite(const void* const ptr, size_t const size, size_t const nitems, FILE* const stream) {
     size_t already_written_len = 0;

     #pragma nounroll
     while (true) {
          size_t const one_take_readed_len = fwrite(&((u8*)ptr)[already_written_len * size], size, nitems - already_written_len, stream);
          already_written_len             += one_take_readed_len;
          if (already_written_len == nitems || feof(stream) || errno != EINTR) return already_written_len;
          clearerr(stream);
     }
}

static inline int nointr_thrd_sleep(const struct timespec* duration, struct timespec* const remain) {
     #pragma nounroll
     while (true) {
          int const result = thrd_sleep(duration, remain);
          if (result == 0 || errno != EINTR) return result;
          duration = remain;
     }
}

#define realloc(p, size)                      safe_realloc(p, size)
#define aligned_realloc(align, pointer, size) safe_aligned_realloc(align, pointer, size)
#define malloc(size)                          safe_malloc(size)
#define aligned_alloc(align, size)            safe_aligned_alloc(align, size)
#define strdup(string)                        safe_strdup(string)
#define mtx_init(mtx, type)                   safe_mtx_init(mtx, type)
#define cnd_init(cnd)                         safe_cnd_init(cnd)
#define cnd_broadcast(cnd)                    safe_cnd_broadcast(cnd)
#define cnd_signal(cnd)                       safe_cnd_signal(cnd)
#define cnd_wait(cnd, mtx)                    safe_cnd_wait(cnd, mtx)
#define thrd_create(thrd, fn, arg)            safe_thrd_create(thrd, fn, arg)
#define fclose(stream)                        nointr_fclose(stream)
#define fflush(stream)                        nointr_fflush(stream)
#define fopen(pathname, mode)                 nointr_fopen(pathname, mode)
#define fread(ptr, size, nitems, stream)      nointr_fread(ptr, size, nitems, stream)
#define fseek(stream, offset, whence)         nointr_fseek(stream, offset, whence)
#define fwrite(ptr, size, nitems, stream)     nointr_fwrite(ptr, size, nitems, stream)
#define thrd_sleep(duration, remain)          nointr_thrd_sleep(duration, remain)

[[clang::always_inline]]
static u64 cast_double_to_u64(double const double_) {
     return *(const u64*)&double_;
}

[[clang::always_inline]]
static double cast_u64_to_double(u64 const u64_) {
     return *(const double*)&u64_;
}

[[clang::always_inline]]
static t_ptr64 ptr_to_ptr64(const void* const ptr) {
     return (const t_ptr64){.bits = (u64)ptr};
}

[[clang::always_inline]]
static u64 get_ref_cnt(t_any const arg) {
     return *(u64*)arg.qwords[0] & ref_cnt_bit_mask;
}

[[clang::always_inline]]
static void set_ref_cnt(t_any const arg, u64 const ref_cnt) {
     *(u64*)arg.qwords[0] = *(u64*)arg.qwords[0] & (u64)-1 << ref_cnt_bits | ref_cnt;
}

static inline u64 linear__alloc(t_linear_allocator* const allocator, u64 const size, u64 const align) {
     assert((i64)size > 0);
     assert(align < 17);
     assert(__builtin_popcountll(align) == 1);

     u64 const spacer_size  = allocator->mem == nullptr || ((u64)(allocator->mem) & align - 1) == 0 ? 0 : align - ((u64)(allocator->mem) & align - 1);
     u64 const new_mem_size = allocator->mem_size + size + spacer_size;

     assert(new_mem_size > allocator->mem_size);

     if (new_mem_size > allocator->mem_cap) {
          allocator->mem_cap = new_mem_size;
          allocator->mem     = aligned_realloc(16, allocator->mem, new_mem_size);
     }

     u64 const new_states_qty = spacer_size != 0 ? 2 : 1;
     u64 const idx            = allocator->states_len + new_states_qty - 1;
     if (idx >= allocator->states_cap) {
          u64 const new_cap     = allocator->states_cap + 4;
          allocator->states_cap = new_cap;
          allocator->states     = realloc(allocator->states, new_cap * sizeof(i64));
     }

     if (spacer_size != 0) {
          allocator->states[allocator->states_len] = allocator->mem_size | (u64)1 << 63;
          allocator->mem_size                     += spacer_size;
          allocator->states_len                   += 1;
     }

     allocator->states[idx] = allocator->mem_size;
     allocator->states_len += 1;
     allocator->mem_size    = new_mem_size;

     return idx;
}

[[clang::always_inline]]
static void* linear__get_mem_of_idx(const t_linear_allocator* const allocator, u64 const idx) {
     return &((u8*)allocator->mem)[allocator->states[idx]];
}

[[clang::always_inline]]
static void* linear__get_mem_of_last(const t_linear_allocator* const allocator) {
     return &((u8*)allocator->mem)[allocator->states[allocator->states_len - 1]];
}

[[clang::always_inline]]
static bool linear__idx_is_last(const t_linear_allocator* const allocator, u64 const idx) {
     return allocator->states_len - 1 == idx;
}

static inline void linear__last_realloc(t_linear_allocator* const allocator, u64 const size) {
     assert((i64)size > 0);

     u64 const last_idx     = allocator->states_len - 1;
     u64 const new_mem_size = allocator->states[last_idx] + size;
     if (new_mem_size > allocator->mem_cap) {
          allocator->mem_cap = new_mem_size;
          allocator->mem     = aligned_realloc(16, allocator->mem, new_mem_size);
     }
     allocator->mem_size = new_mem_size;
}

static inline void linear__free(t_linear_allocator* const allocator, u64 const idx) {
     assert(idx < allocator->states_len);

     if (allocator->states_len - 1 == idx) {
          allocator->mem_size = allocator->states[idx];
          i64 look_idx        = idx - 1;
          for (; look_idx != -1 && allocator->states[look_idx] < 0; allocator->mem_size = allocator->states[look_idx] & (u64)-1 >> 1, look_idx -= 1);
          allocator->states_len = look_idx + 1;
     } else allocator->states[idx] |= (u64)1 << 63;
}

#ifdef CALL_STACK
[[clang::always_inline]]
static void call_stack__next_is_tail_call(t_thrd_data* const thrd_data) {thrd_data->call_stack.next_is_tail_call = true;}

static inline void call_stack__push(t_thrd_data* const thrd_data, const char* const item) {
     u64 const stack_len = thrd_data->call_stack.len;
     if (stack_len == thrd_data->call_stack.cap) {
          u64 const cap = thrd_data->call_stack.cap * 2 | 8;
          thrd_data->call_stack.cap             = cap;
          thrd_data->call_stack.stack           = realloc(thrd_data->call_stack.stack, cap * sizeof(char*));
          thrd_data->call_stack.tail_call_flags = realloc(thrd_data->call_stack.tail_call_flags, cap >> 3);
     }

     u64 const tail_call_bucket_idx = stack_len >> 3;
     u8  const tail_call_mask       = (u8)1 << ((u8)stack_len & (u8)7);
     if (thrd_data->call_stack.next_is_tail_call) {
          thrd_data->call_stack.tail_call_flags[tail_call_bucket_idx] |= tail_call_mask;
          thrd_data->call_stack.next_is_tail_call                      = false;
     } else thrd_data->call_stack.tail_call_flags[tail_call_bucket_idx] &= ~tail_call_mask;

     thrd_data->call_stack.stack[stack_len] = item;
     thrd_data->call_stack.len              = stack_len + 1;
}

static inline void call_stack__pop(t_thrd_data* const thrd_data) {
     u64 stack_len = thrd_data->call_stack.len;

     #pragma nounroll
     while (true) {
          stack_len -= 1;

          u64  const tail_call_bucket_idx = stack_len >> 3;
          u8   const tail_call_mask       = (u8)1 << ((u8)stack_len & (u8)7);
          bool const is_not_tail_call     = (thrd_data->call_stack.tail_call_flags[tail_call_bucket_idx] & tail_call_mask) == 0;
          if (is_not_tail_call) {
               thrd_data->call_stack.len = stack_len;
               return;
          }
     }
}

[[clang::always_inline]]
static void call_stack__pop_if_tail_call(t_thrd_data* const thrd_data) {
     bool const is_tail_call = thrd_data->call_stack.next_is_tail_call;
     if (is_tail_call) {
          thrd_data->call_stack.next_is_tail_call = false;
          call_stack__pop(thrd_data);
     }
}

[[clang::noinline]]
static void call_stack__show(u64 const stack_len, const char* const* const stack, FILE* const out) {
     assert(stack_len != 0);

     fprintf(out, "[CALL STACK]\n%s\n", stack[0]);
     for (u64 idx = 1; idx < stack_len; idx += 1)
          fprintf(out, "-> %s\n", stack[idx]);
     fprintf(out, "\n");
}
#else
#define call_stack__next_is_tail_call(thrd_data) ((void)0)
#define call_stack__push(thrd_data, item) ((void)0)
#define call_stack__pop(thrd_data) ((void)0)
#define call_stack__pop_if_tail_call(thrd_data) ((void)0)
#define call_stack__show(stack_len, stack, out) ((void)0)
#endif

[[gnu::cold, noreturn]]
static void fail_with_call_stack(t_thrd_data* const thrd_data, const char* const msg, const char* const owner) {
#ifdef CALL_STACK
     call_stack__show(thrd_data->call_stack.len, thrd_data->call_stack.stack, stderr);
#endif

     fprintf(stderr, "%s (%s)\n", msg, owner);
     exit(EXIT_FAILURE);
}

[[clang::always_inline]]
static int scar__memcmp(const void* const mem_1, const void* const mem_2, u64 const len) {
     const u8* const mem_1_u8 = (const u8*)mem_1;
     const u8* const mem_2_u8 = (const u8*)mem_2;

     typedef u8 t_vec_64 __attribute__((ext_vector_type(64), aligned(1)));
     typedef u8 t_vec_32 __attribute__((ext_vector_type(32), aligned(1)));
     typedef u8 t_vec_16 __attribute__((ext_vector_type(16), aligned(1)));

     u64 idx = 0;
     for (; len - idx >= 64; idx += 64) {
          u64 const cmp_result = v_64_bool_to_u64(__builtin_convertvector(*(const t_vec_64*)&mem_1_u8[idx] != *(const t_vec_64*)&mem_2_u8[idx], v_64_bool));
          if (cmp_result != 0) {
               idx += __builtin_ctzll(cmp_result);
               return mem_1_u8[idx] > mem_2_u8[idx] ? 1 : -1;
          }
     }

     u8 const remain_bytes = len - idx;
     switch (remain_bytes < 3 ? remain_bytes : sizeof(int) * 8 - __builtin_clz(remain_bytes - 1)) {
     case 6: {
          assert(remain_bytes > 32 && remain_bytes < 64);

          u64 const cmp_result =
               v_32_bool_to_u32(__builtin_convertvector(*(const t_vec_32*)&mem_1_u8[idx] != *(const t_vec_32*)&mem_2_u8[idx], v_32_bool)) |
               (u64)v_32_bool_to_u32(__builtin_convertvector(*(const t_vec_32*)&mem_1_u8[len - 32] != *(const t_vec_32*)&mem_2_u8[len - 32], v_32_bool)) << (len - 32 - idx);

          if (cmp_result != 0) {
               idx += __builtin_ctzll(cmp_result);
               return mem_1_u8[idx] > mem_2_u8[idx] ? 1 : -1;
          }
          return 0;
     }
     case 5: {
          assert(remain_bytes > 16 && remain_bytes <= 32);

          u32 const cmp_result =
               v_16_bool_to_u16(__builtin_convertvector(*(const t_vec_16*)&mem_1_u8[idx] != *(const t_vec_16*)&mem_2_u8[idx], v_16_bool)) |
               (u32)v_16_bool_to_u16(__builtin_convertvector(*(const t_vec_16*)&mem_1_u8[len - 16] != *(const t_vec_16*)&mem_2_u8[len - 16], v_16_bool)) << (len - 16 - idx);

          if (cmp_result != 0) {
               idx += __builtin_ctzll(cmp_result);
               return mem_1_u8[idx] > mem_2_u8[idx] ? 1 : -1;
          }
          return 0;
     }
     case 4: {
          assert(remain_bytes > 8 && remain_bytes <= 16);

          u64 const num_1 = __builtin_bswap64(*(const ua_u64*)&mem_1_u8[idx]);
          u64 const num_2 = __builtin_bswap64(*(const ua_u64*)&mem_2_u8[idx]);
          u64 const num_3 = __builtin_bswap64(*(const ua_u64*)&mem_1_u8[len - 8]);
          u64 const num_4 = __builtin_bswap64(*(const ua_u64*)&mem_2_u8[len - 8]);

          return num_1 == num_2 ? (num_3 == num_4 ? 0 : num_3 > num_4 ? 1 : -1) : num_1 > num_2 ? 1 : -1;
     }
     case 3: {
          assert(remain_bytes > 4 && remain_bytes <= 8);

          u32 const num_1 = __builtin_bswap32(*(const ua_u32*)&mem_1_u8[idx]);
          u32 const num_2 = __builtin_bswap32(*(const ua_u32*)&mem_2_u8[idx]);
          u32 const num_3 = __builtin_bswap32(*(const ua_u32*)&mem_1_u8[len - 4]);
          u32 const num_4 = __builtin_bswap32(*(const ua_u32*)&mem_2_u8[len - 4]);

          return num_1 == num_2 ? (num_3 == num_4 ? 0 : num_3 > num_4 ? 1 : -1) : num_1 > num_2 ? 1 : -1;
     }
     case 2: {
          assert(remain_bytes >= 2 && remain_bytes <= 4);

          u16 const num_1 = __builtin_bswap16(*(const ua_u16*)&mem_1_u8[idx]);
          u16 const num_2 = __builtin_bswap16(*(const ua_u16*)&mem_2_u8[idx]);
          u16 const num_3 = __builtin_bswap16(*(const ua_u16*)&mem_1_u8[len - 2]);
          u16 const num_4 = __builtin_bswap16(*(const ua_u16*)&mem_2_u8[len - 2]);

          return num_1 == num_2 ? (num_3 == num_4 ? 0 : num_3 > num_4 ? 1 : -1) : num_1 > num_2 ? 1 : -1;
     }
     case 1:
          return mem_1_u8[0] == mem_2_u8[0] ? 0 : mem_1_u8[0] > mem_2_u8[0] ? 1 : -1;
     case 0:
          return 0;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memcpy_le_16(void* const dst, const void* const src, u64 const len) {
     assume(len < 17);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 4:
          assert(len > 8 && len <= 16);

          memcpy_inline(dst, src, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &((const u8*)src)[len - 8], 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memcpy_inline(dst, src, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &((const u8*)src)[len - 4], 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memcpy_inline(dst, src, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &((const u8*)src)[len - 2], 2);
          return;
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memcpy_le_32(void* const dst, const void* const src, u64 const len) {
     assume(len < 33);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 5:
          assert(len > 16 && len <= 32);

          memcpy_inline(dst, src, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &((const u8*)src)[len - 16], 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memcpy_inline(dst, src, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &((const u8*)src)[len - 8], 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memcpy_inline(dst, src, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &((const u8*)src)[len - 4], 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memcpy_inline(dst, src, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &((const u8*)src)[len - 2], 2);
          return;
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memcpy_le_64(void* const dst, const void* const src, u64 const len) {
     assume(len < 65);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 6:
          assert(len > 32 && len <= 64);

          memcpy_inline(dst, src, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &((const u8*)src)[len - 32], 32);
          return;
     case 5:
          assert(len > 16 && len <= 32);

          memcpy_inline(dst, src, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &((const u8*)src)[len - 16], 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memcpy_inline(dst, src, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &((const u8*)src)[len - 8], 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memcpy_inline(dst, src, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &((const u8*)src)[len - 4], 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memcpy_inline(dst, src, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &((const u8*)src)[len - 2], 2);
          return;
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memcpy_le_128(void* const dst, const void* const src, u64 const len) {
     assume(len < 129);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 7:
          assert(len > 64 && len <= 128);

          memcpy_inline(dst, src, 64);
          memcpy_inline(&((u8*)dst)[len - 64], &((const u8*)src)[len - 64], 64);
          return;
     case 6:
          assert(len > 32 && len <= 64);

          memcpy_inline(dst, src, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &((const u8*)src)[len - 32], 32);
          return;
     case 5:
          assert(len > 16 && len <= 32);

          memcpy_inline(dst, src, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &((const u8*)src)[len - 16], 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memcpy_inline(dst, src, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &((const u8*)src)[len - 8], 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memcpy_inline(dst, src, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &((const u8*)src)[len - 4], 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memcpy_inline(dst, src, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &((const u8*)src)[len - 2], 2);
          return;
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memcpy_le_256(void* const dst, const void* const src, u64 const len) {
     assume(len < 257);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 8:
          assert(len > 128 && len <= 256);

          memcpy_inline(dst, src, 128);
          memcpy_inline(&((u8*)dst)[len - 128], &((const u8*)src)[len - 128], 128);
          return;
     case 7:
          assert(len > 64 && len <= 128);

          memcpy_inline(dst, src, 64);
          memcpy_inline(&((u8*)dst)[len - 64], &((const u8*)src)[len - 64], 64);
          return;
     case 6:
          assert(len > 32 && len <= 64);

          memcpy_inline(dst, src, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &((const u8*)src)[len - 32], 32);
          return;
     case 5:
          assert(len > 16 && len <= 32);

          memcpy_inline(dst, src, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &((const u8*)src)[len - 16], 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memcpy_inline(dst, src, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &((const u8*)src)[len - 8], 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memcpy_inline(dst, src, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &((const u8*)src)[len - 4], 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memcpy_inline(dst, src, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &((const u8*)src)[len - 2], 2);
          return;
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memset_le_16(void* const dst, u8 const item, u64 const len) {
     assume(len < 17);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 4:
          assert(len > 8 && len <= 16);

          memset_inline(dst, item, 8);
          memset_inline(&((u8*)dst)[len - 8], item, 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memset_inline(dst, item, 4);
          memset_inline(&((u8*)dst)[len - 4], item, 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memset_inline(dst, item, 2);
          memset_inline(&((u8*)dst)[len - 2], item, 2);
          return;
     case 1:
          *(u8*)dst = item;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memset_le_32(void* const dst, u8 const item, u64 const len) {
     assume(len < 33);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 5:
          assert(len > 16 && len <= 32);

          memset_inline(dst, item, 16);
          memset_inline(&((u8*)dst)[len - 16], item, 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memset_inline(dst, item, 8);
          memset_inline(&((u8*)dst)[len - 8], item, 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memset_inline(dst, item, 4);
          memset_inline(&((u8*)dst)[len - 4], item, 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memset_inline(dst, item, 2);
          memset_inline(&((u8*)dst)[len - 2], item, 2);
          return;
     case 1:
          *(u8*)dst = item;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memset_le_64(void* const dst, u8 const item, u64 const len) {
     assume(len < 65);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 6:
          assert(len > 32 && len <= 64);

          memset_inline(dst, item, 32);
          memset_inline(&((u8*)dst)[len - 32], item, 32);
          return;
     case 5:
          assert(len > 16 && len <= 32);

          memset_inline(dst, item, 16);
          memset_inline(&((u8*)dst)[len - 16], item, 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memset_inline(dst, item, 8);
          memset_inline(&((u8*)dst)[len - 8], item, 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memset_inline(dst, item, 4);
          memset_inline(&((u8*)dst)[len - 4], item, 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memset_inline(dst, item, 2);
          memset_inline(&((u8*)dst)[len - 2], item, 2);
          return;
     case 1:
          *(u8*)dst = item;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memset_le_128(void* const dst, u8 const item, u64 const len) {
     assume(len < 129);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 7:
          assert(len > 64 && len <= 128);

          memset_inline(dst, item, 64);
          memset_inline(&((u8*)dst)[len - 64], item, 64);
          return;
     case 6:
          assert(len > 32 && len <= 64);

          memset_inline(dst, item, 32);
          memset_inline(&((u8*)dst)[len - 32], item, 32);
          return;
     case 5:
          assert(len > 16 && len <= 32);

          memset_inline(dst, item, 16);
          memset_inline(&((u8*)dst)[len - 16], item, 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memset_inline(dst, item, 8);
          memset_inline(&((u8*)dst)[len - 8], item, 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memset_inline(dst, item, 4);
          memset_inline(&((u8*)dst)[len - 4], item, 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memset_inline(dst, item, 2);
          memset_inline(&((u8*)dst)[len - 2], item, 2);
          return;
     case 1:
          *(u8*)dst = item;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memset_le_256(void* const dst, u8 const item, u64 const len) {
     assume(len < 257);

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 8:
          assert(len > 128 && len <= 256);

          memset_inline(dst, item, 128);
          memset_inline(&((u8*)dst)[len - 128], item, 128);
          return;
     case 7:
          assert(len > 64 && len <= 128);

          memset_inline(dst, item, 64);
          memset_inline(&((u8*)dst)[len - 64], item, 64);
          return;
     case 6:
          assert(len > 32 && len <= 64);

          memset_inline(dst, item, 32);
          memset_inline(&((u8*)dst)[len - 32], item, 32);
          return;
     case 5:
          assert(len > 16 && len <= 32);

          memset_inline(dst, item, 16);
          memset_inline(&((u8*)dst)[len - 16], item, 16);
          return;
     case 4:
          assert(len > 8 && len <= 16);

          memset_inline(dst, item, 8);
          memset_inline(&((u8*)dst)[len - 8], item, 8);
          return;
     case 3:
          assert(len > 4 && len <= 8);

          memset_inline(dst, item, 4);
          memset_inline(&((u8*)dst)[len - 4], item, 4);
          return;
     case 2:
          assert(len >= 2 && len <= 4);

          memset_inline(dst, item, 2);
          memset_inline(&((u8*)dst)[len - 2], item, 2);
          return;
     case 1:
          *(u8*)dst = item;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memmove_le_16(void* const dst, const void* const src, u64 const len) {
     assume(len < 17);

     [[gnu::aligned(alignof(u64))]]
     u8 buffer[16];

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 4: {
          assert(len > 8 && len <= 16);

          memcpy_inline(buffer, src, 8);
          memcpy_inline(&buffer[8], &((const u8*)src)[len - 8], 8);
          memcpy_inline(dst, buffer, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &buffer[8], 8);
          return;
     }
     case 3: {
          assert(len > 4 && len <= 8);

          memcpy_inline(buffer, src, 4);
          memcpy_inline(&buffer[4], &((const u8*)src)[len - 4], 4);
          memcpy_inline(dst, buffer, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &buffer[4], 4);
          return;
     }
     case 2: {
          assert(len >= 2 && len <= 4);

          memcpy_inline(buffer, src, 2);
          memcpy_inline(&buffer[2], &((const u8*)src)[len - 2], 2);
          memcpy_inline(dst, buffer, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &buffer[2], 2);
          return;
     }
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memmove_le_32(void* const dst, const void* const src, u64 const len) {
     assume(len < 33);

     [[gnu::aligned(16)]]
     u8 buffer[32];

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 5: {
          assert(len > 16 && len <= 32);

          memcpy_inline(buffer, src, 16);
          memcpy_inline(&buffer[16], &((const u8*)src)[len - 16], 16);
          memcpy_inline(dst, buffer, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &buffer[16], 16);
          return;
     }
     case 4: {
          assert(len > 8 && len <= 16);

          memcpy_inline(buffer, src, 8);
          memcpy_inline(&buffer[8], &((const u8*)src)[len - 8], 8);
          memcpy_inline(dst, buffer, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &buffer[8], 8);
          return;
     }
     case 3: {
          assert(len > 4 && len <= 8);

          memcpy_inline(buffer, src, 4);
          memcpy_inline(&buffer[4], &((const u8*)src)[len - 4], 4);
          memcpy_inline(dst, buffer, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &buffer[4], 4);
          return;
     }
     case 2: {
          assert(len >= 2 && len <= 4);

          memcpy_inline(buffer, src, 2);
          memcpy_inline(&buffer[2], &((const u8*)src)[len - 2], 2);
          memcpy_inline(dst, buffer, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &buffer[2], 2);
          return;
     }
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memmove_le_64(void* const dst, const void* const src, u64 const len) {
     assume(len < 65);

     [[gnu::aligned(32)]]
     u8 buffer[64];

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 6: {
          assert(len > 32 && len <= 64);

          memcpy_inline(buffer, src, 32);
          memcpy_inline(&buffer[32], &((const u8*)src)[len - 32], 32);
          memcpy_inline(dst, buffer, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &buffer[32], 32);
          return;
     }
     case 5: {
          assert(len > 16 && len <= 32);

          memcpy_inline(buffer, src, 16);
          memcpy_inline(&buffer[16], &((const u8*)src)[len - 16], 16);
          memcpy_inline(dst, buffer, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &buffer[16], 16);
          return;
     }
     case 4: {
          assert(len > 8 && len <= 16);

          memcpy_inline(buffer, src, 8);
          memcpy_inline(&buffer[8], &((const u8*)src)[len - 8], 8);
          memcpy_inline(dst, buffer, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &buffer[8], 8);
          return;
     }
     case 3: {
          assert(len > 4 && len <= 8);

          memcpy_inline(buffer, src, 4);
          memcpy_inline(&buffer[4], &((const u8*)src)[len - 4], 4);
          memcpy_inline(dst, buffer, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &buffer[4], 4);
          return;
     }
     case 2: {
          assert(len >= 2 && len <= 4);

          memcpy_inline(buffer, src, 2);
          memcpy_inline(&buffer[2], &((const u8*)src)[len - 2], 2);
          memcpy_inline(dst, buffer, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &buffer[2], 2);
          return;
     }
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memmove_le_128(void* const dst, const void* const src, u64 const len) {
     assume(len < 129);

     [[gnu::aligned(64)]]
     u8 buffer[128];

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 7: {
          assert(len > 64 && len <= 128);

          memcpy_inline(buffer, src, 64);
          memcpy_inline(&buffer[64], &((const u8*)src)[len - 64], 64);
          memcpy_inline(dst, buffer, 64);
          memcpy_inline(&((u8*)dst)[len - 64], &buffer[64], 64);
          return;
     }
     case 6: {
          assert(len > 32 && len <= 64);

          memcpy_inline(buffer, src, 32);
          memcpy_inline(&buffer[32], &((const u8*)src)[len - 32], 32);
          memcpy_inline(dst, buffer, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &buffer[32], 32);
          return;
     }
     case 5: {
          assert(len > 16 && len <= 32);

          memcpy_inline(buffer, src, 16);
          memcpy_inline(&buffer[16], &((const u8*)src)[len - 16], 16);
          memcpy_inline(dst, buffer, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &buffer[16], 16);
          return;
     }
     case 4: {
          assert(len > 8 && len <= 16);

          memcpy_inline(buffer, src, 8);
          memcpy_inline(&buffer[8], &((const u8*)src)[len - 8], 8);
          memcpy_inline(dst, buffer, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &buffer[8], 8);
          return;
     }
     case 3: {
          assert(len > 4 && len <= 8);

          memcpy_inline(buffer, src, 4);
          memcpy_inline(&buffer[4], &((const u8*)src)[len - 4], 4);
          memcpy_inline(dst, buffer, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &buffer[4], 4);
          return;
     }
     case 2: {
          assert(len >= 2 && len <= 4);

          memcpy_inline(buffer, src, 2);
          memcpy_inline(&buffer[2], &((const u8*)src)[len - 2], 2);
          memcpy_inline(dst, buffer, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &buffer[2], 2);
          return;
     }
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

[[clang::always_inline]]
static void memmove_le_256(void* const dst, const void* const src, u64 const len) {
     assume(len < 257);

     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     u8 buffer[256];

     switch (len < 3 ? len : sizeof(unsigned short) * 8 - __builtin_clzs((unsigned short)(len - 1))) {
     case 8: {
          assert(len > 128 && len <= 256);

          memcpy_inline(buffer, src, 128);
          memcpy_inline(&buffer[128], &((const u8*)src)[len - 128], 128);
          memcpy_inline(dst, buffer, 128);
          memcpy_inline(&((u8*)dst)[len - 128], &buffer[128], 128);
          return;
     }
     case 7: {
          assert(len > 64 && len <= 128);

          memcpy_inline(buffer, src, 64);
          memcpy_inline(&buffer[64], &((const u8*)src)[len - 64], 64);
          memcpy_inline(dst, buffer, 64);
          memcpy_inline(&((u8*)dst)[len - 64], &buffer[64], 64);
          return;
     }
     case 6: {
          assert(len > 32 && len <= 64);

          memcpy_inline(buffer, src, 32);
          memcpy_inline(&buffer[32], &((const u8*)src)[len - 32], 32);
          memcpy_inline(dst, buffer, 32);
          memcpy_inline(&((u8*)dst)[len - 32], &buffer[32], 32);
          return;
     }
     case 5: {
          assert(len > 16 && len <= 32);

          memcpy_inline(buffer, src, 16);
          memcpy_inline(&buffer[16], &((const u8*)src)[len - 16], 16);
          memcpy_inline(dst, buffer, 16);
          memcpy_inline(&((u8*)dst)[len - 16], &buffer[16], 16);
          return;
     }
     case 4: {
          assert(len > 8 && len <= 16);

          memcpy_inline(buffer, src, 8);
          memcpy_inline(&buffer[8], &((const u8*)src)[len - 8], 8);
          memcpy_inline(dst, buffer, 8);
          memcpy_inline(&((u8*)dst)[len - 8], &buffer[8], 8);
          return;
     }
     case 3: {
          assert(len > 4 && len <= 8);

          memcpy_inline(buffer, src, 4);
          memcpy_inline(&buffer[4], &((const u8*)src)[len - 4], 4);
          memcpy_inline(dst, buffer, 4);
          memcpy_inline(&((u8*)dst)[len - 4], &buffer[4], 4);
          return;
     }
     case 2: {
          assert(len >= 2 && len <= 4);

          memcpy_inline(buffer, src, 2);
          memcpy_inline(&buffer[2], &((const u8*)src)[len - 2], 2);
          memcpy_inline(dst, buffer, 2);
          memcpy_inline(&((u8*)dst)[len - 2], &buffer[2], 2);
          return;
     }
     case 1:
          *(u8*)dst = *(const u8*)src;
          return;
     case 0:
          return;
     default:
          unreachable;
     }
}

static void scar__move(void* const mem, u64 from, u64 to, u64 len) {
     u8* const mem_u8 = (u8*)mem;

     [[gnu::aligned(ALIGN_FOR_CACHE_LINE)]]
     u8 buffer[256];
     while (!(len == 0 || from == to)) {
          u64 overlap_size = len - (from < to ? to - from : from - to);
          if ((i64)overlap_size > 0) {
               u64 const old_to = to;
               to               = from + (from > to) * overlap_size;
               from             = old_to + !(from > old_to) * overlap_size;
               len             -= overlap_size;
               overlap_size     = len - (from < to ? to - from : from - to);
          }

          if (len <= 256) {
               memcpy_le_256(buffer, &mem_u8[from], len);
               memmove(&mem_u8[from + (from > to) * overlap_size], &mem_u8[to + !(from > to) * overlap_size], len - overlap_size);
               memcpy_le_256(&mem_u8[to], buffer, len);
               break;
          }

          u64 remain_bytes = len;
          u8* block_1      = &mem_u8[from];
          u8* block_2      = &mem_u8[to];
          for (; remain_bytes >= 256; remain_bytes -= 256, block_1 = &block_1[256], block_2 = &block_2[256]) {
               memcpy_inline(buffer, block_1, 256);
               memcpy_inline(block_1, block_2, 256);
               memcpy_inline(block_2, buffer, 256);
          }
          memcpy_le_256(buffer, block_1, remain_bytes);
          memcpy_le_256(block_1, block_2, remain_bytes);
          memcpy_le_256(block_2, buffer, remain_bytes);

          to = from < to ? to - len : to + len;
     }
}
