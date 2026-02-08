#include "packed-string.h"
#include <ctype.h>
#include <math.h>

// ============================================================================
// INTERNAL CONSTANTS AND HELPERS
// ============================================================================

// 6-bit to char conversion table
static const char PS_SIXBIT_TO_CHAR[64] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "_$";

// Char to 6-bit conversion (invalid chars map to 0)
static const u8 PS_CHAR_TO_SIXBIT[256] = {
    // 0-127: ASCII
    ['0'] =  0, ['1'] =  1, ['2'] =  2, ['3'] =  3, ['4'] =  4,
    ['5'] =  5, ['6'] =  6, ['7'] =  7, ['8'] =  8, ['9'] =  9,
    ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14,
    ['f'] = 15, ['g'] = 16, ['h'] = 17, ['i'] = 18, ['j'] = 19,
    ['k'] = 20, ['l'] = 21, ['m'] = 22, ['n'] = 23, ['o'] = 24,
    ['p'] = 25, ['q'] = 26, ['r'] = 27, ['s'] = 28, ['t'] = 29,
    ['u'] = 30, ['v'] = 31, ['w'] = 32, ['x'] = 33, ['y'] = 34,
    ['z'] = 35,
    ['A'] = 36, ['B'] = 37, ['C'] = 38, ['D'] = 39, ['E'] = 40,
    ['F'] = 41, ['G'] = 42, ['H'] = 43, ['I'] = 44, ['J'] = 45,
    ['K'] = 46, ['L'] = 47, ['M'] = 48, ['N'] = 49, ['O'] = 50,
    ['P'] = 51, ['Q'] = 52, ['R'] = 53, ['S'] = 54, ['T'] = 55,
    ['U'] = 56, ['V'] = 57, ['W'] = 58, ['X'] = 59, ['Y'] = 60,
    ['Z'] = 61,
    ['_'] = 62,
    ['$'] = 63
};

// Lookup table for lowercase conversion (6-bit values)
static const u8 TO_LOWER_TABLE[64] = {
    // 0-9: unchanged
    0,1,2,3,4,5,6,7,8,9,
    // a-z: unchanged
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,
    // A-Z: convert to a-z
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,
    // _ and $: unchanged
    62,63
};

// Lookup table for uppercase conversion
static const u8 TO_UPPER_TABLE[64] = {
    // 0-9: unchanged
    0,1,2,3,4,5,6,7,8,9,
    // a-z: convert to A-Z
    36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,
    // A-Z: unchanged
    36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,
    // _ and $: unchanged
    62,63
};

static inline u8 ps_get_n_sixbit(const u64 lo, const u64 hi, const u8 n) {
    if (n < 10) {
        return lo >> (n * 6) & 0x3F;
    }

    if (n > 10) {
        const int shift = (n - 11) * 6 + 2;  // +2 to skip hi[0:1]
        return hi >> shift & 0x3F;
    }

    // n == 10
    return (lo >> 60 & 0xF) << 2 | hi & 0x3;  // hi[0:1]
}

static inline void ps_insert_n_sixbit(u64* lo, u64* hi, const u8 n, const u8 sixbit) {
    if (n < 10) {
        *lo &= ~(0x3FULL << (n * 6));
        *lo |= (u64)sixbit << (n * 6);
    } else if (n > 10) {
        // n > 10 (11-19)
        const int shift = (n - 11) * 6 + 2;     // +2 to skip hi[0:1]
        *hi &= ~(0x3FULL << shift);
        *hi |= (u64)sixbit << shift;
    } else {
        *lo &= ~(0xFULL << 60);           // Clear lo[60:63]
        *hi &= ~0x3ULL;                   // Clear hi[0:1]
        *lo |= (u64)(sixbit >> 2) << 60;  // Set lo[60:63]
        *hi |= sixbit & 0x3;              // Set hi[0:1]
    }
}

static inline u8 ps_extract_metadata(const u64 hi) {
    return hi >> 56 & 0xFFu;  // hi[56:63]
}

static inline void ps_insert_metadata(u64* hi, const u8 metadata) {
    *hi = *hi & 0x00FFFFFFFFFFFFFFULL | (u64)metadata << 56;
}

static inline u8 ps_pack_metadata(const u8 length, const u8 flags) {
    return (u64)(length & 0x1F | flags << 5);
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

// ============================================================================
// CORE IMPLEMENTATION
// ============================================================================

PackedString ps_pack(const char* str) {
    if (!str) return PACKED_STRING_INVALID;

    u64 lo = 0, hi = 0;
    u8 length = 0;
    u8 flags = 0;
    bool has_upper = false;
    bool has_special = false;

    // Scan and analyze string
    while (str[length] && length < PACKED_STRING_MAX_LEN) {
        const char c = str[length];

        // Track case information
        if (c - 'A' <= 26) has_upper = true;
        if (c == '_' || c == '$') has_special = true;

        const u8 sixbit = ps_char_to_sixbit(c);
        if (sixbit == UINT8_MAX) {
            // Invalid character, return invalid
            return PACKED_STRING_INVALID;
        }

        ps_insert_n_sixbit(&lo, &hi, length, sixbit);
        length++;
    }

    // Check if too long
    if (str[length] != '\0') {
        // String too long, return invalid
        return PACKED_STRING_INVALID;
    }

    // Set flags
    if (has_upper) {
        // Mixed case - preserve it
        // All uppercase - could be constant
        flags |= PACKED_FLAG_CASE_SENSITIVE;
    }
    // All lowercase gets flag=0 (we'll store as lowercase)

    if (length > 0 && str[0] - '0' <= 9) {
        // Check first character is digit
        flags |= PACKED_FLAG_STARTS_WITH_DIGIT;
    }

    if (has_special) {
        flags |= PACKED_FLAG_CONTAINS_SPECIAL;
    }

    // Insert metadata into hi word (bits 60-63 of hi, and we'll use bits 60-67)
    // Actually, we need 8 bits for metadata, so we'll use top 8 bits of hi
    const u8 metadata = ps_pack_metadata(length, flags);
    ps_insert_metadata(&hi, metadata);

    return (PackedString){.lo = lo, .hi = hi};
}

PackedString ps_pack_ex(const char* str, const u8 length, const u8 flags) {
    if (!str || length > PACKED_STRING_MAX_LEN)
        return PACKED_STRING_INVALID;

    u64 lo = 0, hi = 0;

    for (u8 i = 0; i < length; i++) {
        char c = str[i];

        // Apply case folding if not case-sensitive
        c += ('a' - 'A') * (!(flags & PACKED_FLAG_CASE_SENSITIVE) && c - 'A' <= 26);

        const u8 sixbit = ps_char_to_sixbit(c);
        if (sixbit == UINT8_MAX) {
            // Invalid character, return invalid
            return PACKED_STRING_INVALID;
        }

        ps_insert_n_sixbit(&lo, &hi, i, sixbit);
    }

    // Insert metadata
    const u8 metadata = ps_pack_metadata(length, flags);
    ps_insert_metadata(&hi, metadata);

    return (PackedString){.lo = lo, .hi = hi};
}

char* ps_unpack(const PackedString ps, char* buffer) {
    if (!buffer || !ps_valid(ps))
        return NULL;

    const u8 length = ps_length(ps);

    for (u8 i = 0; i < length; i++) {
        const u8 sixbit = ps_get_n_sixbit(ps.lo, ps.hi, i);
        buffer[i] = ps_sixbit_to_char(sixbit);
    }

    buffer[length] = '\0';
    return buffer;
}

char* ps_unpack_ex(const PackedString ps, char* buffer, const u8 length, const u8 flags) {
    if (!buffer || length > PACKED_STRING_MAX_LEN || !ps_valid(ps))
        return NULL;

    for (u8 i = 0; i < length; i++) {
        const u8 sixbit = ps_get_n_sixbit(ps.lo, ps.hi, i);
        buffer[i] = ps_sixbit_to_char(sixbit);
    }

    buffer[length] = '\0';
    return buffer;
}

u8 ps_sixbit_at(const PackedString ps, const u8 index) {
    const u8 length = ps_length(ps);
    if (index >= length) return UINT8_MAX;

    return ps_get_n_sixbit(ps.lo, ps.hi, index);
}

char ps_char_at(const PackedString ps, const u8 index) {
    const u8 length = ps_length(ps);
    if (index >= length) return '\0';

    const u8 sixbit = ps_get_n_sixbit(ps.lo, ps.hi, index);
    return ps_sixbit_to_char(sixbit);
}

char ps_first_char(const PackedString ps) {
    const u8 sixbit = ps.lo & 0x3F;
    return ps_sixbit_to_char(sixbit);
}

char ps_last_char(const PackedString ps) {
    const u8 length = ps_length(ps);
    if (length == 0) return '\0';

    const u8 sixbit = ps_get_n_sixbit(ps.lo, ps.hi, length - 1);
    return ps_sixbit_to_char(sixbit);
}

// ============================================================================
// COMPARISON IMPLEMENTATIONS
// ============================================================================

bool ps_equal_nocase(const PackedString a, const PackedString b) {
    // Exact match (including case)
    if (ps_equal(a, b)) return true;

    const u8 len_a = ps_length(a);
    const u8 len_b = ps_length(b);

    // Different lengths can't be equal
    if (len_a != len_b) return false;

    // Process in 64-bit chunks where possible
    // First, handle characters 0-9 in lo

    // Create case-folded versions of lo chunks
    u64 a_lo_folded = 0;
    u64 b_lo_folded = 0;

    const u8 compare_lo_len = len_a > 10 ? 10 : len_a;

    for (u8 i = 0; i < compare_lo_len; i++) {
        const u8 a_sixbit = (a.lo >> (i * 6)) & 0x3F;
        const u8 b_sixbit = (b.lo >> (i * 6)) & 0x3F;

        a_lo_folded |= (u64)TO_LOWER_TABLE[a_sixbit] << (i * 6);
        b_lo_folded |= (u64)TO_LOWER_TABLE[b_sixbit] << (i * 6);
    }

    // Compare folded lo parts
    if (a_lo_folded != b_lo_folded) return false;

    // Check character 10 if present
    if (len_a > 10) {
        const u8 a_char10 = ((a.lo >> 60) & 0xF) | ((a.hi & 0x3) << 4);
        const u8 b_char10 = ((b.lo >> 60) & 0xF) | ((b.hi & 0x3) << 4);

        if (TO_LOWER_TABLE[a_char10] != TO_LOWER_TABLE[b_char10]) {
            return false;
        }
    }

    // Check characters 11-19 in hi
    if (len_a > 11) {
        const u8 hi_chars = len_a - 11;

        for (u8 i = 0; i < hi_chars; i++) {
            const u8 shift = i * 6 + 2;
            const u8 a_sixbit = (a.hi >> shift) & 0x3F;
            const u8 b_sixbit = (b.hi >> shift) & 0x3F;

            if (TO_LOWER_TABLE[a_sixbit] != TO_LOWER_TABLE[b_sixbit]) {
                return false;
            }
        }
    }

    return true;
}

int ps_compare(const PackedString a, const PackedString b) {
    // Fast equality check
    if (ps_equal(a, b)) return 0;

    const u8 len_a = ps_length(a);
    const u8 len_b = ps_length(b);
    const u8 min_len = len_a < len_b ? len_a : len_b;

    // Compare first min(10, min_len) characters from lo
    const u8 compare_lo_len = min_len > 10 ? 10 : min_len;
    const u64 mask_lo = (1ULL << (compare_lo_len * 6)) - 1;

    const u64 a_lo_part = a.lo & mask_lo;
    const u64 b_lo_part = b.lo & mask_lo;

    if (a_lo_part != b_lo_part) {
        // Find first differing character in lo
        u64 const diff = a_lo_part ^ b_lo_part;
        int const first_diff_bit = __builtin_ctzll(diff);
        int const char_pos = first_diff_bit / 6;

        const u8 a_char = (a.lo >> (char_pos * 6)) & 0x3F;
        const u8 b_char = (b.lo >> (char_pos * 6)) & 0x3F;
        return (a_char < b_char) ? -1 : 1;
    }

    // If we compared all characters in lo and min_len > 10, check char 10
    if (min_len > 10) {
        // Compare character 10 (split between lo[60:63] and hi[0:1])
        const u8 a_char10 = ((a.lo >> 60) & 0xF) | ((a.hi & 0x3) << 4);
        const u8 b_char10 = ((b.lo >> 60) & 0xF) | ((b.hi & 0x3) << 4);

        if (a_char10 != b_char10) {
            return (a_char10 < b_char10) ? -1 : 1;
        }
    }

    // Compare characters 11-19 in hi
    if (min_len > 11) {
        const u8 compare_hi_len = min_len - 11;
        const u64 mask_hi = (1ULL << (compare_hi_len * 6)) - 1;

        // hi[2:56] contains chars 11-19, shifted by 2 bits
        const u64 a_hi_part = (a.hi >> 2) & (mask_hi << 2);
        const u64 b_hi_part = (b.hi >> 2) & (mask_hi << 2);

        if (a_hi_part != b_hi_part) {
            // Find first differing character in hi
            const u64 diff = a_hi_part ^ b_hi_part;
            const int first_diff_bit = __builtin_ctzll(diff);
            const int char_pos = first_diff_bit / 6;  // Relative to hi

            const u8 a_char = (a.hi >> (char_pos * 6 + 2)) & 0x3F;
            const u8 b_char = (b.hi >> (char_pos * 6 + 2)) & 0x3F;
            return (a_char < b_char) ? -1 : 1;
        }
    }

    // All compared characters equal, compare lengths
    if (len_a < len_b) return -1;
    if (len_a > len_b) return 1;
    return 0;
}

// ============================================================================
// STRING OPERATIONS
// ============================================================================

bool ps_starts_with(const PackedString ps, const PackedString prefix) {
    const u8 len_ps = ps_length(ps);
    const u8 len_prefix = ps_length(prefix);

    if (len_prefix > len_ps) return false;

    if (len_prefix <= 10) {
        const u64 mask = (1ULL << (len_prefix * 6)) - 1;
        return (ps.lo & mask) == prefix.lo;
    }

    if (ps.lo == prefix.lo) {
        const u64 mask = (1ULL << ((len_prefix - 10) * 6)) - 1;
        return (ps.hi & mask) == (prefix.hi & mask);
    }

    return false;
}

bool ps_ends_with(const PackedString ps, const PackedString suffix) {
    const u8 len_ps = ps_length(ps);
    const u8 len_suffix = ps_length(suffix);

    if (len_suffix > len_ps) return false;


    if (len_suffix <= 10) {
        const u64 mask = (1ULL << (len_suffix * 6)) - 1;
        return (ps.hi & ~0xffull & mask) == (suffix.hi & ~0xffull & mask);
    }

    if (ps.lo == suffix.lo) {
        const u64 mask = (1ULL << ((len_suffix - 10) * 6)) - 1;
        return (ps.hi & mask) == (suffix.hi & mask);
    }

    return false;
}
