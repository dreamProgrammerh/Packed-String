/**
 * @file packed-string.h
 * PackedString - pretty fast compact string with 20 chars stored in 128-bit
 *
 * API prefix: 'ps_'
 * API debug prefix: 'psd_'
 * 
 * Type name: PackedString
 *  - Shorthands: packed, ps_t
 *
 * Functions and time complexity:
 * | ID   | Function                  | Complexity | 100x call time |
 * |------|---------------------------|------------|----------------|
 * | 1    | char                      | O(1)       | ?              |
 * | 2    | six                       | O(1)       | ?              |
 * | 3    | alphabet                  | O(1)       | ?              |
 * | 4    | length                    | O(1)       | ?              |
 * | 5    | flags                     | O(1)       | ?              |
 * | 6    | valid                     | O(1)       | ?              |
 * | 7    | is_empty                  | O(1)       | ?              |
 * | 8    | empty                     | O(1)       | ?              |
 * | 9    | form                      | O(1)       | ?              |
 * | 10   | make                      | O(1)       | ?              |
 * | 11   | pack                      | O(N)       | ?              |
 * | 12   | unpack                    | O(N)       | ?              |
 * | 13   | pack_ex                   | O(N)       | ?              |
 * | 14   | unpack_ex                 | O(N)       | ?              |
 * | 15   | scan                      | O(N)       | ?              |
 * | 16   | is_case_sensitive         | O(1)       | ?              |
 * | 17   | contains_digit            | O(1)       | ?              |
 * | 18   | contains_special          | O(1)       | ?              |
 * | 19   | set                       | O(1)       | ?              |
 * | 20   | at                        | O(1)       | ?              |
 * | 21   | first                     | O(1)       | ?              |
 * | 22   | last                      | O(1)       | ?              |
 * | 23   | equal                     | O(1)       | ?              |
 * | 24   | equal_nometa              | O(1)       | ?              |
 * | 25   | equal_nocase              | O(N)       | ?              |
 * | 26   | packed_compare            | O(1)       | ?              |
 * | 27   | compare                   | O(1)       | ?              |
 * | 28   | starts_with               | O(1)       | ?              |
 * | 29   | ends_with                 | O(1)       | ?              |
 * | 30   | starts_with_at            | O(1)       | ?              |
 * | 31   | ends_with_at              | O(1)       | ?              |
 * | 32   | skip                      | O(1)       | ?              |
 * | 33   | trunc                     | O(1)       | ?              |
 * | 34   | substring                 | O(1)       | ?              |
 * | 35   | concat                    | O(1)       | ?              |
 * | 36   | to_lower                  | O(N)       | ?              |
 * | 37   | to_upper                  | O(N)       | ?              |
 * | 38   | pad_left                  | O(N)       | ?              |
 * | 39   | pad_right                 | O(N)       | ?              |
 * | 40   | pad_center                | O(N)       | ?              |
 * | 41   | find_six                  | O(1)       | ?              |
 * | 42   | find_from_six             | O(1)       | ?              |
 * | 43   | find_last_six             | O(1)       | ?              |
 * | 44   | contains_six              | O(1)       | ?              |
 * | 45   | contains                  | O(N)       | ?              |
 * | 46   | hash32                    | O(1)       | ?              |
 * | 47   | hash64                    | O(1)       | ?              |
 * | 48   | table_hash                | O(1)       | ?              |
 * | 49   | lock                      | O(1)       | ?              |
 * | 50   | unlock                    | O(1)       | ?              |
 * | 51   | hex                       | O(1)       | ?              |
 * | 52   | binary                    | O(?)       | ?              |
 * | 53   | encoding_binary           | O(?)       | ?              |
 * | 54   | info                      | O(?)       | ?              |
 * | 55   | visualize_bits            | O(?)       | ?              |
 * | 56   | psd_inspect               | O(?)       | ?              |
 * | 57   | psd_cstr                  | O(?)       | ?              |
 * | 58   | psd_warper                | O(?)       | ?              |
 * 
 */

#ifndef PACKED_STRING_H
#define PACKED_STRING_H

#include "encoding.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// PUBLIC API - PackedString Type and Constants
// ============================================================================

/**
 * PackedString - 128-bit compact string storage
 * 
 * Stores up to 20 characters using 6-bit encoding (64-character alphabet).
 * Layout: [120 bits character data][3 bits flags][5 bits length]
 *
 * Characters 0-9: in lo[0:59] (bits 0 to 59)
 * Character 10: in lo[60:63] and hi[0:1] (4 bits from lo, 2 bits from hi)
 * Characters 11-19: in hi[2:55] (bits 2 to 56 of hi)
 *
 * Metadata in hi[56:63]
 *  * Flags hi[56:58]
 *  * Length hi[59:63]
 *
 * Character set: 0-9 a-z A-Z _ $
 * Flags: case_sensitive | starts_with_digit | contains_special
 */
typedef struct packed_string {
    u64 lo; // Lower 64 bits: chars 0-9 (60 bits) + 4 bits
    u64 hi; // Upper 64 bits: chars 11-19 (60 bits) + 8 bits metadata
} PackedString;

typedef struct packed_string packed;    // Shorthand for PackedString
typedef struct packed_string ps_t;      // Shorthand for PackedString

typedef i32 (*PsDebugFunc)(PackedString ps, char* buffer);

// Error cases
#define PSC_INVALID  31
#define PSC_NULL     30
#define PSC_EMPTY    29
// You can define your own error state from 28-21 are free to use

// Constants
#define PACKED_STRING_MAX_LEN   20
#define PACKED_STRING_ALPHABET  "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_$"
#define PACKED_STRING_INVALID   ((PackedString){.lo = 0, .hi = (u64)PSC_INVALID << 59})
#define PACKED_STRING_NULL      ((PackedString){.lo = 0, .hi = (u64)PSC_NULL    << 59})
#define PACKED_STRING_EMPTY     ((PackedString){.lo = 0, .hi = (u64)PSC_EMPTY   << 59})

// Flag bit positions (in metadata byte)
#define PACKED_FLAG_CASE_SENSITIVE      (1u << 0)   // Bit 0: 1=preserve case, 0=lowercase
#define PACKED_FLAG_CONTAINS_DIGIT      (1u << 1)   // Bit 1: 1=contains 0-9
#define PACKED_FLAG_CONTAINS_SPECIAL    (1u << 2)   // Bit 2: 1=contains '_' or '$'

// ============================================================================
// STATIC
// ============================================================================

/**
 * Convert ASCII character to sixbit character.
 *
 * @param c Character to be converted
 * @return Sixbit Character or UINT8_MAX if not valid
 */
static inline u8 ps_char(const char c)  {
    if (c == '0') return 0;
    const u8 r = PS_CHAR_TO_SIXBIT[(u8)c];
    if (r == 0) return UINT8_MAX;
    return r;
}

/**
 * Convert sixbit character to ASCII character .
 *
 * @param six Sixbit character to be converted
 * @return ASCII Character or '?' if not valid
 */
static inline char ps_six(const u8 six) {
    if (six >= 64) return '?';
    return PS_SIXBIT_TO_CHAR[six];
}

/** Check if char in packed string alphabet */
static inline bool ps_alphabet(const char c) {
    if (c == '0') return true;
    return PS_CHAR_TO_SIXBIT[(u8)c] != 0;
}

// ============================================================================
// CORE OPERATIONS
// ============================================================================

/**
 * Get length of packed string (0-20).
 *
 * @param ps Packed string
 * @return String length
 */
static inline u8 ps_length(const PackedString ps) {
    return ps.hi >> 59;
}

/**
 * Get flags of packed string.
 *
 * @param ps Packed string
 * @return Flags byte (bits 5-7 are valid)
 */
static inline u8 ps_flags(const PackedString ps) {
    return (ps.hi >> 56) & 0x7;
}

/**
 * Check if packed string is valid.
 *
 * @param ps Packed string
 * @return true if valid
 */
static inline bool ps_valid(const PackedString ps) {
    return (ps.hi >> 59) <= PACKED_STRING_MAX_LEN;
}

/**
 * Check if packed string is empty.
 *
 * @param ps Packed string
 * @return true if packed string is empty
 */
static inline bool ps_is_empty(const PackedString ps) {
    return ps_length(ps) == 0;
}

/**
 * Make an empty packed string
 *
 * @return Empty packed string
 */
static inline PackedString ps_empty(void) {
    return (PackedString){.lo = 0, .hi = 0};
}

/**
 * Short Hand for (PackedString){.lo = lo, .hi = hi}.
 *
 * @param lo Lower 64 bits
 * @param hi Upper 64 bits
 * @return Packed string
 */
static inline PackedString ps_from(const u64 lo, const u64 hi) {
    return (PackedString){.lo = lo, .hi = hi};
}

/**
 * Make a packed string with given values.
 *
 * @param lo Lower 64 bits
 * @param hi Upper 64 bits
 * @param length Length of string
 * @param flags Flags of string
 * @return Packed string
 */
static inline PackedString ps_make(const u64 lo, const u64 hi,
    u8 const length, u8 const flags) {
    const u8 meta = length << 3 | flags;
    const u64 h = hi & 0x00FFFFFFFFFFFFFFULL | (u64)meta << 56;
    return (PackedString){.lo = lo, .hi = h};
}

/**
 * Scan and fix flags of packed string.
 *
 * @param ps Packed string to scan
 * @return Scanned and fixed packed string
 */
PackedString ps_scan(PackedString ps);

/**
 * Pack a C string into PackedString (max 20 chars).
 * Smart flags detection.
 * 
 * @param str Null-terminated C string
 * @return Packed string (invalid if str is NULL, have invalid char or length > 20)
 */
PackedString ps_pack(const char* str);

/**
 * Pack a string with exact flags (advanced use).
 * 
 * @param str String to pack
 * @param length String length (must be ≤ 20)
 * @param flags Combination of PACKED_FLAG_* constants
 * @return Packed string (invalid if str is NULL. have invalid char or length > 20)
 */
PackedString ps_pack_ex(const char* str, u8 length, u8 flags);

/**
 * Unpack to pre-allocated buffer.
 * 
 * @param ps Packed string to unpack
 * @param buffer Output buffer (must have at least PACKED_STRING_MAX_LEN+1 bytes)
 * @return -1 if (ps is invalid) else length
 */
i32 ps_unpack(PackedString ps, char* buffer);

/**
 * Unpack exact length with specified flags (advanced use).
 *
 * @param ps Packed string to unpack
 * @param buffer Output buffer (must have at least PACKED_STRING_MAX_LEN+1 bytes)
 * @param length String length (must be ≤ 20)
 * @param flags Combination of PACKED_FLAG_* constants
 * @return -1 if (ps is invalid or length > 20) else length
 */
i32 ps_unpack_ex(PackedString ps, char* buffer, u8 length, u8 flags);

// ============================================================================
// FLAGS CHECK O(1)
// ============================================================================

/**
 * Check if string is case-sensitive (using flag, O(1)).
 *
 * @param ps Packed string
 * @return true if case should be preserved
 */
static inline bool ps_is_case_sensitive(const PackedString ps) {
    return (ps_flags(ps) & PACKED_FLAG_CASE_SENSITIVE) != 0;
}

/**
 * Check if string contains digit (using flag, O(1)).
 *
 * @param ps Packed string
 * @return true if contains character is 0-9
 */
static inline bool ps_contains_digit(const PackedString ps) {
    return (ps_flags(ps) & PACKED_FLAG_CONTAINS_DIGIT) != 0;
}

/**
 * Check if string contains special chars (using flag, O(1)).
 *
 * @param ps Packed string
 * @return true if contains '_' or '$'
 */
static inline bool ps_contains_special(const PackedString ps) {
    return (ps_flags(ps) & PACKED_FLAG_CONTAINS_SPECIAL) != 0;
}

// ============================================================================
// CHARACTER ACCESS
// ============================================================================

/**
 * Set sixbit at position (0-based).
 *
 * @param ps Packed string
 * @param index Sixbit position (0-19)
 * @param sixbit New sixbit
 * @return Sixbit or UINT8_MAX if out of bound
 */
u8 ps_set(PackedString* ps, u8 index, u8 sixbit);

/**
 * Get sixbit at position (0-based).
 * Returns UINT8_MAX if index out of bounds.
 *
 * @param ps Packed string
 * @param index Sixbit position (0-19)
 * @return Sixbit or UINT8_MAX if out of bound
 */
u8 ps_at(PackedString ps, u8 index);

/**
 * Get first sixbit character.
 * Faster than ps_at(ps, 0) for first char.
 * 
 * @param ps Packed string
 * @return First sixbit character or UINT8_MAX if empty
 */
u8 ps_first(PackedString ps);

/**
 * Get last sixbit character.
 * Same as ps_at(ps, len-1) for last char.
 *
 * @param ps Packed string
 * @return Last sixbit character or UINT8_MAX if empty
 */
u8 ps_last(PackedString ps);

// ============================================================================
// COMPARISON OPERATIONS
// ============================================================================

/**
 * Exact equality comparison (case-sensitive).
 * This is the FAST PATH - two 64-bit integer compares.
 * 
 * @param a First packed string
 * @param b Second packed string
 * @return true if exactly equal
 */
static inline bool ps_equal(const PackedString a, const PackedString b) {
    return a.lo == b.lo && a.hi == b.hi;
}

/**
 * Equality comparison ignoring metadata (case-sensitive).
 *
 * @param a First packed string
 * @param b Second packed string
 * @return true if equal ignoring metadata
 */
static inline bool ps_equal_nometa(const PackedString a, const PackedString b) {
    const u64 mask = 0x00FFFFFFFFFFFFFFULL;
    return a.lo == b.lo && (a.hi & mask) == (b.hi & mask);
}

/**
 * Case-insensitive equality.
 * Fast if both strings have CASE_SENSITIVE=0.
 * 
 * @param a First packed string
 * @param b Second packed string
 * @return true if equal ignoring case
 */
bool ps_equal_nocase(PackedString a, PackedString b);

/**
 * Packed character 120bit comparison.
 * 
 * @param a First packed string
 * @param b Second packed string
 * @return
 *      <0 if a < b,
 *      =0 if equal,
 *      >0 if a > b
 */
static inline i32 ps_packed_compare(const PackedString a, const PackedString b) {
    // Mask metadata bits
    const u64 mask = 0x00FFFFFFFFFFFFFFULL;
    const u64 hiA = a.hi & mask;
    const u64 hiB = b.hi & mask;

    if (hiA < hiB) return -1;
    if (hiA > hiB) return 1;

    // hi parts equal
    if (a.lo < b.lo) return -1;
    if (a.lo > b.lo) return 1;

    return 0;
}

/**
 * Lexicographic comparison.
 *
 * @param a First packed string
 * @param b Second packed string
 * @return
 *      <0 if a < b,
 *      =0 if equal,
 *      >0 if a > b
 */
i32 ps_compare(PackedString a, PackedString b);

// ============================================================================
// STRING OPERATIONS
// ============================================================================

/**
 * Check if packed string starts with prefix.
 * 
 * @param ps Packed string
 * @param prefix Prefix to check
 * @return true if ps starts with prefix
 */
bool ps_starts_with(PackedString ps, PackedString prefix);

/**
 * Check if packed string ends with suffix.
 * 
 * @param ps Packed string
 * @param suffix Suffix to check
 * @return true if ps ends with suffix
 */
bool ps_ends_with(PackedString ps, PackedString suffix);

/**
 * Check if packed string starts with prefix at start.
 *
 * @param ps Packed string
 * @param prefix Prefix to check
 * @param start Check after start position
 * @return true if ps starts with prefix
 */
bool ps_starts_with_at(PackedString ps, PackedString prefix, u8 start);

/**
 * Check if packed string ends with suffix from end.
 *
 * @param ps Packed string
 * @param suffix Suffix to check
 * @param end Check before end position
 * @return true if ps ends with suffix
 */
bool ps_ends_with_at(PackedString ps, PackedString suffix, u8 end);

/**
 * Move start of string to specified start.
 *
 * @param ps Packed string
 * @param start Starting char
 * @return Packed string starting at start char
 */
PackedString ps_skip(PackedString ps, u8 start);

/**
 * Truncated a string to length.
 *
 * @param ps Packed string
 * @param length Desired length
 * @return Truncated packed string to length
 */
PackedString ps_trunc(PackedString ps, u8 length);

/**
 * Get substring as new packed string.
 * Returns empty string if start ≥ length.
 * 
 * @param ps Packed string
 * @param start Starting position (0-based)
 * @param length Number of characters
 * @return Substring (maybe truncated)
 */
PackedString ps_substring(PackedString ps, u8 start, u8 length);

/**
 * Concatenate two packed strings.
 * Returns truncated result if total length > 20.
 * 
 * @param a First string
 * @param b Second string
 * @return Concatenated string
 */
PackedString ps_concat(PackedString a, PackedString b);

/**
 * Convert to lowercase (returns new packed string).
 * CASE_SENSITIVE flag will be cleared.
 * 
 * @param ps Packed string to convert
 * @return Lowercase version
 */
PackedString ps_to_lower(PackedString ps);

/**
 * Convert to uppercase (returns new packed string).
 * CASE_SENSITIVE flag will be set.
 * 
 * @param ps Packed string to convert
 * @return Uppercase version
 */
PackedString ps_to_upper(PackedString ps);

/**
 * Pad a string to left until length with sixbit.
 *
 * @param ps Packed string
 * @param sixbit Sixbit char
 * @param length Max length
 * @return Left padded packed string
 */
PackedString ps_pad_left(PackedString ps, u8 sixbit, u8 length);

/**
 * Pad a string to right until length with sixbit.
 *
 * @param ps Packed string
 * @param sixbit Sixbit char
 * @param length Max length
 * @return Right padded packed string
 */
PackedString ps_pad_right(PackedString ps, u8 sixbit, u8 length);

/**
 * Pad a string to center until length with sixbit.
 *
 * @param ps Packed string
 * @param sixbit Sixbit char
 * @param length Max length
 * @return Center padded packed string
 */
PackedString ps_pad_center(PackedString ps, u8 sixbit, u8 length);

// ============================================================================
// SEARCH OPERATIONS
// ============================================================================

/**
 * Find sixbit character in packed string.
 * 
 * @param ps Packed string
 * @param sixbit Sixbit character to find
 * @return Index of first occurrence, or -1 if not found
 */
i8 ps_find_six(PackedString ps, u8 sixbit);

/**
 * Find sixbit character starting from position.
 * 
 * @param ps Packed string
 * @param sixbit Sixbit character to find
 * @param start Starting position
 * @return Index of occurrence, or -1 if not found
 */
i8 ps_find_from_six(PackedString ps, u8 sixbit, u8 start);

/**
 * Find sixbit last occurrence of character.
 * 
 * @param ps Packed string
 * @param sixbit Sixbit character to find
 * @return Index of last occurrence, or -1 if not found
 */
i8 ps_find_last_six(PackedString ps, u8 sixbit);

/**
 * Check if string contains sixbit character.
 * 
 * @param ps Packed string
 * @param sixbit Sixbit character to check
 * @return true if character found
 */
bool ps_contains_six(PackedString ps, u8 sixbit);

/**
 * Check if string contains pattern.
 *
 * @param ps Packed string
 * @param pat Pattern to check
 * @return true if pattern validated
 */
bool ps_contains(PackedString ps, PackedString pat);

// ============================================================================
// HASHING & LOCKING
// ============================================================================

/**
 * 32-bit hash of packed string (MurmurHash3).
 * 
 * @param ps Packed string
 * @return 32-bit hash value
 */
u32 ps_hash32(PackedString ps);

/**
 * 64-bit hash of packed string (MurmurHash3).
 * 
 * @param ps Packed string
 * @return 64-bit hash value
 */
u64 ps_hash64(PackedString ps);

/**
 * Hash suitable for hash tables (combines 32-bit hash with length).
 * 
 * @param ps Packed string
 * @return Combined hash
 */
static inline u32 ps_table_hash(const PackedString ps) {
    const u32 h = ps_hash32(ps);
    return h ^ ps_length(ps) << 24;
}

/**
 * Lock packed string with key to unreadable version.
 *
 * @param ps Packed string
 * @param key Lock Key
 * @return Locked packed string
 */
PackedString ps_lock(PackedString ps, PackedString key);

/**
 * Unlock locked packed string with key to readable version again.
 *
 * @param ps Packed string
 * @param key Lock Key
 * @return Normal unlocked packed string if key is correct
 */
PackedString ps_unlock(PackedString ps, PackedString key);

// ============================================================================
// VALIDATION & UTILITIES
// ============================================================================

/**
 * Check if packed string could be a valid identifier.
 * Uses flags for fast path.
 * 
 * @param ps Packed string
 * @return true if valid identifier
 */
bool ps_is_valid_identifier(PackedString ps);

// ============================================================================
// DEBUGGING & FORMATTING
// ============================================================================

/**
 * Format as hex string (32 chars + null).
 * 
 * @param ps Packed string
 * @param buffer Output buffer (33 bytes min)
 * @return -1 if (buffer is invalid) else (output length + null)
 */
i32 psd_hex(PackedString ps, char* buffer);

/**
 * Format as binary string (129 chars + null).
 *
 * @param ps Packed string
 * @param buffer Output buffer (30 bytes min)
 * @return -1 if (buffer is invalid) else (output length + null)
 */
i32 psd_binary(PackedString ps, char* buffer);

/**
 * Format encoding char by char as binary string.
 *
 * @param ps Packed string
 * @param buffer Output buffer
 * @return -1 if (buffer is invalid) else (output length + null)
 */
i32 psd_encoding_binary(PackedString ps, char* buffer);

/**
 * Format packed string info for debugging.
 * 
 * @param ps Packed string
 * @param buffer Output buffer
 * @return Pointer to buffer
 */
i32 psd_info(PackedString ps, char* buffer);

/**
 * Visualize encoded bits as string.
 *
 * @param ps Packed string
 * @param buffer Output buffer
 * @return -1 if (buffer is invalid) else (output length + null)
*/
i32 psd_visualize_bits(PackedString ps, char* buffer);

/**
 * Format packed string data as string aka (toString).
 *
 * @param ps Packed string
 * @param buffer Output buffer
 * @return -1 if (buffer is invalid) else (output length + null)
*/
i32 psd_inspect(PackedString ps, char* buffer);

/**
 * Get c string representation.
 *
 * @param ps Packed string
 * @param buffer Output buffer
 * @return C string length including null
 */
i32 psd_cstr(PackedString ps, char* buffer);

/**
 * Warp debug functions with a temporary buffer.
 * Warning: this use a thread local buffer do not use it other than printing.
 *
 * @param func Debug function to call
 * @param ps Packed string
 * @return C string length including null
 */
char* psd_warper(PsDebugFunc func, PackedString ps);

// ============================================================================
// COMPILE-TIME HELPERS
// ============================================================================

/**
 * Compile-time string packing for constants.
 * Example: PackedString str = PS_LITERAL("hello");
 */
#define PS_LITERAL(str) (ps_pack(str))

/**
 * Compile-time length check.
 */
#define PS_STATIC_ASSERT_LEN(str) \
    static_assert(sizeof(str) - 1 <= PACKED_STRING_MAX_LEN, \
                  "String too long for PackedString")

#ifdef __cplusplus
}
#endif

#endif // PACKED_STRING_H