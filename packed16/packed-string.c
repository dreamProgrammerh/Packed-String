#include "packed-string.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "helper.h"

// ============================================================================
// CORE IMPLEMENTATION
// ============================================================================

PackedString ps_scan(const PackedString ps) {
    const u8 len = ps_length(ps);

    u8 flags = 0;

    // Scan characters in lo (0-9)
    for (u8 i = 0; i < 10 && i < len; i++) {
        const u8 sixbit = ps_get_lo(ps.lo, i);

        if (36 <= sixbit && sixbit <= 61) flags |= PACKED_FLAG_CONTAINS_DIGIT;
        else if (sixbit <= 9) flags |= PACKED_FLAG_CONTAINS_DIGIT;
        else if (sixbit == 62 || sixbit == 63) flags |= PACKED_FLAG_CONTAINS_SPECIAL;
    }

    // Process character 10 if exist
    if (len > 10) {
        const u8 sixbit = ps_get_mid(ps.lo, ps.hi);

        if (36 <= sixbit && sixbit <= 61) flags |= PACKED_FLAG_CONTAINS_DIGIT;
        else if (sixbit <= 9) flags |= PACKED_FLAG_CONTAINS_DIGIT;
        else if (sixbit == 62 || sixbit == 63) flags |= PACKED_FLAG_CONTAINS_SPECIAL;
    }

    // Process characters 11-19 in hi
    const u8 hi_len = len < 11 ? 0 : len - 11;
    for (u8 i = 0; i < hi_len; i++) {
        const u8 sixbit = ps_get_hi(ps.hi, i);

        if (36 <= sixbit && sixbit <= 61) flags |= PACKED_FLAG_CONTAINS_DIGIT;
        else if (sixbit <= 9) flags |= PACKED_FLAG_CONTAINS_DIGIT;
        else if (sixbit == 62 || sixbit == 63) flags |= PACKED_FLAG_CONTAINS_SPECIAL;
    }

    u64 hi = ps.hi;
    ps_insert_metadata(&hi, ps_pack_metadata(len, flags));
    return (PackedString){.lo = ps.lo, .hi = hi};
}

PackedString ps_pack(const char* str) {
    if (!str) return PACKED_STRING_INVALID;

    u64 lo = 0, hi = 0;
    u8 length = 0, flags = 0;

    // Scan and analyze string
    while (str[length] && length < PACKED_STRING_MAX_LEN) {
        const char c = str[length];

        // Track information
        if ('A' <= c && c <= 'Z') flags |= PACKED_FLAG_CASE_SENSITIVE;
        else if ('0' <= c && c <= '9') flags |= PACKED_FLAG_CONTAINS_DIGIT;
        else if (c == '_' || c == '$') flags |= PACKED_FLAG_CONTAINS_SPECIAL;

        const u8 sixbit = ps_char_to_sixbit(c);
        if (sixbit == UINT8_MAX) {
            // Invalid character, return invalid
            return PACKED_STRING_INVALID;
        }

        ps_write_sixbit(&lo, &hi, sixbit, length * 6);
        length++;
    }

    // Check if too long
    if (str[length] != '\0') {
        // String too long, return invalid
        return PACKED_STRING_INVALID;
    }

    // Insert metadata into hi
    const u8 metadata = ps_pack_metadata(length, flags);
    ps_insert_metadata(&hi, metadata);

    return (PackedString){.lo = lo, .hi = hi};
}

PackedString ps_pack_ex(const char* str, const u8 length, const u8 flags) {
    if (!str || length > PACKED_STRING_MAX_LEN)
        return PACKED_STRING_INVALID;

    u64 lo = 0, hi = 0;
    u8 new_flags = 0;
    const bool
        cannot_have_upper = (flags & PACKED_FLAG_CASE_SENSITIVE) == 0,
        cannot_have_digit = (flags & PACKED_FLAG_CONTAINS_DIGIT) == 0,
        cannot_have_special = (flags & PACKED_FLAG_CONTAINS_SPECIAL) == 0;

    for (u8 i = 0; i < length; i++) {
        char c = str[i];
        const bool
            is_digit = '0' <= c && c <= '9',
            is_upper = 'A' <= c && c <= 'Z',
            is_special = c == '_' || c == '$';

        // Track information
        if (is_upper) new_flags |= PACKED_FLAG_CASE_SENSITIVE;
        else if (is_digit) new_flags |= PACKED_FLAG_CONTAINS_DIGIT;
        else if (is_special) new_flags |= PACKED_FLAG_CONTAINS_SPECIAL;

        // Apply case folding if not case-sensitive
        if (cannot_have_upper && is_upper)
            c += 'a' - 'A';

        if (cannot_have_digit && is_digit)
            return PACKED_STRING_INVALID;

        if (cannot_have_special && is_special)
            return PACKED_STRING_INVALID;

        const u8 sixbit = ps_char_to_sixbit(c);
        if (sixbit == UINT8_MAX)
            // Invalid character, return invalid
            return PACKED_STRING_INVALID;

        ps_write_sixbit(&lo, &hi, sixbit, i * 6);
    }

    // Insert metadata
    const u8 metadata = ps_pack_metadata(length, new_flags);
    ps_insert_metadata(&hi, metadata);

    return (PackedString){.lo = lo, .hi = hi};
}

i32 ps_unpack(const PackedString ps, char* buffer) {
    if (!buffer || !ps_valid(ps))
        return -1;

    const u8 length = ps_length(ps);

    for (u8 i = 0; i < length; i++) {
        const u8 sixbit = ps_get_n_sixbit(ps.lo, ps.hi, i);
        buffer[i] = ps_sixbit_to_char(sixbit);
    }

    buffer[length] = '\0';
    return length;
}

i32 ps_unpack_ex(const PackedString ps, char* buffer, const u8 length, const u8 flags) {
    if (!buffer || length > PACKED_STRING_MAX_LEN || !ps_valid(ps))
        return -1;

    const bool
        cannot_have_upper = (flags & PACKED_FLAG_CASE_SENSITIVE) == 0,
        cannot_have_digit = (flags & PACKED_FLAG_CONTAINS_DIGIT) == 0,
        cannot_have_special = (flags & PACKED_FLAG_CONTAINS_SPECIAL) == 0;

    u8 len = 0;
    for (u8 i = 0; i < length; i++) {
        u8 sixbit = ps_get_n_sixbit(ps.lo, ps.hi, i);

        if (cannot_have_digit && sixbit <= 9) continue;
        if (cannot_have_special && (sixbit == 62 || sixbit == 63)) continue;
        if (cannot_have_upper) sixbit = TO_LOWER_TABLE[sixbit];

        buffer[len++] = ps_sixbit_to_char(sixbit);
    }

    buffer[len] = '\0';
    return len;
}

u8 ps_set(PackedString* ps, const u8 index, const u8 sixbit) {
    const u8 length = ps_length(*ps);
    if (index >= length) return UINT8_MAX;

    ps_set_n_sixbit(&ps->lo, &ps->hi, index, sixbit);
    return sixbit;
}

u8 ps_at(const PackedString ps, const u8 index) {
    const u8 length = ps_length(ps);
    if (index >= length) return UINT8_MAX;

    return ps_get_n_sixbit(ps.lo, ps.hi, index);
}

u8 ps_first(const PackedString ps) {
    const u8 length = ps_length(ps);
    if (length == 0) return UINT8_MAX;

    return ps.lo & 0x3F;
}

u8 ps_last(const PackedString ps) {
    const u8 length = ps_length(ps);
    if (length == 0) return UINT8_MAX;

    return ps_get_n_sixbit(ps.lo, ps.hi, length - 1);
}

// ============================================================================
// COMPARISON IMPLEMENTATIONS
// ============================================================================

bool ps_equal_nocase(const PackedString a, const PackedString b) {
    // Equal match (including case)
    if (ps_equal_nometa(a, b)) return true;

    const u8 len_a = ps_length(a);
    const u8 len_b = ps_length(b);

    // Different lengths can't be equal
    if (len_a != len_b) return false;

    const u8 lo_len = len_a > 10 ? 10 : len_a;

    // First, handle characters 0-9 in lo
    for (u8 i = 0; i < lo_len; i++) {
        const u8 a_sixbit = ps_get_lo(a.lo, i);
        const u8 b_sixbit = ps_get_lo(b.lo, i);

        if (TO_LOWER_TABLE[a_sixbit] != TO_LOWER_TABLE[b_sixbit])
            return false;
    }

    // Check character 10 if present
    if (len_a > 10) {
        const u8 a_char10 = ps_get_mid(a.lo, b.lo);
        const u8 b_char10 = ps_get_mid(a.lo, b.lo);

        if (TO_LOWER_TABLE[a_char10] != TO_LOWER_TABLE[b_char10])
            return false;
    }

    if (len_a <= 11) return true;

    const u8 hi_len = len_a - 11;

    // Check characters 11-19 in hi
    for (u8 i = 0; i < hi_len; i++) {
        const u8 a_sixbit = ps_get_hi(a.hi, i);
        const u8 b_sixbit = ps_get_hi(b.hi, i);

        if (TO_LOWER_TABLE[a_sixbit] != TO_LOWER_TABLE[b_sixbit])
            return false;
    }

    return true;
}

i32 ps_compare(const PackedString a, const PackedString b) {
    // Fast equality check
    if (ps_equal_nometa(a, b)) return 0;

    const u8 la = ps_length(a);
    const u8 lb = ps_length(b);

    const u8 min = la < lb ? la : lb;

    /* Compare first 10 characters */
    if (min < 10) {
        const u64 mask = (1ULL << (min * 6)) - 1;
        const u64 da = a.lo & mask;
        const u64 db = b.lo & mask;

        if (da != db)
            return da < db ? -1 : 1;

        return (i32)la - (i32)lb;
    }

    if (a.lo != b.lo) {
        return a.lo < b.lo ? -1 : 1;
    }

    /* Compare middle character (char 10) */
    if (min > 10) {
        const u8 ca = ((a.hi & 0x3) << 4) | (a.lo >> 60);
        const u8 cb = ((b.hi & 0x3) << 4) | (b.lo >> 60);

        if (ca != cb) return ca < cb ? -1 : 1;
    } else if (min == 10) {
        return (i32)la - (i32)lb;
    }

    /* Compare remaining high block (11–19) */
    if (min > 11) {
        const u32 bits = (min - 11) * 6;
        const u64 mask = (1ULL << bits) - 1;

        const u64 ha = (a.hi >> 2) & mask;
        const u64 hb = (b.hi >> 2) & mask;

        if (ha != hb) return ha < hb ? -1 : 1;
    }

    return (i32)la - (i32)lb;
}

// ============================================================================
// STRING OPERATIONS
// ============================================================================

bool ps_starts_with(const PackedString ps, const PackedString prefix) {
    const u8 len_ps = ps_length(ps);
    const u8 len_prefix = ps_length(prefix);

    if (len_prefix > len_ps) return false;

    if (len_prefix < 11) {
        const u64 mask = (1ULL << (len_prefix * 6)) - 1;
        return (ps.lo & mask) == prefix.lo;
    }

    if (ps.lo == prefix.lo) {
        if (len_prefix == 11) {
            const u64 mask = 0x3;
            return (ps.hi & mask) == (prefix.hi & mask);
        }

        const u64 mask = ((1ULL << ((len_prefix - 11) * 6 + 2)) - 1);
        return (ps.hi & mask) == (prefix.hi & mask);
    }

    return false;
}

bool ps_ends_with(const PackedString ps, const PackedString suffix) {
    const u8 len_ps = ps_length(ps);
    const u8 len_suffix = ps_length(suffix);

    return ps_is_at(
        ps.lo, ps.hi, len_ps,
        suffix.lo, suffix.hi, len_suffix,
        len_ps - len_suffix
    );
}

bool ps_starts_with_at(const PackedString ps, const PackedString prefix, const u8 start) {
    const u8 len_ps = ps_length(ps);
    const u8 len_prefix = ps_length(prefix);

    if (start + len_prefix > len_ps) return false;

    return ps_is_at(
        ps.lo, ps.hi, len_ps,
        prefix.lo, prefix.hi, len_prefix,
        start
    );
}

bool ps_ends_with_at(const PackedString ps, const PackedString suffix, const u8 end) {
    const u8 len_ps = ps_length(ps);
    const u8 len_suffix = ps_length(suffix);

    if (len_ps - end < len_suffix) return false;

    return ps_is_at(
        ps.lo, ps.hi, len_ps,
        suffix.lo, suffix.hi, len_suffix,
        len_ps - len_suffix - end
    );
}

PackedString ps_skip(const PackedString ps, const u8 start) {
    if (start == 0) return ps;

    const u8 len = ps_length(ps);
    if (start > len) return ps_empty();

    u64 lo = ps.lo, hi = ps.hi & 0x00FFFFFFFFFFFFFFFFULL;
    ps_shr128(&lo, &hi, start * 6);

    ps_insert_metadata(&hi, ps_pack_metadata(len - start, 0));
    return (PackedString){ .lo=lo, .hi=hi };
}

PackedString ps_trunc(const PackedString ps, const u8 length) {
    if (length == 0) return ps_empty();
    if (length >= ps_length(ps)) return ps;

    u64 lo = ps.lo, hi = ps.hi;
    ps_limit(&lo, &hi, length);

    ps_insert_metadata(&hi, ps_pack_metadata(length, 0));
    return (PackedString){ .lo=lo, .hi=hi };
}

PackedString ps_substring(const PackedString ps, const u8 start, const u8 length) {
    const u8 total = ps_length(ps);

    if (length == 0 || start + length > total)
        return ps_empty();

    u64 lo = ps.lo, hi = ps.hi;

    const u32 bit_start = (u32)start * 6;

    if (bit_start != 0)
        ps_shr128(&lo, &hi, bit_start);

    ps_limit(&lo, &hi, length);

    const u8 meta = ps_pack_metadata(length, 0);
    ps_insert_metadata(&hi, meta);

    return (PackedString){ .lo=lo, .hi=hi };
}

PackedString ps_concat(const PackedString a, const PackedString b) {
    const u8 len_a = ps_length(a);
    const u8 len_b = ps_length(b);

    if (len_a == 20) return a;

    const u8 a_bits = len_a * 6;
    u64 lo = b.lo, hi = b.hi;

    ps_shl128(&lo, &hi, a_bits);

    lo |= a.lo;
    hi |= a.hi;

    const u8 new_length = len_a + len_b > 20 ? 20 : len_a + len_b;
    const u8 new_flags = ps_flags(a) | ps_flags(b);
    const u8 metadata = ps_pack_metadata(new_length, new_flags);
    ps_insert_metadata(&hi, metadata);

    return (PackedString){ .lo=lo, .hi=hi };
}

PackedString ps_to_lower(const PackedString ps) {
    const u8 len = ps_length(ps);
    u64 lo = ps.lo, hi = ps.hi;

    // Process characters in lo (0-9)
    for (u8 i = 0; i < 10 && i < len; i++) {
        u8 sixbit = ps_get_lo(lo, i);
        sixbit = TO_LOWER_TABLE[sixbit];
        ps_set_lo(&lo, i, sixbit);
    }

    // Process character 10 if exist
    if (len > 10) {
        u8 sixbit = ps_get_mid(lo, hi);
        sixbit = TO_LOWER_TABLE[sixbit];
        ps_set_mid(&lo, &hi, sixbit);
    }

    // Process characters 11-19 in hi
    for (u8 i = 0, l = len - 11; i < l; i++) {
        u8 sixbit = ps_get_hi(hi, i);
        sixbit = TO_LOWER_TABLE[sixbit];
        ps_set_hi(&hi, i, sixbit);
    }

    // Clear CASE_SENSITIVE flag since we're now lowercase
    const u8 flags = ps_flags(ps) & ~PACKED_FLAG_CASE_SENSITIVE;
    const u8 metadata = ps_pack_metadata(len, flags);
    ps_insert_metadata(&hi, metadata);

    return (PackedString){ .lo=lo, .hi=hi };
}

PackedString ps_to_upper(const PackedString ps) {
    const u8 len = ps_length(ps);
    u64 lo = ps.lo, hi = ps.hi;

    // Process characters in lo (0-9)
    for (u8 i = 0; i < 10 && i < len; i++) {
        u8 sixbit = ps_get_lo(lo, i);
        sixbit = TO_UPPER_TABLE[sixbit];
        ps_set_lo(&lo, i, sixbit);
    }

    // Process character 10 if exist
    if (len > 10) {
        u8 sixbit = ps_get_mid(lo, hi);
        sixbit = TO_UPPER_TABLE[sixbit];
        ps_set_mid(&lo, &hi, sixbit);
    }

    // Process characters 11-19 in hi
    for (u8 i = 0, l = len - 11; i < l; i++) {
        u8 sixbit = ps_get_hi(hi, i);
        sixbit = TO_UPPER_TABLE[sixbit];
        ps_set_hi(&hi, i, sixbit);
    }

    // Set CASE_SENSITIVE flag since we're preserving uppercase
    const u8 flags = ps_flags(ps) | PACKED_FLAG_CASE_SENSITIVE;
    const u8 metadata = ps_pack_metadata(len, flags);
    ps_insert_metadata(&hi, metadata);

    return (PackedString){ .lo=lo, .hi=hi };
}

PackedString ps_pad_left(const PackedString ps, const u8 sixbit, const u8 length) {
    const u8 len = ps_length(ps);
    if (len >= length) return ps;
    u8 flags = ps_flags(ps);

    if (36 <= sixbit && sixbit <= 61) flags |= PACKED_FLAG_CONTAINS_DIGIT;
    else if (sixbit <= 9) flags |= PACKED_FLAG_CONTAINS_DIGIT;
    else if (sixbit == 62 || sixbit == 63) flags |= PACKED_FLAG_CONTAINS_SPECIAL;

    const u8 pad_len = length - len;
    u64 pad_lo, pad_hi, lo = ps.lo, hi = ps.hi;

    ps_fill(&pad_lo, &pad_hi, sixbit, pad_len);
    ps_shl128(&lo, &hi, pad_len * 6);

    lo |= pad_lo;
    hi |= pad_hi;

    ps_insert_metadata(&hi, ps_pack_metadata(length, flags));
    return (PackedString){ .lo=lo, .hi=hi };
}

PackedString ps_pad_right(const PackedString ps, const u8 sixbit, const u8 length) {
    const u8 len = ps_length(ps);
    if (len >= length) return ps;
    u8 flags = ps_flags(ps);

    if (36 <= sixbit && sixbit <= 61) flags |= PACKED_FLAG_CONTAINS_DIGIT;
    else if (sixbit <= 9) flags |= PACKED_FLAG_CONTAINS_DIGIT;
    else if (sixbit == 62 || sixbit == 63) flags |= PACKED_FLAG_CONTAINS_SPECIAL;

    const u8 pad_len = length - len;
    u64 pad_lo, pad_hi, lo = ps.lo, hi = ps.hi;

    ps_fill(&pad_lo, &pad_hi, sixbit, pad_len);
    ps_shl128(&pad_lo, &pad_hi, len * 6);

    lo |= pad_lo;
    hi |= pad_hi;

    ps_insert_metadata(&hi, ps_pack_metadata(length, flags));
    return (PackedString){ .lo=lo, .hi=hi };
}

PackedString ps_pad_center(const PackedString ps, const u8 sixbit, const u8 length) {
    const u8 len = ps_length(ps);
    if (len >= length) return ps;
    u8 flags = ps_flags(ps);

    if (36 <= sixbit && sixbit <= 61) flags |= PACKED_FLAG_CONTAINS_DIGIT;
    else if (sixbit <= 9) flags |= PACKED_FLAG_CONTAINS_DIGIT;
    else if (sixbit == 62 || sixbit == 63) flags |= PACKED_FLAG_CONTAINS_SPECIAL;

    const u8 pad_len = length - len;
    const u8 padl_len = pad_len / 2;
    const u8 padr_len = pad_len - padl_len;
    u64 padl_lo, padl_hi, padr_lo, padr_hi,
        lo = ps.lo, hi = ps.hi;

    ps_fill(&padr_lo, &padr_hi, sixbit, padr_len);
    ps_fill(&padl_lo, &padl_hi, sixbit, padl_len);
    ps_shl128(&lo, &hi, padl_len * 6);
    ps_shl128(&padr_lo, &padr_hi, (padl_len + len) * 6);

    lo |= padl_lo | padr_lo;
    hi |= padl_hi | padr_hi;

    ps_insert_metadata(&hi, ps_pack_metadata(length, flags));
    return (PackedString){ .lo=lo, .hi=hi };
}

// ============================================================================
// SEARCH OPERATIONS
// ============================================================================

i8 ps_find_six(const PackedString ps, const u8 sixbit) {
    if (sixbit >= 64) return -1;

    const u8 len = ps_length(ps);
    if (len == 0) return -1;

    return ps_find(ps.lo, ps.hi, 0, sixbit);
}

i8 ps_find_from_six(const PackedString ps, const u8 sixbit, const u8 start) {
    if (sixbit >= 64) return -1;

    const u8 len = ps_length(ps);
    if (start >= len) return -1;

    return ps_find(ps.lo, ps.hi, start, sixbit);
}

i8 ps_find_last_six(const PackedString ps, const u8 sixbit) {
    if (sixbit >= 64) return -1;

    const u8 len = ps_length(ps);
    if (len == 0) return -1;

    return ps_reverse_find(ps.lo, ps.hi, len - 1, sixbit);
}

bool ps_contains_six(const PackedString ps, const u8 sixbit) {
    if (sixbit >= 64) return false;

    const u8 len = ps_length(ps);
    if (len == 0) return false;

    return ps_reverse_find(ps.lo, ps.hi, len - 1, sixbit) != -1;
}

bool ps_contains(const PackedString ps, const PackedString pat) {
    const u8 n = ps_length(ps);
    const u8 m = ps_length(pat);

    if (m > n) return false;

    for (u8 i = 0; i <= n - m; ++i) {
        if (ps_is_at(ps.lo, ps.hi, n,
            pat.lo, pat.hi, m, i))
            return true;
    }
    return false;
}

// ============================================================================
// HASHING
// ============================================================================

u32 ps_hash32(const PackedString ps) {
    // Fast hash mixing lo and hi directly
    u64 combined = ps.lo ^ ps.hi;

    // Mix the bits (MurmurHash3 finalizer)
    combined ^= combined >> 33;
    combined *= 0xff51afd7ed558ccdULL;
    combined ^= combined >> 33;
    combined *= 0xc4ceb9fe1a85ec53ULL;
    combined ^= combined >> 33;

    return (u32)combined ^ (u32)(combined >> 32);
}

u64 ps_hash64(const PackedString ps) {
    // Mix lo and hi
    u64 h = ps.lo ^ ps.hi;

    // MurmurHash3 64-bit finalizer
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;

    return h;
}

PackedString ps_lock(const PackedString ps, const PackedString key) {
    const u64 mask_hi = 0x07FFFFFFFFFFFFFFULL;  // Lower 59 bits of hi
    u64 lo = ps.lo, hi = ps.hi;

    const u8 rotate = ps_length(key);
    if (rotate == 0) return ps_empty();

    /* Save last 5 bits (length) */
    const u64 save = hi & 0xF800000000000000ULL;
    const u64 hi_masked = hi & mask_hi;
    const u64 lo_orig = lo;

    // Left rotate 123 bits by rotate
    lo = (lo << rotate) | (hi_masked >> (59 - rotate));
    hi = (((hi_masked << rotate) & mask_hi) | (lo_orig >> (64 - rotate)));

    // Restore save
    hi |= save;

    /* Xor with key */
    lo ^= key.lo;
    hi ^= key.hi & mask_hi;

    return (PackedString){ .lo = lo, .hi = hi };
}

PackedString ps_unlock(const PackedString ps, const PackedString key) {
    const u64 mask_hi = 0x07FFFFFFFFFFFFFFULL;  // Lower 59 bits of hi
    u64 lo = ps.lo, hi = ps.hi;

    const u8 rotate = ps_length(key);
    if (rotate == 0) return ps_empty();

    /* First XOR with key */
    lo ^= key.lo;
    hi ^= (key.hi & mask_hi);

    /* Save last 5 bits (length) */
    const u64 save = hi & 0xF800000000000000ULL;
    const u64 hi_masked = hi & mask_hi; // rotated 59-bit part

    // Right rotate (Reverse the left rotate)
    // Extract components
    const u64 H_high = lo & ((1ULL << rotate) - 1);         // top 'rotate' bits of original hi
    const u64 H_low = (hi_masked >> rotate) & ((1ULL << (59 - rotate)) - 1);
    hi = (H_high << (59 - rotate)) | H_low;

    const u64 L_high = hi_masked & ((1ULL << rotate) - 1);  // top 'rotate' bits of original lo
    const u64 L_low = lo >> rotate;                         // low (64 - rotate) bits of original lo
    lo = (L_high << (64 - rotate)) | L_low;

    // Restore save
    hi |= save;

    return (PackedString){ .lo = lo, .hi = hi };
}

// ============================================================================
// VALIDATION & UTILITIES
// ============================================================================

bool ps_is_valid_identifier(const PackedString ps) {
    // if (ps_length(ps) == 0) return false;
    // No need to check the length,
    // because if length is zero then
    // first char is 0, and it's a digit so it returns false

    // Doesn't start with digit
    return (ps.lo & 0x3F) > 9;
}

// ============================================================================
// DEBUGGING & FORMATTING
// ============================================================================

i32 psd_hex(const PackedString ps, char* buffer) {
    if (!buffer) return -1;

    // Format as 32-character hex: 16 bytes = 32 hex chars
    // hi first (big-endian style), then lo
    const i32 len = snprintf(buffer, 33,
        "%016llX%016llX", ps.hi, ps.lo);

    return len;
}

#define BIT(c) (c) & 1 ? '1' : '0'

i32 psd_binary(const PackedString ps, char* buffer) {
    if (!buffer) return -1;
    char* ptr = buffer;

    // Simple binary: 128 bits continuous
    // Print hi word first (big-endian style)
    for (i8 i = 63; i >= 0; i--) {
        *ptr++ = BIT(ps.hi >> i);
    }

    // Separator between words
    *ptr++ = ' ';

    // Print lo word
    for (i8 i = 63; i >= 0; i--) {
        *ptr++ = BIT(ps.lo >> i);
    }

    // Close buffer and return length
    *ptr = '\0';
    return (i32)((size_t)ptr - (size_t)buffer);
}

i32 psd_encoding_binary(const PackedString ps, char* buffer) {
    if (!buffer) return -1;
    char* ptr = buffer;

    // Format with separation:
    // - [20 chars * 6 bits] [8 bits metadata]
    const u8 len = ps_length(ps);

    // Print characters 0-9 from lo (60 bits)
    for (u8 i = 0; i < 10; i++) {
        // Get 6-bit character
        const u8 sixbit = i < len
            ? ps_get_lo(ps.lo, i)
            : 0; // Unused character slot

        // Print as 6 binary digits
        for (i8 bit = 5; bit >= 0; bit--) {
            *ptr++ = (BIT(sixbit >> bit));
        }

        // Separator between characters
        if (i < 9) *ptr++ = ' ';
    }

    // Separator between lo and char10
    *ptr++ = ' '; *ptr++ = '|'; *ptr++ = ' ';

    // Print character 10 (split between lo[60:63] and hi[0:1])
    if (len > 10) {
        const u8 char10 = ps_get_mid(ps.lo, ps.hi);
        for (i8 bit = 5; bit >= 0; bit--) {
            *ptr++ = BIT(char10 >> bit);
        }
    } else {
        // Unused character 10
        for (i8 bit = 5; bit >= 0; bit--) {
            *ptr++ = '0';
        }
    }

    // Separator between char10 and chars 11-19
    *ptr++ = ' '; *ptr++ = '|'; *ptr++ = ' ';

    // Print characters 11-19 from hi[2:56] (54 bits)
    for (u8 i = 0; i < 9; i++) {
        const u8 sixbit = i + 11 < len
            ? ps_get_hi(ps.hi, i)
            : 0; // Unused character slot

        for (i8 bit = 5; bit >= 0; bit--) {
            *ptr++ = BIT(sixbit >> bit);
        }

        if (i < 8) *ptr++ = ' ';
    }

    // Separator between characters and metadata
    *ptr++ = ' '; *ptr++ = '|'; *ptr++ = ' ';

    // Print metadata (8 bits in hi[56:63])
    const u8 metadata = ps_extract_metadata(ps.hi);

    for (i8 bit = 2; bit >= 0; bit--) *ptr++ = BIT(metadata >> bit);
    *ptr++ = ':';  // Mark length/flags boundary
    for (i8 bit = 7; bit >= 3; bit--) *ptr++ = BIT(metadata >> bit);

    *ptr = '\0';
    return (i32)((size_t)ptr - (size_t)buffer);
}

#undef BIT

i32 psd_info(const PackedString ps, char* buffer) {
    if (!buffer) return -1;

    char str_buf[PACKED_STRING_MAX_LEN + 1];
    ps_unpack(ps, str_buf);

    const u8 length = ps_length(ps);
    const u8 flags = ps_flags(ps);
    const u8 metadata = ps_extract_metadata(ps.hi);

    // Get flag descriptions
    const char* case_str = (flags & PACKED_FLAG_CASE_SENSITIVE) ? "preserve" : "lowercase";
    const char* digit_str = (flags & PACKED_FLAG_CONTAINS_DIGIT) ? "has-digit" : "no-digit";
    const char* special_str = (flags & PACKED_FLAG_CONTAINS_SPECIAL) ? "has-special" : "no-special";

    // Build flag string
    char flag_buf[64] = "";
    if (flags) {
        char* fptr = flag_buf;
        if (flags & PACKED_FLAG_CASE_SENSITIVE) {
            fptr += snprintf(fptr, sizeof(flag_buf), "case ");
        }
        if (flags & PACKED_FLAG_CONTAINS_DIGIT) {
            fptr += snprintf(fptr, sizeof(flag_buf) - (fptr - flag_buf), "%sdigit ",
                           fptr > flag_buf ? "| " : "");
        }
        if (flags & PACKED_FLAG_CONTAINS_SPECIAL) {
            fptr += snprintf(fptr, sizeof(flag_buf) - (fptr - flag_buf), "%sspecial ",
                           fptr > flag_buf ? "| " : "");
        }
        // Remove trailing space
        if (fptr > flag_buf) *(fptr - 1) = '\0';
    } else {
        strcpy(flag_buf, "none");
    }

    // Get character breakdown
    char chars_buf[128]  = "";
    for (u8 i = 0; i < length; i++) {
        const u8 sixbit = ps_at(ps, i);
        const char c = ps_six(sixbit);
        snprintf(chars_buf + strlen(chars_buf),
                 sizeof(chars_buf) - strlen(chars_buf),
                 "%c(%02u) ", c, sixbit);
    }
    if (length == 0) {
        strcpy(chars_buf, "[empty]");
    }

    // Get bit layout
    char layout_buf[256]  = "";
    snprintf(layout_buf, sizeof(layout_buf),
             "lo[0:59]=chars0-9 lo[60:63]+hi[0:1]=char10 "
             "hi[2:55]=chars11-19 hi[56:63]=metadata");

    // Format final string
    const i32 len = snprintf(buffer, 512,
             "PackedString {\n"
             "  string:   \"%s\"\n"
             "  length:   %u\n"
             "  metadata: 0x%02X (len=%u, flags=0x%01X)\n"
             "  flags:    %s\n"
             "  chars:    %s\n"
             "  layout:   %s\n"
             "  valid:    %s\n"
             "  hex:      %016llX%016llX\n"
             "  case:     %s\n"
             "  digit:    %s\n"
             "  special:  %s\n"
             "}",
             str_buf,
             length,
             metadata, length, flags,
             flag_buf,
             chars_buf,
             layout_buf,
             ps_valid(ps) ? "yes" : "NO (invalid)",
             ps.hi, ps.lo,
             case_str,
             digit_str,
             special_str);

    return len;
}

i32 psd_visualize_bits(const PackedString ps, char* buffer) {
    if (!buffer) return -1;

    char* ptr = buffer;
    const u8 len = ps_length(ps);
    const u8 length = ps_length(ps);
    const u8 flags = ps_flags(ps);

    const size_t save = (size_t)ptr;
    ptr += sprintf(ptr, " indx: ");

    // Header: character positions
    for (u8 i = 0; i < 20; i++) {
        if (i == 10 || i == 11) {
            ptr += sprintf(ptr, "| ");  // Separator for char10
        }
        ptr += sprintf(ptr, "%2d ", i);
    }
    ptr += sprintf(ptr, "| metadata");

    const size_t lineLength = (size_t)ptr - save + 1;
    *ptr++ = '\n';

    // Separator line
    for (int i = 0; i < lineLength; i++) *ptr++ = '-';
    *ptr++ = '\n';

    // Lo part: characters 0-9
    ptr += sprintf(ptr, " code:");
    for (u8 i = 0; i < 10; i++) {
        if (i < len) {
            const u8 sixbit = (ps.lo >> (i * 6)) & 0x3F;
            ptr += sprintf(ptr, " %02X", sixbit);
        } else {
            ptr += sprintf(ptr, " --");
        }
    }

    // Char10 (split)
    ptr += sprintf(ptr, " | ");
    if (len > 10) {
        const u8 char10 = ((ps.lo >> 60) & 0xF) | ((ps.hi & 0x3) << 4);
        ptr += sprintf(ptr, "%02X", char10);
    } else {
        ptr += sprintf(ptr, "--");
    }

    // Hi part: characters 11-19
    ptr += sprintf(ptr, " |");
    for (u8 i = 0; i < 9; i++) {
        if (i + 11 < len) {
            const u8 sixbit = (ps.hi >> (i * 6 + 2)) & 0x3F;
            ptr += sprintf(ptr, " %02X", sixbit);
        } else {
            ptr += sprintf(ptr, " --");
        }
    }

    // Metadata
    ptr += sprintf(ptr, " | %02X %02X\n",
        length, flags);

    // Second line: actual characters
    ptr += sprintf(ptr, " char:");
    for (u8 i = 0; i < 10; i++) {
        if (i < len) {
            const char c = ps_six(ps_at(ps, i));
            ptr += sprintf(ptr, "  %c", c);
        } else {
            ptr += sprintf(ptr, "  .");
        }
    }

    ptr += sprintf(ptr, " | ");
    if (len > 10) {
        const char c10 = ps_six(ps_at(ps, 10));
        ptr += sprintf(ptr, " %c", c10);
    } else {
        ptr += sprintf(ptr, " .");
    }

    ptr += sprintf(ptr, " |");
    for (u8 i = 11; i < 20; i++) {
        if (i < len) {
            const char c = ps_six(ps_at(ps, i));
            ptr += sprintf(ptr, "  %c", c);
        } else {
            ptr += sprintf(ptr, "  .");
        }
    }

    ptr += sprintf(ptr, " |");

    ptr += sprintf(ptr, " (len=%u", length);

    // Metadata breakdown
    if (flags) {
        ptr += sprintf(ptr, ", flags=");
        if (flags & PACKED_FLAG_CASE_SENSITIVE)
            ptr += sprintf(ptr, "CASE");

        if (flags & PACKED_FLAG_CONTAINS_DIGIT) {
            if (flags & PACKED_FLAG_CASE_SENSITIVE) ptr += sprintf(ptr, "|");
            ptr += sprintf(ptr, "DIGIT");
        }
        if (flags & PACKED_FLAG_CONTAINS_SPECIAL) {
            if (flags & (PACKED_FLAG_CASE_SENSITIVE | PACKED_FLAG_CONTAINS_DIGIT))
                ptr += sprintf(ptr, "|");

            ptr += sprintf(ptr, "SPECIAL");
        }
    }

    ptr += sprintf(ptr, ")\n");

    // Third line: bit boundaries
    for (int i = 0; i < lineLength; i++) *ptr++ = '-';

    *ptr = '\0';
    return (i32)((size_t)ptr - (size_t)buffer);
}

i32 psd_inspect(const PackedString ps, char* buffer) {
    if (!buffer) return -1;

    if (!ps_valid(ps)) {
        const i32 len = snprintf(buffer, 32,
            "PackedString<INVALID>");
        return len;
    }

    char str_buf[PACKED_STRING_MAX_LEN + 1];
    ps_unpack(ps, str_buf);

    const u8 len = ps_length(ps);
    const u8 flags = ps_flags(ps);

    // Build flag string compactly
    char flag_chars[4] = "---";
    if (flags & PACKED_FLAG_CASE_SENSITIVE) flag_chars[0] = 'C';
    if (flags & PACKED_FLAG_CONTAINS_DIGIT) flag_chars[1] = 'D';
    if (flags & PACKED_FLAG_CONTAINS_SPECIAL) flag_chars[2] = 'S';

    const i32 length = snprintf(buffer, 64,
        "PackedString<\"%s\" len=%u flags=%s>",
        str_buf, len, flag_chars);

    return length;
}

i32 psd_cstr(const PackedString ps, char* buffer) {
    static const char
        *marker_invalid  = "[INVALID:invalid]",
        *marker_null     = "[INVALID:null]",
        *marker_empty    = "[INVALID:empty]",
        *marker_unpack   = "[INVALID:unpack]",
        *marker_unknown  = "[INVALID:unknown]";

    const u8 len = ps_length(ps);

    if (len > PACKED_STRING_MAX_LEN) {
        if (len == PSC_INVALID) {
            strcpy(buffer, marker_invalid);
            return -1;
        }

        if (len == PSC_NULL) {
            strcpy(buffer, marker_null);
            return -1;
        }

        if (len == PSC_EMPTY) {
            strcpy(buffer, marker_empty);
            return -1;
        }

        strcpy(buffer, marker_unknown);
        return -1;
    }

    // Try to unpack
    const i32 length = ps_unpack(ps, buffer);
    if (length != -1) return length;

    strcpy(buffer, marker_unpack);
    return -1;
}

static __thread char debug_buffer[1024];

char* psd_warper(const PsDebugFunc func, const PackedString ps) {
    func(ps, debug_buffer);
    return debug_buffer;
}