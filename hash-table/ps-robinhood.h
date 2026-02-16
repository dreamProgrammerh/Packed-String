#ifndef PACKED_STRING_PS_ROBINHOOD_H
#define PACKED_STRING_PS_ROBINHOOD_H

#include "../packed16/packed-string.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
  uint16_t fp;     // 0 = empty
  ps_t     key;
  uint64_t value;
} psrh_slot;

typedef struct {
  psrh_slot* slots;
  size_t   capacity;
  size_t   mask;
  size_t   size;
} psrh_map;

static inline uint64_t psrh_hash64(const ps_t k) {
  uint64_t x = k.lo ^ (k.hi * 0x9E3779B97F4A7C15ULL);
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ULL;
  x ^= x >> 33;
  return x;
}

static inline uint16_t psrh_fp(const uint64_t h) {
  const uint16_t f = (uint16_t)h;
  return f ? f : 1;   // avoid 0
}

static inline size_t psrh_probe_distance(const size_t slot_index, const size_t ideal_index, const size_t mask) {
  return (slot_index + (mask + 1) - ideal_index) & mask;
}

static inline bool psrh_equal(const ps_t a, const ps_t b) {
  return a.lo == b.lo && a.hi == b.hi;
}

static inline bool psrh_init(psrh_map* m, const size_t capacity) {
  size_t cap = 1;
  while (cap < capacity) cap <<= 1;

  m->slots = _aligned_malloc(cap * sizeof(psrh_slot), 64);
  if (!m->slots) return false;

  memset(m->slots, 0, cap * sizeof(psrh_slot));

  m->capacity = cap;
  m->mask = cap - 1;
  m->size = 0;
  return true;
}

static inline void psrh_free(psrh_map* m) {
  free(m->slots);
  m->slots = NULL;
  m->capacity = 0;
  m->size = 0;
}

static inline void psrh_clear(psrh_map* m) {
  memset(m->slots, 0, m->capacity * sizeof(psrh_slot));
  m->size = 0;
}

static inline bool psrh_set(psrh_map* m, ps_t key, uint64_t value) {
  if (m->size * 2 >= m->capacity) {
    return false; // no resize implemented
  }

  uint64_t h = psrh_hash64(key);
  uint16_t fp = psrh_fp(h);
  size_t idx = h & m->mask;
  size_t dist = 0;

  while (1) {
    psrh_slot* s = &m->slots[idx];

    if (s->fp == 0) {
      s->fp = fp;
      s->key = key;
      s->value = value;
      m->size++;
      return true;
    }

    if (s->fp == fp && psrh_equal(s->key, key)) {
      s->value = value;
      return true;
    }

    const uint64_t sh = psrh_hash64(s->key);
    const size_t ideal = sh & m->mask;
    const size_t s_dist = psrh_probe_distance(idx, ideal, m->mask);

    if (s_dist < dist) {
      // swap
      const psrh_slot tmp = *s;
      s->fp = fp;
      s->key = key;
      s->value = value;

      key = tmp.key;
      value = tmp.value;
      fp = tmp.fp;
      h = psrh_hash64(key);
      dist = s_dist;
    }

    idx = (idx + 1) & m->mask;
    dist++;
  }
}

static inline bool psrh_contains(const psrh_map* m, const ps_t key) {
  const uint64_t h = psrh_hash64(key);
  const uint16_t fp = psrh_fp(h);
  size_t idx = h & m->mask;
  size_t dist = 0;

  while (1) {
    const psrh_slot* s = &m->slots[idx];

    if (s->fp == 0)
      return false;

    if (s->fp == fp && psrh_equal(s->key, key))
      return true;

    const uint64_t sh = psrh_hash64(s->key);
    const size_t ideal = sh & m->mask;
    const size_t s_dist = psrh_probe_distance(idx, ideal, m->mask);

    if (s_dist < dist)
      return false;

    idx = (idx + 1) & m->mask;
    dist++;
  }
}

static inline bool psrh_get(const psrh_map* m, ps_t const key, uint64_t* out) {
  const uint64_t h = psrh_hash64(key);
  const uint16_t fp = psrh_fp(h);
  size_t idx = h & m->mask;
  size_t dist = 0;

  while (1) {
    const psrh_slot* s = &m->slots[idx];

    if (s->fp == 0)
      return false;

    if (s->fp == fp && psrh_equal(s->key, key)) {
      *out = s->value;
      return true;
    }

    const uint64_t sh = psrh_hash64(s->key);
    const size_t ideal = sh & m->mask;
    const size_t s_dist = psrh_probe_distance(idx, ideal, m->mask);

    if (s_dist < dist)
      return false;

    idx = (idx + 1) & m->mask;
    dist++;
  }
}

static inline bool psrh_delete(psrh_map* m, const ps_t key) {
  const uint64_t h = psrh_hash64(key);
  const uint16_t fp = psrh_fp(h);
  size_t idx = h & m->mask;
  size_t dist = 0;

  while (1) {
    const psrh_slot* s = &m->slots[idx];

    if (s->fp == 0)
      return false;

    if (s->fp == fp && psrh_equal(s->key, key))
      break;

    const uint64_t sh = psrh_hash64(s->key);
    const size_t ideal = sh & m->mask;
    const size_t s_dist = psrh_probe_distance(idx, ideal, m->mask);

    if (s_dist < dist)
      return false;

    idx = (idx + 1) & m->mask;
    dist++;
  }

  // backward shift
  size_t next = (idx + 1) & m->mask;

  while (1) {
    const psrh_slot* s = &m->slots[next];

    if (s->fp == 0)
      break;

    const uint64_t sh = psrh_hash64(s->key);
    const size_t ideal = sh & m->mask;

    if (psrh_probe_distance(next, ideal, m->mask) == 0)
      break;

    m->slots[idx] = *s;
    idx = next;
    next = (next + 1) & m->mask;
  }

  m->slots[idx].fp = 0;
  m->size--;
  return true;
}


#endif // PACKED_STRING_PS_ROBINHOOD_H
