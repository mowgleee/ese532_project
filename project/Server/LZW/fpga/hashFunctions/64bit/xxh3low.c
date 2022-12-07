#include <stdint.h>

/* Constants for the XXH3low_mix16B() function */
#define XXH3LOW_MIX16B_MUL 0x9E3779B97F4A7C15ull
#define XXH3LOW_MIX16B_ROT1 19
#define XXH3LOW_MIX16B_ROT2 27

/*
 * The XXH3low_mix16B() function is used to mix the input data for the
 * xxh3low hash function.
 *
 * This implementation is based on the reference implementation found here:
 * https://github.com/Cyan4973/xxHash/blob/master/xxh3.c
 */
static uint64_t XXH3low_mix16B(uint64_t val, uint64_t mul) {
  uint64_t h32 = (val ^ mul) * XXH3LOW_MIX16B_MUL;
  h32 ^= h32 >> XXH3LOW_MIX16B_ROT1;
  h32 *= XXH3LOW_MIX16B_MUL;
  h32 ^= h32 >> XXH3LOW_MIX16B_ROT2;

  return h32;
}

/*
 * The xxh3low() function is used to quickly hash a buffer of data.
 *
 * This implementation is based on the reference implementation found here:
 * https://github.com/Cyan4973/xxHash/blob/master/xxh3.c
 */
uint64_t xxh3low(const void *data, size_t len, uint64_t seed) {
  const uint8_t *p = (const uint8_t *)data;
  const uint8_t *const bEnd = p + len;
  uint64_t acc = len * XXH3LOW_MIX16B_MUL ^ seed;

  while (p + 16 <= bEnd) {
    acc = XXH3low_mix16B(*(const uint64_t *)p, acc);
    acc = XXH3low_mix16B(*(const uint64_t *)(p + 8), acc);
    p += 16;
  }

  if (p + 8 <= bEnd) {
    acc = XXH3low_mix16B(*(const uint64_t *)p, acc);
    p += 8;
  }

  if (p + 4 <= bEnd) {
    acc = XXH3low_mix16B(*(const uint32_t *)p, acc);
    p += 4;
  }

  if (p + 2 <= bEnd) {
    acc = XXH3low_mix16B(*(const uint16_t *)p, acc);
    p += 2;
  }

  if (p + 1 <= bEnd) {
    acc = XXH3low_mix16B(*p, acc);
  }

  return XXH3low_mix16B(acc ^ len, seed);
}