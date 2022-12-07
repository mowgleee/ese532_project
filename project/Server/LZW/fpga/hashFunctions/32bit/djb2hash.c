uint32_t djb2hash(const void *key, size_t len) {
  uint32_t hash = 5381;
  const uint64_t * data = (const uint64_t *)key;

  for (size_t i = 0; i < len; i++)
    hash = ((hash << 5) + hash) + data[i]; /* hash * 33 + c */

  return hash;
}