#pragma once

#include "../../common/bool_vectors.h"
#include "../../common/include.h"
#include "../../common/macro.h"
#include "../../common/type.h"
#include "../string/basic.h"

[[clang::always_inline]]
static u64 int__rnd(t_thrd_data* const thrd_data) {
     u64 const rnd_num      = __builtin_reduce_or((thrd_data->rnd_num_src >> (const v_rnd_src){33, 15, 16, 8} & 0xffff) << (const v_rnd_src){0, 16, 32, 48});
     thrd_data->rnd_num_src = thrd_data->rnd_num_src * (const v_rnd_src){6364136223846793005ull, 1103515245ull, 25214903917ull, 65793ull} + (const v_rnd_src){1, 12345ull, 11, 4282663ull};

     return rnd_num;
}

[[clang::always_inline]]
static u64 int__sqrt(u64 const integer) {
     assert((i64)integer >= 0);

     if (integer < 0x20'0000'0440'48a3) return __builtin_sqrt((double)(i64)integer);
     if (sizeof(long double) >= 80) return __builtin_sqrtl((long double)(i64)integer);

     u64 left  = 9'490'6265ull;
     u64 right = integer + 1;
     for (u64 middle = (left + right) >> 1; left != right - 1; middle = (left + right) >> 1)
          if (middle * middle <= integer && middle < 3'037'000'500ull) left = middle; else right = middle;

     return left;
}

[[clang::noinline]]
static t_any format_int(u64 integer, u8 const base, bool const signed_) {
     assume(base >= 2 && base <= 36);

     typedef u8 v_64_u8 __attribute__((ext_vector_type(64)));

     [[gnu::aligned(alignof(v_64_u8))]]
     u8 buffer[64];

     if (integer == 0) return (const t_any){.bytes = {'0'}};

     bool const is_neg = signed_ && (i64)integer < 0;
     integer           = is_neg ? -integer : integer;

     u8 str_len;
     u8 str_offset;

     switch (base) {
     case 2: {
          str_len    = sizeof(long long) * 8 - __builtin_clzll(integer);
          str_offset = 64 - str_len;

          integer           = __builtin_bitreverse64(integer);
          *(v_64_u8*)buffer = __builtin_convertvector(u64_to_v_64_bool(integer), v_64_u8) + (u8)'0';

          break;
     }
     case 3: {
          typedef u64 v_64_u64 __attribute__((ext_vector_type(64)));

          *(v_64_u8*)buffer = __builtin_convertvector((const v_64_u64)integer / (const v_64_u64){
               12157665459056928801ull, 4052555153018976267ull, 1350851717672992089ull, 450283905890997363ull, 150094635296999121ull, 50031545098999707ull,
               16677181699666569ull, 5559060566555523ull, 1853020188851841ull, 617673396283947ull, 205891132094649ull, 68630377364883ull, 22876792454961ull,
               7625597484987ull, 2541865828329ull, 847288609443ull, 282429536481ull, 94143178827ull, 31381059609ull, 10460353203ull, 3486784401ull, 1162261467ull,
               387420489ull, 129140163ull, 43046721ull, 14348907ull, 4782969ull, 1594323ull, 531441ull, 177147ull, 59049ull, 19683ull, 6561ull, 2187ull, 729ull,
               243ull, 81ull, 27ull, 9ull, 3ull, 1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1,
               (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 3, v_64_u8) + (u8)'0';
          str_offset = __builtin_ctzll(v_64_bool_to_u64(__builtin_convertvector(*(v_64_u8*)buffer != '0', v_64_bool)));
          str_len    = 41 - str_offset;

          break;
     }
     case 4: {
          typedef u8 v_8_u8  __attribute__((ext_vector_type(8), aligned(alignof(u64))));
          typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));

          str_len    = (u8)(sizeof(long long) * 8 - __builtin_clzll(integer) + 1) / 2;
          str_offset = 32 - str_len;

          *(v_32_u8*)buffer = ((
               (*(const v_8_u8*)&integer).s77776666555544443333222211110000 >> (const v_32_u8){6, 4, 2, 0, 6, 4, 2, 0, 6, 4, 2, 0, 6, 4, 2, 0, 6, 4, 2, 0, 6, 4, 2, 0, 6, 4, 2, 0, 6, 4, 2, 0}
          ) & 3) + (u8)'0';

          break;
     }
     case 5: {
          typedef u8  v_32_u8  __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));
          typedef u64 v_32_u64 __attribute__((ext_vector_type(32)));

          *(v_32_u8*)buffer = __builtin_convertvector((const v_32_u64)integer / (const v_32_u64){
               7450580596923828125ull, 1490116119384765625ull, 298023223876953125ull, 59604644775390625ull, 11920928955078125ull, 2384185791015625ull, 476837158203125ull,
               95367431640625ull, 19073486328125ull, 3814697265625ull, 762939453125ull, 152587890625ull, 30517578125ull, 6103515625ull, 1220703125ull, 244140625ull,
               48828125ull, 9765625ull, 1953125ull, 390625ull, 78125ull, 15625ull, 3125ull, 625ull, 125ull, 25ull, 5ull, 1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 5, v_32_u8) + (u8)'0';
          str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8*)buffer != '0', v_32_bool)));
          str_len    = 28 - str_offset;

          break;
     }
     case 6: {
          typedef u8  v_32_u8  __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));
          typedef u64 v_32_u64 __attribute__((ext_vector_type(32)));

          *(v_32_u8*)buffer = __builtin_convertvector((const v_32_u64)integer / (const v_32_u64){
               4738381338321616896ull, 789730223053602816ull, 131621703842267136ull, 21936950640377856ull, 3656158440062976ull, 609359740010496ull, 101559956668416ull,
               16926659444736ull, 2821109907456ull, 470184984576ull, 78364164096ull, 13060694016ull, 2176782336ull, 362797056ull, 60466176ull, 10077696ull, 1679616ull,
               279936ull, 46656ull, 7776ull, 1296ull, 216ull, 36ull, 6ull, 1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 6, v_32_u8) + (u8)'0';
          str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8*)buffer != '0', v_32_bool)));
          str_len    = 25 - str_offset;

          break;
     }
     case 7: {
          typedef u8  v_32_u8  __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));
          typedef u64 v_32_u64 __attribute__((ext_vector_type(32)));

          *(v_32_u8*)buffer = __builtin_convertvector((const v_32_u64)integer / (const v_32_u64){
               3909821048582988049ull, 558545864083284007ull, 79792266297612001ull, 11398895185373143ull, 1628413597910449ull, 232630513987207ull, 33232930569601ull,
               4747561509943ull, 678223072849ull, 96889010407ull, 13841287201ull, 1977326743ull, 282475249ull, 40353607ull, 5764801ull, 823543ull, 117649ull, 16807ull,
               2401ull, 343ull, 49ull, 7ull, 1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 7, v_32_u8) + (u8)'0';
          str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8*)buffer != '0', v_32_bool)));
          str_len    = 23 - str_offset;

          break;
     }
     case 8: {
          typedef u8 v_8_u8  __attribute__((ext_vector_type(8), aligned(alignof(u64))));
          typedef u8 v_32_u8 __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));

          str_len    = (u8)(sizeof(long long) * 8 - __builtin_clzll(integer) + 2) / 3;
          str_offset = 32 - str_len;

          *(v_32_u8*)buffer = (
               __builtin_shufflevector(*(const v_8_u8*)&integer, (const v_8_u8){}, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 6, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 1, 1, 1, 0, 0, 0) >>
               (const v_32_u8){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 4, 1, 6, 3, 0, 5, 2, 7, 4, 1, 6, 3, 0, 5, 2, 7, 4, 1, 6, 3, 0}
          ) & (const v_32_u8){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 7, 7, 3, 7, 7, 7, 7, 1, 7, 7, 3, 7, 7, 7, 7, 1, 7, 7, 3, 7, 7};
          *(v_32_u8*)buffer |= (
               __builtin_shufflevector(*(const v_8_u8*)&integer, (const v_8_u8){}, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 5, 5, 5, 4, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0) <<
               (const v_32_u8){0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 2, 3, 3, 3, 3, 1, 3, 3, 2, 3, 3, 3, 3, 1, 3, 3, 2, 3, 3}
          ) & 7;
          *(v_32_u8*)buffer += (u8)'0';
          *(v_32_u8*)buffer += (*(const v_32_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          break;
     }
     case 9: {
          typedef u8  v_32_u8  __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));
          typedef u64 v_32_u64 __attribute__((ext_vector_type(32)));

          *(v_32_u8*)buffer = __builtin_convertvector((const v_32_u64)integer / (const v_32_u64){
               12157665459056928801ull, 1350851717672992089ull, 150094635296999121ull, 16677181699666569ull, 1853020188851841ull, 205891132094649ull, 22876792454961ull,
               2541865828329ull, 282429536481ull, 31381059609ull, 3486784401ull, 387420489ull, 43046721ull, 4782969ull, 531441ull, 59049ull, 6561ull, 729ull, 81ull,
               9ull, 1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 9, v_32_u8) + (u8)'0';
          str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8*)buffer != '0', v_32_bool)));
          str_len    = 21 - str_offset;

          break;
     }
     case 10: {
          typedef u8  v_32_u8  __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));
          typedef u64 v_32_u64 __attribute__((ext_vector_type(32)));

          *(v_32_u8*)buffer = __builtin_convertvector((const v_32_u64)integer / (const v_32_u64){
               10000000000000000000ull, 1000000000000000000ull, 100000000000000000ull, 10000000000000000ull, 1000000000000000ull, 100000000000000ull, 10000000000000ull,
               1000000000000ull, 100000000000ull, 10000000000ull, 1000000000ull, 100000000ull, 10000000ull, 1000000ull, 100000ull, 10000ull, 1000ull, 100ull, 10ull,
               1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 10, v_32_u8) + (u8)'0';

          str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8*)buffer != '0', v_32_bool)));
          str_len    = 20 - str_offset;

          break;
     }
     case 11: {
          typedef u8  v_32_u8  __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));
          typedef u64 v_32_u64 __attribute__((ext_vector_type(32)));

          *(v_32_u8*)buffer = __builtin_convertvector((const v_32_u64)integer / (const v_32_u64){
               5559917313492231481ull, 505447028499293771ull, 45949729863572161ull, 4177248169415651ull, 379749833583241ull, 34522712143931ull, 3138428376721ull, 285311670611ull,
               25937424601ull, 2357947691ull, 214358881ull, 19487171ull, 1771561ull, 161051ull, 14641ull, 1331ull, 121ull, 11ull, 1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1,
               (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 11, v_32_u8) + (u8)'0';
          *(v_32_u8*)buffer += (*(const v_32_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8*)buffer != '0', v_32_bool)));
          str_len    = 19 - str_offset;

          break;
     }
     case 12: {
          typedef u8  v_32_u8  __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));
          typedef u64 v_32_u64 __attribute__((ext_vector_type(32)));

          *(v_32_u8*)buffer = __builtin_convertvector((const v_32_u64)integer / (const v_32_u64){
               2218611106740436992ull, 184884258895036416ull, 15407021574586368ull, 1283918464548864ull, 106993205379072ull, 8916100448256ull, 743008370688ull, 61917364224ull,
               5159780352ull, 429981696ull, 35831808ull, 2985984ull, 248832ull, 20736ull, 1728ull, 144ull, 12ull, 1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1,
               (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 12, v_32_u8) + (u8)'0';
          *(v_32_u8*)buffer += (*(const v_32_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8*)buffer != '0', v_32_bool)));
          str_len    = 18 - str_offset;

          break;
     }
     case 13: {
          typedef u8  v_32_u8  __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));
          typedef u64 v_32_u64 __attribute__((ext_vector_type(32)));

          *(v_32_u8*)buffer = __builtin_convertvector((const v_32_u64)integer / (const v_32_u64){
               8650415919381337933ull, 665416609183179841ull, 51185893014090757ull, 3937376385699289ull, 302875106592253ull, 23298085122481ull, 1792160394037ull, 137858491849ull,
               10604499373ull, 815730721ull, 62748517ull, 4826809ull, 371293ull, 28561ull, 2197ull, 169ull, 13ull, 1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1,
               (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 13, v_32_u8) + (u8)'0';
          *(v_32_u8*)buffer += (*(const v_32_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8*)buffer != '0', v_32_bool)));
          str_len    = 18 - str_offset;

          break;
     }
     case 14: {
          typedef u8  v_32_u8  __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));
          typedef u64 v_32_u64 __attribute__((ext_vector_type(32)));

          *(v_32_u8*)buffer = __builtin_convertvector((const v_32_u64)integer / (const v_32_u64){
               2177953337809371136ull, 155568095557812224ull, 11112006825558016ull, 793714773254144ull, 56693912375296ull, 4049565169664ull, 289254654976ull, 20661046784ull, 1475789056ull,
               105413504ull, 7529536ull, 537824ull, 38416ull, 2744ull, 196ull, 14ull, 1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1,
               (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 14, v_32_u8) + (u8)'0';
          *(v_32_u8*)buffer += (*(const v_32_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8*)buffer != '0', v_32_bool)));
          str_len    = 17 - str_offset;

          break;
     }
     case 15: {
          typedef u8  v_32_u8  __attribute__((ext_vector_type(32), aligned(alignof(v_64_u8))));
          typedef u64 v_32_u64 __attribute__((ext_vector_type(32)));

          *(v_32_u8*)buffer = __builtin_convertvector((const v_32_u64)integer / (const v_32_u64){
               6568408355712890625ull, 437893890380859375ull, 29192926025390625ull, 1946195068359375ull, 129746337890625ull, 8649755859375ull, 576650390625ull, 38443359375ull,
               2562890625ull, 170859375ull, 11390625ull, 759375ull, 50625ull, 3375ull, 225ull, 15ull, 1ull, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1,
               (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1, (u64)-1
          } % 15, v_32_u8) + (u8)'0';
          *(v_32_u8*)buffer += (*(const v_32_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctzl(v_32_bool_to_u32(__builtin_convertvector(*(v_32_u8*)buffer != '0', v_32_bool)));
          str_len    = 17 - str_offset;

          break;
     }
     case 16: {
          typedef u8 v_8_u8  __attribute__((ext_vector_type(8), aligned(alignof(u64))));
          typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));

          str_len    = (u8)(sizeof(long long) * 8 - __builtin_clzll(integer) + 3) / 4;
          str_offset = 16 - str_len;

          *(v_16_u8*)buffer  = (((*(const v_8_u8*)&integer).s7766554433221100 >> (const v_16_u8){4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 0}) & 15) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          break;
     }
     case 17: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               2862423051509815793ull, 168377826559400929ull, 9904578032905937ull, 582622237229761ull, 34271896307633ull, 2015993900449ull, 118587876497ull, 6975757441ull,
               410338673ull, 24137569ull, 1419857ull, 83521ull, 4913ull, 289ull, 17ull, 1ull
          } % 17, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 16 - str_offset;

          break;
     }
     case 18: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               6746640616477458432ull, 374813367582081024ull, 20822964865671168ull, 1156831381426176ull, 64268410079232ull, 3570467226624ull, 198359290368ull, 11019960576ull,
               612220032ull, 34012224ull, 1889568ull, 104976ull, 5832ull, 324ull, 18ull, 1ull,
          } % 18, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 16 - str_offset;

          break;
     }
     case 19: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               15181127029874798299ull, 799006685782884121ull, 42052983462257059ull, 2213314919066161ull, 116490258898219ull, 6131066257801ull, 322687697779ull, 16983563041ull,
               893871739ull, 47045881ull, 2476099ull, 130321ull, 6859ull, 361ull, 19ull, 1ull,
          } % 19, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 16 - str_offset;

          break;
     }
     case 20: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               1638400000000000000ull, 81920000000000000ull, 4096000000000000ull, 204800000000000ull, 10240000000000ull, 512000000000ull, 25600000000ull, 1280000000ull, 64000000ull,
               3200000ull, 160000ull, 8000ull, 400ull, 20ull, 1ull, (u64)-1
          } % 20, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 15 - str_offset;

          break;
     }
     case 21: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               3243919932521508681ull, 154472377739119461ull, 7355827511386641ull, 350277500542221ull, 16679880978201ull, 794280046581ull, 37822859361ull, 1801088541ull, 85766121ull,
               4084101ull, 194481ull, 9261ull, 441ull, 21ull, 1ull, (u64)-1
          } % 21, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 15 - str_offset;

          break;
     }
     case 22: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               6221821273427820544ull, 282810057883082752ull, 12855002631049216ull, 584318301411328ull, 26559922791424ull, 1207269217792ull, 54875873536ull, 2494357888ull, 113379904ull,
               5153632ull, 234256ull, 10648ull, 484ull, 22ull, 1ull, (u64)-1
          } % 22, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 15 - str_offset;

          break;
     }
     case 23: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               11592836324538749809ull, 504036361936467383ull, 21914624432020321ull, 952809757913927ull, 41426511213649ull, 1801152661463ull, 78310985281ull, 3404825447ull, 148035889ull,
               6436343ull, 279841ull, 12167ull, 529ull, 23ull, 1ull, (u64)-1
          } % 23, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 15 - str_offset;

          break;
     }
     case 24: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               876488338465357824ull, 36520347436056576ull, 1521681143169024ull, 63403380965376ull, 2641807540224ull, 110075314176ull, 4586471424ull, 191102976ull, 7962624ull, 331776ull,
               13824ull, 576ull, 24ull, 1ull, (u64)-1, (u64)-1
          } % 24, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 14 - str_offset;

          break;
     }
     case 25: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               1490116119384765625ull, 59604644775390625ull, 2384185791015625ull, 95367431640625ull, 3814697265625ull, 152587890625ull, 6103515625ull, 244140625ull, 9765625ull, 390625ull,
               15625ull, 625ull, 25ull, 1ull, (u64)-1, (u64)-1
          } % 25, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 14 - str_offset;

          break;
     }
     case 26: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               2481152873203736576ull, 95428956661682176ull, 3670344486987776ull, 141167095653376ull, 5429503678976ull, 208827064576ull, 8031810176ull, 308915776ull, 11881376ull, 456976ull,
               17576ull, 676ull, 26ull, 1ull, (u64)-1, (u64)-1
          } % 26, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 14 - str_offset;

          break;
     }
     case 27: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               4052555153018976267ull, 150094635296999121ull, 5559060566555523ull, 205891132094649ull, 7625597484987ull, 282429536481ull, 10460353203ull, 387420489ull, 14348907ull, 531441ull,
               19683ull, 729ull, 27ull, 1ull, (u64)-1, (u64)-1
          } % 27, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 14 - str_offset;

          break;
     }
     case 28: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               6502111422497947648ull, 232218265089212416ull, 8293509467471872ull, 296196766695424ull, 10578455953408ull, 377801998336ull, 13492928512ull, 481890304ull, 17210368ull, 614656ull,
               21952ull, 784ull, 28ull, 1ull, (u64)-1, (u64)-1
          } % 28, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 14 - str_offset;

          break;
     }
     case 29: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               10260628712958602189ull, 353814783205469041ull, 12200509765705829ull, 420707233300201ull, 14507145975869ull, 500246412961ull, 17249876309ull, 594823321ull, 20511149ull, 707281ull,
               24389ull, 841ull, 29ull, 1ull, (u64)-1, (u64)-1
          } % 29, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 14 - str_offset;

          break;
     }
     case 30: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               15943230000000000000ull, 531441000000000000ull, 17714700000000000ull, 590490000000000ull, 19683000000000ull, 656100000000ull, 21870000000ull, 729000000ull, 24300000ull, 810000ull,
               27000ull, 900ull, 30ull, 1ull, (u64)-1, (u64)-1
          } % 30, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 14 - str_offset;

          break;
     }
     case 31: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               787662783788549761ull, 25408476896404831ull, 819628286980801ull, 26439622160671ull, 852891037441ull, 27512614111ull, 887503681ull, 28629151ull, 923521ull, 29791ull, 961ull, 31ull,
               1ull, (u64)-1, (u64)-1, (u64)-1
          } % 31, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 13 - str_offset;

          break;
     }
     case 32: {
          typedef u8 v_8_u8  __attribute__((ext_vector_type(8), aligned(alignof(u64))));
          typedef u8 v_16_u8 __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));

          str_len    = (u8)(sizeof(long long) * 8 - __builtin_clzll(integer) + 4) / 5;
          str_offset = 16 - str_len;

          *(v_16_u8*)buffer = (
               __builtin_shufflevector(*(const v_8_u8*)&integer, (const v_8_u8){}, 8, 8, 8, 7, 6, 6, 5, 5, 4, 3, 3, 2, 1, 1, 0, 0) >>
               (const v_16_u8){0, 0, 0, 4, 7, 2, 5, 0, 3, 6, 1, 4, 7, 2, 5, 0}
          ) & (const v_16_u8){0, 0, 0, 15, 1, 31, 7, 31, 31, 3, 31, 15, 1, 31, 7, 31};
          *(v_16_u8*)buffer |= (
               __builtin_shufflevector(*(const v_8_u8*)&integer, (const v_8_u8){}, 8, 8, 8, 8, 7, 6, 6, 5, 4, 4, 3, 3, 2, 1, 1, 0) <<
               (const v_16_u8){0, 0, 0, 4, 1, 5, 3, 5, 5, 2, 5, 4, 1, 5, 3, 5}
          ) & 31;
          *(v_16_u8*)buffer += (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          break;
     }
     case 33: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               1667889514952984961ull, 50542106513726817ull, 1531578985264449ull, 46411484401953ull, 1406408618241ull, 42618442977ull, 1291467969ull, 39135393ull, 1185921ull, 35937ull, 1089ull, 33ull,
               1ull, (u64)-1, (u64)-1, (u64)-1
          } % 33, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 13 - str_offset;

          break;
     }
     case 34: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               2386420683693101056ull, 70188843638032384ull, 2064377754059776ull, 60716992766464ull, 1785793904896ull, 52523350144ull, 1544804416ull, 45435424ull, 1336336ull, 39304ull, 1156ull, 34ull,
               1ull, (u64)-1, (u64)-1, (u64)-1
          } % 34, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 13 - str_offset;

          break;
     }
     case 35: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               3379220508056640625ull, 96549157373046875ull, 2758547353515625ull, 78815638671875ull, 2251875390625ull, 64339296875ull, 1838265625ull, 52521875ull, 1500625ull, 42875ull, 1225ull, 35ull,
               1ull, (u64)-1, (u64)-1, (u64)-1
          } % 35, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 13 - str_offset;

          break;
     }
     case 36: {
          typedef u8  v_16_u8  __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u64 v_16_u64 __attribute__((ext_vector_type(16)));

          *(v_16_u8*)buffer = __builtin_convertvector((const v_16_u64)integer / (const v_16_u64){
               4738381338321616896ull, 131621703842267136ull, 3656158440062976ull, 101559956668416ull, 2821109907456ull, 78364164096ull, 2176782336ull, 60466176ull, 1679616ull, 46656ull, 1296ull, 36ull,
               1ull, (u64)-1, (u64)-1, (u64)-1
          } % 36, v_16_u8) + (u8)'0';
          *(v_16_u8*)buffer += (*(const v_16_u8*)buffer > (u8)'9') & (u8)('a' - ':');

          str_offset = __builtin_ctz(v_16_bool_to_u16(__builtin_convertvector(*(v_16_u8*)buffer != '0', v_16_bool)));
          str_len    = 13 - str_offset;

          break;
     }
     default:
          unreachable;
     }

     u8 const full_len = str_len + is_neg;

     if (full_len < 16) {
          t_any result = {.bytes = {is_neg ? '-' : 0}};
          memcpy_le_16(&result.bytes[is_neg], &buffer[str_offset], str_len);
          return result;
     }

     t_any result = long_string__new(full_len);
     slice_array__set_len(result, full_len);
     result       = slice__set_metadata(result, 0, full_len);
     u8*   chars  = slice_array__get_items(result);
     *(u32*)chars = is_neg ? (u32)'-' << 16 : 0;
     chars        = is_neg ? &chars[3] : chars;

     if (str_len > 32) {
          typedef u8 v_32_u8_a1 __attribute__((ext_vector_type(32), aligned(1)));
          typedef u8 v_64_u8_a1 __attribute__((ext_vector_type(64), aligned(1)));

          v_32_u8_a1 const low_part  = *(const v_32_u8_a1*)&buffer[str_offset];
          v_32_u8_a1 const high_part = *(const v_32_u8_a1*)&buffer[str_offset + str_len - 32];

          *(v_64_u8_a1*)chars = __builtin_shufflevector(low_part, (const v_32_u8_a1){},
               32, 32, 0, 32, 32, 1, 32, 32, 2, 32, 32, 3, 32, 32, 4, 32, 32, 5, 32, 32, 6, 32, 32, 7, 32, 32, 8, 32, 32, 9, 32, 32, 10,
               32, 32, 11, 32, 32, 12, 32, 32, 13, 32, 32, 14, 32, 32, 15, 32, 32, 16, 32, 32, 17, 32, 32, 18, 32, 32, 19, 32, 32, 20, 32
          );
          *(v_32_u8_a1*)&chars[64] = __builtin_shufflevector(low_part, (const v_32_u8_a1){},
               32, 21, 32, 32, 22, 32, 32, 23, 32, 32, 24, 32, 32, 25, 32, 32, 26, 32, 32, 27, 32, 32, 28, 32, 32, 29, 32, 32, 30, 32, 32, 31
          );

          *(v_64_u8_a1*)&chars[str_len * 3 - 96] = __builtin_shufflevector(high_part, (const v_32_u8_a1){},
               32, 32, 0, 32, 32, 1, 32, 32, 2, 32, 32, 3, 32, 32, 4, 32, 32, 5, 32, 32, 6, 32, 32, 7, 32, 32, 8, 32, 32, 9, 32, 32, 10,
               32, 32, 11, 32, 32, 12, 32, 32, 13, 32, 32, 14, 32, 32, 15, 32, 32, 16, 32, 32, 17, 32, 32, 18, 32, 32, 19, 32, 32, 20, 32
          );
          *(v_32_u8_a1*)&chars[str_len * 3 - 32] = __builtin_shufflevector(high_part, (const v_32_u8_a1){},
               32, 21, 32, 32, 22, 32, 32, 23, 32, 32, 24, 32, 32, 25, 32, 32, 26, 32, 32, 27, 32, 32, 28, 32, 32, 29, 32, 32, 30, 32, 32, 31
          );
     } else if (str_len != 15) {
          typedef u8 v_16_u8_a1 __attribute__((ext_vector_type(16), aligned(1)));
          typedef u8 v_32_u8_a1 __attribute__((ext_vector_type(32), aligned(1)));

          v_16_u8_a1 const low_part  = *(const v_16_u8_a1*)&buffer[str_offset];
          v_16_u8_a1 const high_part = *(const v_16_u8_a1*)&buffer[str_offset + str_len - 16];

          *(v_32_u8_a1*)chars      = __builtin_shufflevector(low_part, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
          *(v_16_u8_a1*)&chars[32] = __builtin_shufflevector(low_part, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);

          *(v_32_u8_a1*)&chars[str_len * 3 - 48] = __builtin_shufflevector(high_part, (const v_16_u8_a1){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
          *(v_16_u8_a1*)&chars[str_len * 3 - 16] = __builtin_shufflevector(high_part, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);
     } else {
          typedef u8 v_16_u8_abuffer __attribute__((ext_vector_type(16), aligned(alignof(v_64_u8))));
          typedef u8 v_16_u8_a1      __attribute__((ext_vector_type(16), aligned(1)));
          typedef u8 v_32_u8_a1      __attribute__((ext_vector_type(32), aligned(1)));

          if (str_offset == 0) {
               v_16_u8_abuffer const buffer_chars = *(const v_16_u8_abuffer*)buffer;

               *(v_32_u8_a1*)chars      = __builtin_shufflevector(buffer_chars, (const v_16_u8_abuffer){}, 16, 16, 0, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16);
               *(v_16_u8_a1*)&chars[29] = __builtin_shufflevector(buffer_chars, (const v_16_u8_abuffer){}, 9, 16, 16, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14);
          } else {
               v_16_u8_a1 const buffer_chars = *(const v_16_u8_a1*)&buffer[str_offset - 1];

               *(v_32_u8_a1*)chars      = __builtin_shufflevector(buffer_chars, (const v_16_u8_a1){}, 16, 16, 1, 16, 16, 2, 16, 16, 3, 16, 16, 4, 16, 16, 5, 16, 16, 6, 16, 16, 7, 16, 16, 8, 16, 16, 9, 16, 16, 10, 16, 16);
               *(v_16_u8_a1*)&chars[29] = __builtin_shufflevector(buffer_chars, (const v_16_u8_a1){}, 10, 16, 16, 11, 16, 16, 12, 16, 16, 13, 16, 16, 14, 16, 16, 15);
          }
     }

     return result;
}