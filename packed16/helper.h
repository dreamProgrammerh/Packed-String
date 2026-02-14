#ifndef PACKED_HELPER_H
#define PACKED_HELPER_H

#include "encoding.h"

static inline void ps_shl(u64 *restrict lo, u64 *restrict hi, const u8 shift) {
    *hi = *hi << shift | *lo >> (64 - shift);
    *lo <<= shift;
}

static inline void ps_shr(u64 *restrict lo, u64 *restrict hi, const u8 shift) {
    *lo = *hi << (64 - shift) | *lo >> shift;
    *hi >>= shift;
}

static inline void ps_shl128(u64 *restrict lo, u64 *restrict hi, const u8 shift) {
    if (shift <= 64) {
        *hi = *hi << shift | *lo >> (64 - shift);
        *lo <<= shift;
    } else {
        *hi = *lo << (shift - 64);
        *lo = 0;
    }
}

static inline void ps_shr128(u64 *restrict lo, u64 *restrict hi, const u8 shift) {
    if (shift < 64) {
        *lo = *lo >> shift | *hi << (64 - shift);
        *hi >>= shift;
    } else {
        *lo = *hi >> (shift - 64);
        *hi = 0;
    }
}

static inline void ps_mask(u64 *restrict lo, u64 *restrict hi, const u8 start, const u8 length) {
    const u8 lo_len = 64 - start;
    const u8 hi_len = length - lo_len;
    u64 lo_mask = ~0ULL, hi_mask = ~0ULL;

    if (lo_len != 64)
        lo_mask = ((1ULL << lo_len) - 1ULL) << start;

    if (hi_len != 64)
        hi_mask = (1ULL << hi_len) - 1ULL;

    *lo &= lo_mask;
    *hi &= hi_mask;
}

static inline void ps_limit(u64 *restrict lo, u64 *restrict hi, const u8 length) {
    const u32 bit_len  = (u32)length * 6;

    if (bit_len < 64) {
        *lo &= (1ULL << bit_len) - 1ULL;
    } else /* if bit_len > 64 */ {
        *hi &= (1ULL << (bit_len - 64)) - 1ULL;
    }
    // Note: bit_len cannot be 64,
    //  because 64 is not divisible by 6
}

static inline u8 ps_get_mid(const u64 lo, const u64 hi) {
    return (hi & 0x3) << 4 | lo >> 60 & 0xF;
}

static inline u8 ps_get_lo(const u64 lo, const u8 n) {
    return lo >> (n * 6) & 0x3F;
}

static inline u8 ps_get_hi(const u64 hi, const u8 n) {
    return hi >> (n * 6 + 2) & 0x3F;   // +2 to skip hi[0:1]
}

static inline void ps_set_mid(u64 *restrict lo, u64 *restrict hi, const u8 sixbit) {
    *lo &= ~(0xFULL << 60);             // Clear lo[60:63]
    *hi &= ~0x3ULL;                     // Clear hi[0:1]
    *lo |= (u64)(sixbit & 0xF) << 60;   // Set lo[60:63]
    *hi |= sixbit >> 4 & 0x3;           // Set hi[0:1]
}

static inline void ps_set_lo(u64* lo, const u8 n, const u8 sixbit) {
    *lo &= ~(0x3FULL << (n * 6));
    *lo |= (u64)sixbit << (n * 6);
}

static inline void ps_set_hi(u64* hi, const u8 n, const u8 sixbit) {
    const i32 shift = n * 6 + 2; // +2 to skip hi[0:1]
    *hi &= ~(0x3FULL << shift);
    *hi |= (u64)sixbit << shift;
}

static inline u8 ps_get_n_sixbit(const u64 lo, const u64 hi, const u8 n) {
    if (n < 10)
        return lo >> (n * 6) & 0x3F;

    if (n == 10)
        return (hi & 0x3) << 4 | lo >> 60 & 0xF;

    // n > 10
    const i32 shift = (n - 11) * 6 + 2;
    return hi >> shift & 0x3F;
}

static inline void ps_set_n_sixbit(u64 *restrict lo, u64 *restrict hi, const u8 n, const u8 sixbit) {
    if (n < 10) {
        const u8 shift = (n * 6);
        *lo &= ~(0x3FULL << shift);
        *lo |= (u64)sixbit << shift;
    } else if (n == 10) {
        *lo &= ~(0xFULL << 60);
        *hi &= ~0x3ULL;
        *lo |= (u64)(sixbit & 0xF) << 60;
        *hi |= sixbit >> 4;
    } else {
        // n > 10 (11-19)
        const u8 shift = (n - 11) * 6 + 2;
        *hi &= ~(0x3FULL << shift);
        *hi |= (u64)sixbit << shift;
    }
}

static inline void ps_write_sixbit(u64 *restrict lo, u64 *restrict hi,
    const u8 sixbit, const u8 bitpos) {
    if (bitpos < 60) {
        *lo |= (u64)sixbit << bitpos;
    } else if (bitpos == 60) {
        *lo |= (u64)(sixbit & 0xF) << 60;
        *hi |= sixbit >> 4;
    } else {
        *hi |= (u64)sixbit << (bitpos - 64);
    }
}

static inline u8 ps_pack_metadata(const u8 length, const u8 flags) {
    return length << 3 | flags;
}

static inline u8 ps_extract_metadata(const u64 hi) {
    return hi >> 56 & 0xFFu;  // hi[56:63]
}

static inline void ps_insert_metadata(u64* hi, const u8 metadata) {
    *hi = *hi & 0x00FFFFFFFFFFFFFFULL | (u64)metadata << 56;
}

// Convert 6-bit value to char (internal)
static inline char ps_sixbit_to_char(const u8 sixbit) {
    return (sixbit < 64) ? PS_SIXBIT_TO_CHAR[sixbit] : '?';
}

// Convert char to 6-bit value (internal)
static inline u8 ps_char_to_sixbit(const char c) {
    const u8 r = PS_CHAR_TO_SIXBIT[(u8)c];
    return r != 0 || c == '0' ? r : UINT8_MAX;
}

// Check if char is in our alphabet
static inline bool ps_char_valid(const char c) {
    return PS_CHAR_TO_SIXBIT[(u8)c] != 0 || c == '0';  // '0' maps to 0
}

static inline bool ps_is_at(
    const u64 lo1, const u64 hi1, const u8 len1,
    const u64 lo2, const u64 hi2, const u8 len2,
    const u8 idx
)
{
    if (idx + len2 > len1) return false;

    const u32 start = (u32)idx * 6;
    const u32 bits  = (u32)len2 * 6;

    // Entirely inside lo
    if (start + bits <= 64) {
        const u64 mask = ((1ULL << bits) - 1) << start;
        return ((lo1 ^ (lo2 << start)) & mask) == 0;
    }

    // Entirely inside hi
    if (start >= 64) {
        const u32 shift = start - 64;
        const u64 mask  = (1ULL << bits) - 1;
        return ((hi1 >> shift) & mask) == (lo2 & mask);
    }

    // Cross boundary
    const u32 lo_bits = 64 - start;
    const u32 hi_bits = bits - lo_bits;

    const u64 lo_mask = (1ULL << lo_bits) - 1;
    const u64 hi_mask = (1ULL << hi_bits) - 1;

    const u64 p1_lo = (lo1 >> start) & lo_mask;
    const u64 p1_hi = hi1 & hi_mask;

    const u64 p2_lo = lo2 & lo_mask;
    const u64 p2_hi = lo_bits < 64
        ? ((lo2 >> lo_bits) | (hi2 << (64 - lo_bits))) & hi_mask
        : (hi2 & hi_mask);

    return (p1_lo == p2_lo) & (p1_hi == p2_hi);
}

static inline i8 ps_find(
    const u64 lo, const u64 hi,
    const u8 idx, const u8 sixbit
){
    switch (idx) {
        case 0: if ((lo & 0x3F) == sixbit) return 0;
        case 1: if ((lo >> 6 & 0x3F) == sixbit) return 1;
        case 2: if ((lo >> 12 & 0x3F) == sixbit) return 2;
        case 3: if ((lo >> 18 & 0x3F) == sixbit) return 3;
        case 4: if ((lo >> 24 & 0x3F) == sixbit) return 4;
        case 5: if ((lo >> 30 & 0x3F) == sixbit) return 5;
        case 6: if ((lo >> 36 & 0x3F) == sixbit) return 6;
        case 7: if ((lo >> 42 & 0x3F) == sixbit) return 7;
        case 8: if ((lo >> 48 & 0x3F) == sixbit) return 8;
        case 9: if ((lo >> 54 & 0x3F) == sixbit) return 9;

        case 10: if (((hi & 0x3) << 4 | lo >> 60 & 0xF) == sixbit) return 10;

        case 11: if ((hi >> 2 & 0x3F) == sixbit) return 11;
        case 12: if ((hi >> 8 & 0x3F) == sixbit) return 12;
        case 13: if ((hi >> 14 & 0x3F) == sixbit) return 13;
        case 14: if ((hi >> 20 & 0x3F) == sixbit) return 14;
        case 15: if ((hi >> 26 & 0x3F) == sixbit) return 15;
        case 16: if ((hi >> 32 & 0x3F) == sixbit) return 16;
        case 17: if ((hi >> 38 & 0x3F) == sixbit) return 17;
        case 18: if ((hi >> 44 & 0x3F) == sixbit) return 18;
        case 19: if ((hi >> 50 & 0x3F) == sixbit) return 19;
        default: return -1;
    }
}

static inline i8 ps_reverse_find(
    const u64 lo, const u64 hi,
    const u8 idx, const u8 sixbit
){
    switch (idx) {
        case 19: if ((hi >> 50 & 0x3F) == sixbit) return 19;
        case 18: if ((hi >> 44 & 0x3F) == sixbit) return 18;
        case 17: if ((hi >> 38 & 0x3F) == sixbit) return 17;
        case 16: if ((hi >> 32 & 0x3F) == sixbit) return 16;
        case 15: if ((hi >> 26 & 0x3F) == sixbit) return 15;
        case 14: if ((hi >> 20 & 0x3F) == sixbit) return 14;
        case 13: if ((hi >> 14 & 0x3F) == sixbit) return 13;
        case 12: if ((hi >> 8 & 0x3F) == sixbit) return 12;
        case 11: if ((hi >> 2 & 0x3F) == sixbit) return 11;

        case 10: if (((hi & 0x3) << 4 | lo >> 60 & 0xF) == sixbit) return 10;

        case 9: if ((lo >> 54 & 0x3F) == sixbit) return 9;
        case 8: if ((lo >> 48 & 0x3F) == sixbit) return 8;
        case 7: if ((lo >> 42 & 0x3F) == sixbit) return 7;
        case 6: if ((lo >> 36 & 0x3F) == sixbit) return 6;
        case 5: if ((lo >> 30 & 0x3F) == sixbit) return 5;
        case 4: if ((lo >> 24 & 0x3F) == sixbit) return 4;
        case 3: if ((lo >> 18 & 0x3F) == sixbit) return 3;
        case 2: if ((lo >> 12 & 0x3F) == sixbit) return 2;
        case 1: if ((lo >> 6 & 0x3F) == sixbit) return 1;
        case 0: if ((lo & 0x3F) == sixbit) return 0;

        default: return -1;
    }
}

static inline void ps_fill(
    u64 *restrict lo, u64 *restrict hi,
    const u8 sixbit, const u8 length
) {
    const u8 bit_len = length * 6;
    *lo = *hi = 0;

    for (u8 pos=0; pos < bit_len; pos+=6) {
        ps_write_sixbit(lo, hi, sixbit, pos);
    }
}

#endif // PACKED_HELPER_H