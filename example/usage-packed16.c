/**
 * @file usage-packed16.c
 * Usage examples for PackedString library
 */

#include "../packed16/packed-string.h"

#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

// Helper to print packed string
void print_ps(const char* label, PackedString ps) {
    char buffer[PACKED_STRING_MAX_LEN + 1];
    ps_unpack(ps, buffer);
    printf("%-30s: '%s' (len=%u, flags=0x%X)\n",
           label, buffer, ps_length(ps), ps_flags(ps));
}

void print_v(const char* label, char* s, ...) {
   char buffer[256];
   va_list args;
   va_start(args, s);

   vsnprintf(buffer, sizeof(buffer), s, args);

   va_end(args);

    printf("%-30s: %s\n", label, buffer);
}

int main() {
    printf("\n=== PackedString Usage Examples ===\n\n");

    // ========================================================================
    // Basic Creation and Unpacking
    // ========================================================================
    printf("--- Basic Creation ---\n");

    // Pack a string
    PackedString greeting = ps_pack("Hello_World");
    print_ps("ps_pack('Hello_World')", greeting);

    // Empty string
    PackedString empty = ps_empty();
    print_ps("ps_empty()", empty);

    // From raw values (advanced)
    PackedString from_raw = ps_from(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL);
    print_v("ps_from(raw)", "lo=%016llX, hi=%016llX", from_raw.lo, from_raw.hi);

    // Make with specific length/flags
    PackedString made = ps_make(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL, 5, 3);
    print_v("ps_make()", "len=%u, flags=%u", ps_length(made), ps_flags(made));

    printf("\n");

    // ========================================================================
    // Character Access
    // ========================================================================
    printf("--- Character Access ---\n");

    PackedString text = ps_pack("Hello123");
    print_ps("Original", text);

    // Get characters
    print_v("ps_first()","'%c'", ps_six(ps_first(text)));
    print_v("ps_last()","'%c'", ps_six(ps_last(text)));
    print_v("ps_at(1)","'%c'", ps_six(ps_at(text, 1)));
    print_v("ps_at(4)","'%c'", ps_six(ps_at(text, 4)));

    // Modify character
    PackedString modified = text;
    ps_set(&modified, 0, ps_char('J'));
    print_ps("After ps_set(0, 'J')", modified);
    print_ps("Original unchanged", text);

    printf("\n");

    // ========================================================================
    // Flag Operations
    // ========================================================================
    printf("--- Flag Operations ---\n");

    PackedString lower = ps_pack("hello");
    PackedString mixed = ps_pack("Hello123");
    PackedString special = ps_pack("hello_world");

    print_ps("lower", lower);
    print_ps("mixed", mixed);
    print_ps("special", special);

    print_v("ps_is_case_sensitive(lower)", "%s\n",
           ps_is_case_sensitive(lower) ? "true" : "false");
    print_v("ps_is_case_sensitive(mixed)", "%s\n",
           ps_is_case_sensitive(mixed) ? "true" : "false");
    print_v("ps_contains_digit(mixed)", "%s\n",
           ps_contains_digit(mixed) ? "true" : "false");
    print_v("ps_contains_special(special)", "%s\n",
           ps_contains_special(special) ? "true" : "false");

    printf("\n");

    // ========================================================================
    // String Operations
    // ========================================================================
    printf("--- String Operations ---\n");

    PackedString str = ps_pack("hello_world");
    print_ps("Original", str);

    // Substring
    PackedString sub = ps_substring(str, 6, 5);
    print_ps("ps_substring(6,5)", sub);

    // Skip prefix
    PackedString skipped = ps_skip(str, 6);
    print_ps("ps_skip(6)", skipped);

    // Truncate
    PackedString truncated = ps_trunc(str, 5);
    print_ps("ps_trunc(5)", truncated);

    // Check prefix/suffix
    PackedString prefix = ps_pack("hello");
    PackedString suffix = ps_pack("world");

    print_v("ps_starts_with('hello')", "%s\n",
           ps_starts_with(str, prefix) ? "true" : "false");
    print_v("ps_ends_with('world')", "%s\n",
           ps_ends_with(str, suffix) ? "true" : "false");

    printf("\n");

    // ========================================================================
    // Concatenation
    // ========================================================================
    printf("--- Concatenation ---\n");

    PackedString first = ps_pack("hello");
    PackedString second = ps_pack("_world");
    PackedString third = ps_pack("123");

    print_ps("first", first);
    print_ps("second", second);

    PackedString concat1 = ps_concat(first, second);
    print_ps("ps_concat(first, second)", concat1);

    PackedString concat2 = ps_concat(concat1, third);
    print_ps("ps_concat(..., third)", concat2);

    // Concatenation with truncation (max 20 chars)
    PackedString long1 = ps_pack("abcdefghij");
    PackedString long2 = ps_pack("klmnopqrst");
    PackedString too_long = ps_concat(long1, long2);
    print_ps("ps_concat(10+10 chars)", too_long);  // Truncated to 20

    printf("\n");

    // ========================================================================
    // Case Conversion
    // ========================================================================
    printf("--- Case Conversion ---\n");

    PackedString mixed_case = ps_pack("HelloWorld123");
    print_ps("Original", mixed_case);

    PackedString lower_case = ps_to_lower(mixed_case);
    print_ps("ps_to_lower()", lower_case);

    PackedString upper_case = ps_to_upper(mixed_case);
    print_ps("ps_to_upper()", upper_case);

    printf("\n");

    // ========================================================================
    // Padding
    // ========================================================================
    printf("--- Padding ---\n");

    PackedString pad_me = ps_pack("hello");
    print_ps("Original", pad_me);

    PackedString left_pad = ps_pad_left(pad_me, ps_char('_'), 10);
    print_ps("ps_pad_left('_', 10)", left_pad);

    PackedString right_pad = ps_pad_right(pad_me, ps_char('_'), 10);
    print_ps("ps_pad_right('_', 10)", right_pad);

    PackedString center_pad = ps_pad_center(pad_me, ps_char('_'), 11);
    print_ps("ps_pad_center('_', 11)", center_pad);

    printf("\n");

    // ========================================================================
    // Search Operations
    // ========================================================================
    printf("--- Search Operations ---\n");

    PackedString search_str = ps_pack("hello_world_hello");
    print_ps("Search string", search_str);

    // Find characters
    print_v("ps_find_six('h')", "%d\n", ps_find_six(search_str, ps_char('h')));
    print_v("ps_find_six('o')", "%d\n", ps_find_six(search_str, ps_char('o')));
    print_v("ps_find_six('x')", "%d\n", ps_find_six(search_str, ps_char('x')));
    print_v("ps_find_from_six('h', 1)", "%d\n", ps_find_from_six(search_str, ps_char('h'), 1));
    print_v("ps_find_last_six('h')", "%d\n", ps_find_last_six(search_str, ps_char('h')));

    // Check if contains
    print_v("ps_contains_six('_')", "%s",
           ps_contains_six(search_str, ps_char('_')) ? "true" : "false");

    // Find patterns
    PackedString pattern1 = ps_pack("world");
    PackedString pattern2 = ps_pack("xyz");

    print_v("ps_contains('world')", "%s",
           ps_contains(search_str, pattern1) ? "true" : "false");
    print_v("ps_contains('xyz')", "%s",
           ps_contains(search_str, pattern2) ? "true" : "false");

    printf("\n");

    // ========================================================================
    // Comparisons
    // ========================================================================
    printf("--- Comparisons ---\n");

    PackedString s1 = ps_pack("hello");
    PackedString s2 = ps_pack("hello");
    PackedString s3 = ps_pack("HELLO");
    PackedString s4 = ps_pack("world");

    print_ps("s1", s1);
    print_ps("s2", s2);
    print_ps("s3", s3);
    print_ps("s4", s4);

    print_v("ps_equal(s1, s2)", "%s", ps_equal(s1, s2) ? "true" : "false");
    print_v("ps_equal(s1, s3)", "%s", ps_equal(s1, s3) ? "true" : "false");
    print_v("ps_equal_nocase(s1, s3)", "%s", ps_equal_nocase(s1, s3) ? "true" : "false");
    print_v("ps_compare(s1, s4)", "%d", ps_compare(s1, s4));
    print_v("ps_packed_compare(s1, s2)", "%d", ps_packed_compare(s1, s2));

    printf("\n");

    // ========================================================================
    // Hashing
    // ========================================================================
    printf("--- Hashing ---\n");

    PackedString hash_me = ps_pack("hello");
    print_ps("String to hash", hash_me);

    print_v("ps_hash32()", "0x%08X", ps_hash32(hash_me));
    print_v("ps_hash64()", "0x%016llX", ps_hash64(hash_me));
    print_v("ps_table_hash()", "0x%08X", ps_table_hash(hash_me));

    // Same strings produce same hashes
    PackedString hash_me2 = ps_pack("hello");
    print_v("Same string hash32: %s",
           ps_hash32(hash_me) == ps_hash32(hash_me2) ? "equal ✓" : "different ✗");

    printf("\n");

    // ========================================================================
    // Lock/Unlock (Simple Encryption)
    // ========================================================================
    printf("--- Lock/Unlock ---\n");

    PackedString secret = ps_pack("my_secret_data");
    PackedString key = ps_pack("key123");

    print_ps("Original secret", secret);
    print_ps("Key", key);

    // Lock the string
    PackedString locked = ps_lock(secret, key);
    print_ps("Locked version", locked);

    // Try to unlock with wrong key
    PackedString wrong_key = ps_pack("wrong");
    PackedString still_locked = ps_unlock(locked, wrong_key);
    print_ps("Unlock with wrong key", still_locked);

    // Unlock with correct key
    PackedString unlocked = ps_unlock(locked, key);
    print_ps("Unlock with correct key", unlocked);

    printf("\n");

    // ========================================================================
    // Validation
    // ========================================================================
    printf("--- Validation ---\n");

    PackedString valid_id = ps_pack("variable_name");
    PackedString invalid_id = ps_pack("123variable");

    print_ps("Valid identifier", valid_id);
    print_ps("Invalid identifier", invalid_id);

    print_v("ps_is_valid_identifier(valid)", "%s",
           ps_is_valid_identifier(valid_id) ? "true" : "false");
    print_v("ps_is_valid_identifier(invalid)", "%s",
           ps_is_valid_identifier(invalid_id) ? "true" : "false");

    printf("\n");

    // ========================================================================
    // Compile-time Literals
    // ========================================================================
    printf("--- Compile-time Literals ---\n");

    // Create at compile time (no runtime packing cost)
    PackedString literal = PS_LITERAL("hello");
    print_ps("PS_LITERAL('hello')", literal);

    // Static assert for length (compile-time check)
    PS_STATIC_ASSERT_LEN("this is fine");
    // PS_STATIC_ASSERT_LEN("this string is too long for PackedString"); // Would fail to compile

    printf("\n");

    // ========================================================================
    // Error Handling
    // ========================================================================
    printf("--- Error Handling ---\n");

    // Invalid characters
    PackedString invalid = ps_pack("hello@world");
    printf("ps_pack('hello@world') valid: %s\n", ps_valid(invalid) ? "yes" : "no");
    if (!ps_valid(invalid)) {
        printf("  Error code: %u\n", ps_length(invalid));
    }

    // Too long string
    PackedString toolong = ps_pack("this_string_is_definitely_longer_than_20_chars");
    printf("ps_pack(>20 chars) valid: %s\n", ps_valid(toolong) ? "yes" : "no");

    // NULL input
    PackedString null_str = ps_pack(NULL);
    printf("ps_pack(NULL) valid: %s\n", ps_valid(null_str) ? "yes" : "no");

    printf("\n");

    // ========================================================================
    // Debug Functions
    // ========================================================================
    printf("--- Debug Functions ---\n");

    PackedString debug_me = ps_pack("Hello123");
    char buffer[1024];

    // Hex dump
    psd_hex(debug_me, buffer);
    printf("[ psd_hex() ]\n%s\n\n", buffer);

    // Binary representation
    psd_binary(debug_me, buffer);
    printf("[ psd_binary() ]\n%s\n\n", buffer);

    // Encoding binary (shows 6-bit encoding)
    psd_encoding_binary(debug_me, buffer);
    printf("[ psd_encoding_binary() ]\n%s\n\n", buffer);

    // Quick inspect
    psd_inspect(debug_me, buffer);
    printf("[ psd_inspect() ]\n%s\n\n", buffer);

    // Full info
    psd_info(debug_me, buffer);
    printf("[ psd_info() ]\n%s\n\n", buffer);

    // Visualize bits
    psd_visualize_bits(debug_me, buffer);
    printf("[ psd_visualize_bits() ]\n%s\n\n", buffer);

    // Warper for easy printing
    printf("[ psd_warper(pd_info) ]\n%s\n\n", psd_warper(psd_info, debug_me));

    printf("\n");

    // ========================================================================
    // Practical Use Cases
    // ========================================================================
    printf("--- Practical Use Cases ---\n");

    // 1. Storing identifiers in a compiler
    printf("1. Compiler symbol table:\n");
    PackedString identifier = PS_LITERAL("user_count");
    printf("   Symbol: '%s' (packed in 16 bytes)\n",
           psd_warper(psd_cstr, identifier));

    // 2. Configuration keys
    printf("\n2. Configuration keys:\n");
    PackedString config_key = ps_pack("max_connections");
    printf("   Config key: '%s'\n", psd_warper(psd_cstr, config_key));

    // 3. Fast string lookup in hash table
    printf("\n3. Hash table lookup:\n");
    PackedString lookup = ps_pack("search_key");
    printf("   Hash value: 0x%08X (for table lookup)\n", ps_table_hash(lookup));

    // 4. Storing enum names
    printf("\n4. Enum names:\n");
    typedef enum {
        STATUS_OK,
        STATUS_ERROR,
        STATUS_PENDING
    } Status;

    PackedString status_names[] = {
        PS_LITERAL("OK"),
        PS_LITERAL("ERROR"),
        PS_LITERAL("PENDING")
    };

    for (int i = 0; i < 3; i++) {
        printf("   %d: %s\n", i, psd_warper(psd_cstr, status_names[i]));
    }

    // 5. Simple encryption with lock/unlock
    printf("\n5. Simple string key encryption:\n");
    PackedString password = ps_pack("secret_key");
    PackedString encrypt_key = ps_pack("secret");
    PackedString encrypted = ps_lock(password, encrypt_key);
    printf("   Original : %s\n", psd_warper(psd_cstr, password));
    printf("   Encrypted: %s\n", psd_warper(psd_cstr, encrypted));
    printf("   Decrypted: %s\n",
           psd_warper(psd_cstr, ps_unlock(encrypted, encrypt_key)));

    // 6. String pool/deduplication
    printf("\n6. String deduplication:\n");
    PackedString pool[3];
    pool[0] = ps_pack("hello");
    pool[1] = ps_pack("hello");  // Same content
    pool[2] = ps_pack("world");

    printf("   pool[0] == pool[1]: %s (fast 128-bit compare)\n",
           ps_equal(pool[0], pool[1]) ? "yes" : "no");
    printf("   pool[0] == pool[2]: %s\n",
           ps_equal(pool[0], pool[2]) ? "yes" : "no");

    printf("\n");

    return 0;
}