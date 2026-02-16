#ifndef PACKED_STRING_CS_ROBINHOOD_H
#define PACKED_STRING_CS_ROBINHOOD_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
  uint16_t fp;     // 0 = empty
  char*     key;
  uint64_t value;
} csrh_slot;

typedef struct {
  csrh_slot* slots;
  size_t   capacity;
  size_t   mask;
  size_t   size;
} csrh_map;

static inline uint64_t csrh_hash64(const char* s) {
  uint64_t h = 1469598103934665603ULL;
  while (*s) {
    h ^= (unsigned char)*s++;
    h *= 1099511628211ULL;
  }
  return h;
}

static inline uint16_t csrh_fp(const uint64_t h) {
  const uint16_t f = (uint16_t)h;
  return f ? f : 1;   // avoid 0
}

static inline size_t csrh_probe_distance(const size_t slot_index, const size_t ideal_index, const size_t mask) {
  return (slot_index + (mask + 1) - ideal_index) & mask;
}

static inline bool csrh_equal(const char* a, const char* b) {
  return strcmp(a, b) == 0;
}

static inline bool csrh_init(csrh_map* m, const size_t capacity) {
  size_t cap = 1;
  while (cap < capacity) cap <<= 1;

  m->slots = _aligned_malloc(cap * sizeof(csrh_slot), 64);
  if (!m->slots) return false;

  memset(m->slots, 0, cap * sizeof(csrh_slot));

  m->capacity = cap;
  m->mask = cap - 1;
  m->size = 0;
  return true;
}

static inline void csrh_free(csrh_map* m) {
  free(m->slots);
  m->slots = NULL;
  m->capacity = 0;
  m->size = 0;
}

static inline void csrh_clear(csrh_map* m) {
  memset(m->slots, 0, m->capacity * sizeof(csrh_slot));
  m->size = 0;
}

static inline bool csrh_set(csrh_map* m, char* key, uint64_t value) {
  if (m->size * 2 >= m->capacity) {
    return false; // no resize implemented
  }

  uint64_t h = csrh_hash64(key);
  uint16_t fp = csrh_fp(h);
  size_t idx = h & m->mask;
  size_t dist = 0;

  while (1) {
    csrh_slot* s = &m->slots[idx];

    if (s->fp == 0) {
      s->fp = fp;
      s->key = key;
      s->value = value;
      m->size++;
      return true;
    }

    if (s->fp == fp && csrh_equal(s->key, key)) {
      s->value = value;
      return true;
    }

    const uint64_t sh = csrh_hash64(s->key);
    const size_t ideal = sh & m->mask;
    const size_t s_dist = csrh_probe_distance(idx, ideal, m->mask);

    if (s_dist < dist) {
      // swap
      const csrh_slot tmp = *s;
      s->fp = fp;
      s->key = key;
      s->value = value;

      key = tmp.key;
      value = tmp.value;
      fp = tmp.fp;
      h = csrh_hash64(key);
      dist = s_dist;
    }

    idx = (idx + 1) & m->mask;
    dist++;
  }
}

static inline bool csrh_contains(const csrh_map* m, const char* key) {
  const uint64_t h = csrh_hash64(key);
  const uint16_t fp = csrh_fp(h);
  size_t idx = h & m->mask;
  size_t dist = 0;

  while (1) {
    const csrh_slot* s = &m->slots[idx];

    if (s->fp == 0)
      return false;

    if (s->fp == fp && csrh_equal(s->key, key))
      return true;

    const uint64_t sh = csrh_hash64(s->key);
    const size_t ideal = sh & m->mask;
    const size_t s_dist = csrh_probe_distance(idx, ideal, m->mask);

    if (s_dist < dist)
      return false;

    idx = (idx + 1) & m->mask;
    dist++;
  }
}

static inline bool csrh_get(const csrh_map* m, const char* const key, uint64_t* out) {
  const uint64_t h = csrh_hash64(key);
  const uint16_t fp = csrh_fp(h);
  size_t idx = h & m->mask;
  size_t dist = 0;

  while (1) {
    const csrh_slot* s = &m->slots[idx];

    if (s->fp == 0)
      return false;

    if (s->fp == fp && csrh_equal(s->key, key)) {
      *out = s->value;
      return true;
    }

    const uint64_t sh = csrh_hash64(s->key);
    const size_t ideal = sh & m->mask;
    const size_t s_dist = csrh_probe_distance(idx, ideal, m->mask);

    if (s_dist < dist)
      return false;

    idx = (idx + 1) & m->mask;
    dist++;
  }
}

static inline bool csrh_delete(csrh_map* m, const char* key) {
  const uint64_t h = csrh_hash64(key);
  const uint16_t fp = csrh_fp(h);
  size_t idx = h & m->mask;
  size_t dist = 0;

  while (1) {
    const csrh_slot* s = &m->slots[idx];

    if (s->fp == 0)
      return false;

    if (s->fp == fp && csrh_equal(s->key, key))
      break;

    const uint64_t sh = csrh_hash64(s->key);
    const size_t ideal = sh & m->mask;
    const size_t s_dist = csrh_probe_distance(idx, ideal, m->mask);

    if (s_dist < dist)
      return false;

    idx = (idx + 1) & m->mask;
    dist++;
  }

  // backward shift
  size_t next = (idx + 1) & m->mask;

  while (1) {
    const csrh_slot* s = &m->slots[next];

    if (s->fp == 0)
      break;

    const uint64_t sh = csrh_hash64(s->key);
    const size_t ideal = sh & m->mask;

    if (csrh_probe_distance(next, ideal, m->mask) == 0)
      break;

    m->slots[idx] = *s;
    idx = next;
    next = (next + 1) & m->mask;
  }

  m->slots[idx].fp = 0;
  m->size--;
  return true;
}


#endif // PACKED_STRING_CS_ROBINHOOD_H
