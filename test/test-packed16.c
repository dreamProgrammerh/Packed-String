/**
 * @file test-packed16.c
 * Test suite for PackedString library
 */
#include "../packed16/packed-string.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define TEST(cond, msg) do \
    { \
        if (!(cond)) { \
            printf("❌ FAIL: %s\n", msg); \
            failures++; \
        } else { \
            printf("✅ OK: %s\n", msg); \
        } \
    } while(0)

#define TEST_EQ(a, b, msg) TEST((a) == (b), msg)
#define TEST_NOT_EQ(a, b, msg) TEST((a) != (b), msg)
#define TEST_PS_EQ(a, b, msg) TEST(ps_equal_nometa((a), (b)), msg)
#define TEST_STR_EQ(a, b, msg) TEST(strcmp((a), (b)) == 0, msg)

// Helper to print test section
static void section(const char* name) {
    printf( "\n═══════════════════════════════════════════════════\n"
            "  %s"
            "\n═══════════════════════════════════════════════════\n", name);
}

// ============================================================================
// STATIC FUNCTIONS TESTS
// ============================================================================

int test_static_functions() {
    section("Static Functions");
    int failures = 0;

    // Test ps_char
    TEST_EQ(ps_char('0'), 0, "ps_char('0') = 0");
    TEST_EQ(ps_char('9'), 9, "ps_char('9') = 9");
    TEST_EQ(ps_char('a'), 10, "ps_char('a') = 10");
    TEST_EQ(ps_char('z'), 35, "ps_char('z') = 35");
    TEST_EQ(ps_char('A'), 36, "ps_char('A') = 36");
    TEST_EQ(ps_char('Z'), 61, "ps_char('Z') = 61");
    TEST_EQ(ps_char('_'), 62, "ps_char('_') = 62");
    TEST_EQ(ps_char('$'), 63, "ps_char('$') = 63");
    TEST_EQ(ps_char('?'), UINT8_MAX, "ps_char('?') = UINT8_MAX");

    // Test ps_six
    TEST_EQ(ps_six(0), '0', "ps_six(0) = '0'");
    TEST_EQ(ps_six(9), '9', "ps_six(9) = '9'");
    TEST_EQ(ps_six(10), 'a', "ps_six(10) = 'a'");
    TEST_EQ(ps_six(35), 'z', "ps_six(35) = 'z'");
    TEST_EQ(ps_six(36), 'A', "ps_six(36) = 'A'");
    TEST_EQ(ps_six(61), 'Z', "ps_six(61) = 'Z'");
    TEST_EQ(ps_six(62), '_', "ps_six(62) = '_'");
    TEST_EQ(ps_six(63), '$', "ps_six(63) = '$'");
    TEST_EQ(ps_six(64), '?', "ps_six(64) = '?'");

    // Test ps_alphabet
    TEST(ps_alphabet('0'), "ps_alphabet('0') = true");
    TEST(ps_alphabet('9'), "ps_alphabet('9') = true");
    TEST(ps_alphabet('a'), "ps_alphabet('a') = true");
    TEST(ps_alphabet('z'), "ps_alphabet('z') = true");
    TEST(ps_alphabet('A'), "ps_alphabet('A') = true");
    TEST(ps_alphabet('Z'), "ps_alphabet('Z') = true");
    TEST(ps_alphabet('_'), "ps_alphabet('_') = true");
    TEST(ps_alphabet('$'), "ps_alphabet('$') = true");
    TEST(!ps_alphabet('?'), "ps_alphabet('?') = false");
    TEST(!ps_alphabet('@'), "ps_alphabet('@') = false");

    return failures;
}

// ============================================================================
// CORE OPERATIONS TESTS
// ============================================================================

int test_core_operations() {
    section("Core Operations");
    int failures = 0;

    // Test ps_empty
    PackedString empty = ps_empty();
    TEST_EQ(ps_length(empty), 0, "ps_length(ps_empty()) = 0");
    TEST_EQ(ps_flags(empty), 0, "ps_flags(ps_empty()) = 0");
    TEST(ps_valid(empty), "ps_valid(ps_empty()) = true");

    // Test ps_from and ps_make
    PackedString ps1 = ps_from(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL);
    PackedString ps2 = ps_make(0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL, 5, 3);

    TEST_EQ(ps1.lo, 0x123456789ABCDEF0ULL, "ps_from().lo = 0x123456789ABCDEF0");
    TEST_EQ(ps1.hi, 0xFEDCBA9876543210ULL, "ps_from().hi = 0xFEDCBA9876543210");

    TEST_EQ(ps_length(ps2), 5, "ps_make(..., 5, 3) length = 5");
    TEST_EQ(ps_flags(ps2), 3, "ps_make(..., 5, 3) flags = 3");

    // Test ps_pack and ps_unpack
    const char* test_str = "hello123";
    PackedString packed = ps_pack(test_str);
    char buffer[PACKED_STRING_MAX_LEN + 1];

    TEST(ps_valid(packed), "ps_valid(ps_pack('hello123')) = true");
    TEST_EQ(ps_length(packed), 8, "ps_length(ps_pack('hello123')) = 8");
    TEST_EQ(ps_unpack(packed, buffer), 8, "ps_unpack() = 8");
    TEST_STR_EQ(buffer, "hello123", "ps_unpack() = 'hello123'");

    // Test invalid inputs
    PackedString invalid = ps_pack(NULL);
    TEST(!ps_valid(invalid), "ps_valid(ps_pack(NULL)) = false");
    TEST_EQ(ps_length(invalid), PSC_INVALID, "ps_length(ps_pack(NULL)) = PSC_INVALID");

    // Test too long string
    const char* too_long = "thisstringisdefinitelylongerthantwentycharacters";
    PackedString too_long_packed = ps_pack(too_long);
    TEST(!ps_valid(too_long_packed), "ps_valid(ps_pack(>20 chars)) = false");

    return failures;
}

// ============================================================================
// FLAGS TESTS
// ============================================================================

int test_flags() {
    section("Flags Operations");
    int failures = 0;

    // Test flag detection
    PackedString ps1 = ps_pack("hello");
    PackedString ps2 = ps_pack("Hello");
    PackedString ps3 = ps_pack("123");
    PackedString ps4 = ps_pack("hello_123");
    PackedString ps5 = ps_pack("$pecial");

    TEST(!ps_is_case_sensitive(ps1), "ps_is_case_sensitive('hello') = false");
    TEST(ps_is_case_sensitive(ps2), "ps_is_case_sensitive('Hello') = true");
    TEST(!ps_contains_digit(ps1), "ps_contains_digit('hello') = false");
    TEST(ps_contains_digit(ps3), "ps_contains_digit('123') = true");
    TEST(!ps_contains_special(ps1), "ps_contains_special('hello') = false");
    TEST(ps_contains_special(ps4), "ps_contains_special('hello_123') = true");
    TEST(ps_contains_special(ps5), "ps_contains_special('$pecial') = true");

    // Test ps_scan
    PackedString ps6 = ps_make(ps1.lo, ps1.hi, 5, 0);
    PackedString scanned = ps_scan(ps6);
    TEST_EQ(ps_flags(scanned), 0, "ps_scan('hello') flags = 0");

    return failures;
}

// ============================================================================
// CHARACTER ACCESS TESTS
// ============================================================================

int test_character_access() {
    section("Character Access");
    int failures = 0;

    PackedString ps = ps_pack("hello_world123");
    char buffer[PACKED_STRING_MAX_LEN + 1];
    ps_unpack(ps, buffer);

    TEST_EQ(ps_at(ps, 0), ps_char('h'), "ps_at(0) = 'h'");
    TEST_EQ(ps_at(ps, 4), ps_char('o'), "ps_at(4) = 'o'");
    TEST_EQ(ps_at(ps, 5), ps_char('_'), "ps_at(5) = '_'");
    TEST_EQ(ps_at(ps, 13), ps_char('3'), "ps_at(14) = '3'");
    TEST_EQ(ps_at(ps, 15), UINT8_MAX, "ps_at(15) = UINT8_MAX");

    TEST_EQ(ps_first(ps), ps_char('h'), "ps_first() = 'h'");
    TEST_EQ(ps_last(ps), ps_char('3'), "ps_last() = '3'");

    PackedString ps_copy = ps;
    TEST_EQ(ps_set(&ps_copy, 0, ps_char('H')), ps_char('H'), "ps_set(0, 'H') = 'H'");
    TEST_EQ(ps_first(ps_copy), ps_char('H'), "after ps_set, first = 'H'");
    TEST_EQ(ps_first(ps), ps_char('h'), "original unchanged, first = 'h'");
    TEST_EQ(ps_set(&ps_copy, 20, 0), UINT8_MAX, "ps_set(20) = UINT8_MAX");

    return failures;
}

// ============================================================================
// COMPARISON TESTS
// ============================================================================

int test_comparisons() {
    section("Comparison Operations");
    int failures = 0;

    PackedString ps1 = ps_pack("hello");
    PackedString ps2 = ps_pack("hello");
    PackedString ps3 = ps_pack("HELLO");
    PackedString ps4 = ps_pack("world");
    PackedString ps5 = ps_pack("hell");

    TEST(ps_equal(ps1, ps2), "ps_equal('hello', 'hello') = true");
    TEST(!ps_equal(ps1, ps3), "ps_equal('hello', 'HELLO') = false");
    TEST(!ps_equal(ps1, ps4), "ps_equal('hello', 'world') = false");

    TEST(ps_equal_nometa(ps1, ps2), "ps_equal_nometa('hello', 'hello') = true");
    TEST(!ps_equal_nometa(ps1, ps5), "ps_equal_nometa('hello', 'hell') = false");

    TEST(ps_equal_nocase(ps1, ps3), "ps_equal_nocase('hello', 'HELLO') = true");
    TEST(!ps_equal_nocase(ps1, ps4), "ps_equal_nocase('hello', 'world') = false");

    TEST_EQ(ps_packed_compare(ps1, ps2), 0, "ps_packed_compare('hello', 'hello') = 0");
    TEST_NOT_EQ(ps_packed_compare(ps1, ps5), 0, "ps_packed_compare('hello', 'hell') != 0");

    TEST(ps_compare(ps1, ps5) > 0, "ps_compare('hello', 'hell') > 0");
    TEST(ps_compare(ps5, ps1) < 0, "ps_compare('hell', 'hello') < 0");
    TEST_EQ(ps_compare(ps1, ps2), 0, "ps_compare('hello', 'hello') = 0");

    return failures;
}

// ============================================================================
// STRING OPERATIONS TESTS
// ============================================================================

int test_string_operations() {
    section("String Operations");
    int failures = 0;

    PackedString ps = ps_pack("hello_world");
    PackedString prefix = ps_pack("hello");
    PackedString suffix = ps_pack("world");
    PackedString not_prefix = ps_pack("world");
    PackedString not_suffix = ps_pack("hello");

    TEST(ps_starts_with(ps, prefix), "ps_starts_with('hello_world', 'hello') = true");
    TEST(!ps_starts_with(ps, not_prefix), "ps_starts_with('hello_world', 'world') = false");
    TEST(ps_ends_with(ps, suffix), "ps_ends_with('hello_world', 'world') = true");
    TEST(!ps_ends_with(ps, not_suffix), "ps_ends_with('hello_world', 'hello') = false");

    PackedString skipped = ps_skip(ps, 6);
    char buffer[PACKED_STRING_MAX_LEN + 1];
    ps_unpack(skipped, buffer);
    TEST_STR_EQ(buffer, "world", "ps_skip('hello_world', 6) = 'world'");

    PackedString truncated = ps_trunc(ps, 5);
    ps_unpack(truncated, buffer);
    TEST_STR_EQ(buffer, "hello", "ps_trunc('hello_world', 5) = 'hello'");

    PackedString sub = ps_substring(ps, 6, 5);
    ps_unpack(sub, buffer);
    TEST_STR_EQ(buffer, "world", "ps_substring('hello_world', 6, 5) = 'world'");

    PackedString a = ps_pack("hello");
    PackedString b = ps_pack("_world");
    PackedString concat = ps_concat(a, b);
    ps_unpack(concat, buffer);
    TEST_STR_EQ(buffer, "hello_world", "ps_concat('hello', '_world') = 'hello_world'");

    PackedString long_a = ps_pack("abcdefghij");
    PackedString long_b = ps_pack("klmnopqrst");
    PackedString too_long = ps_concat(long_a, long_b);
    TEST_EQ(ps_length(too_long), 20, "ps_concat(10+10) length = 20");

    return failures;
}

// ============================================================================
// CASE CONVERSION TESTS
// ============================================================================

int test_case_conversion() {
    section("Case Conversion");
    int failures = 0;

    PackedString mixed = ps_pack("HelloWorld");
    char buffer[PACKED_STRING_MAX_LEN + 1];

    PackedString lower = ps_to_lower(mixed);
    ps_unpack(lower, buffer);
    TEST_STR_EQ(buffer, "helloworld", "ps_to_lower('HelloWorld') = 'helloworld'");
    TEST(!ps_is_case_sensitive(lower), "ps_to_lower() flags &= ~CASE");

    PackedString upper = ps_to_upper(mixed);
    ps_unpack(upper, buffer);
    TEST_STR_EQ(buffer, "HELLOWORLD", "ps_to_upper('HelloWorld') = 'HELLOWORLD'");
    TEST(ps_is_case_sensitive(upper), "ps_to_upper() flags |= CASE");

    return failures;
}

// ============================================================================
// PADDING TESTS
// ============================================================================

int test_padding() {
    section("Padding Operations");
    int failures = 0;

    PackedString ps = ps_pack("hello");
    char buffer[PACKED_STRING_MAX_LEN + 1];

    PackedString left_pad = ps_pad_left(ps, ps_char('_'), 10);
    ps_unpack(left_pad, buffer);
    TEST_STR_EQ(buffer, "_____hello", "ps_pad_left('hello', '_', 10) = '_____hello'");

    PackedString right_pad = ps_pad_right(ps, ps_char('_'), 10);
    ps_unpack(right_pad, buffer);
    TEST_STR_EQ(buffer, "hello_____", "ps_pad_right('hello', '_', 10) = 'hello_____'");

    PackedString center_pad = ps_pad_center(ps, ps_char('_'), 11);
    ps_unpack(center_pad, buffer);
    TEST_STR_EQ(buffer, "___hello___", "ps_pad_center('hello', '_', 11) = '___hello___'");

    PackedString no_pad = ps_pad_left(ps, ps_char('_'), 3);
    ps_unpack(no_pad, buffer);
    TEST_STR_EQ(buffer, "hello", "ps_pad_left(..., 3) = 'hello' (unchanged)");

    return failures;
}

// ============================================================================
// SEARCH OPERATIONS TESTS
// ============================================================================

int test_search() {
    section("Search Operations");
    int failures = 0;

    PackedString ps = ps_pack("hello_world_hello");

    TEST_EQ(ps_find_six(ps, ps_char('h')), 0, "ps_find_six('h') = 0");
    TEST_EQ(ps_find_six(ps, ps_char('o')), 4, "ps_find_six('o') = 4");
    TEST_EQ(ps_find_six(ps, ps_char('x')), -1, "ps_find_six('x') = -1");

    TEST_EQ(ps_find_from_six(ps, ps_char('h'), 1), 12, "ps_find_from_six('h', 1) = 12");
    TEST_EQ(ps_find_from_six(ps, ps_char('o'), 5), 7, "ps_find_from_six('o', 5) = 7");

    TEST_EQ(ps_find_last_six(ps, ps_char('h')), 12, "ps_find_last_six('h') = 12");
    TEST_EQ(ps_find_last_six(ps, ps_char('o')), 16, "ps_find_last_six('o') = 16");

    TEST(ps_contains_six(ps, ps_char('h')), "ps_contains_six('h') = true");
    TEST(ps_contains_six(ps, ps_char('_')), "ps_contains_six('_') = true");
    TEST(!ps_contains_six(ps, ps_char('x')), "ps_contains_six('x') = false");

    PackedString pat1 = ps_pack("world");
    PackedString pat2 = ps_pack("xyz");
    TEST(ps_contains(ps, pat1), "ps_contains('world') = true");
    TEST(!ps_contains(ps, pat2), "ps_contains('xyz') = false");

    return failures;
}

// ============================================================================
// HASHING TESTS
// ============================================================================

int test_hashing() {
    section("Hashing Operations");
    int failures = 0;

    PackedString ps1 = ps_pack("hello");
    PackedString ps2 = ps_pack("hello");
    PackedString ps3 = ps_pack("world");

    TEST_EQ(ps_hash32(ps1), ps_hash32(ps2), "ps_hash32('hello') = ps_hash32('hello')");
    TEST_EQ(ps_hash64(ps1), ps_hash64(ps2), "ps_hash64('hello') = ps_hash64('hello')");
    TEST(ps_hash32(ps1) != ps_hash32(ps3) || ps_hash64(ps1) != ps_hash64(ps3), "hash('hello') != hash('world')");

    u32 h1 = ps_table_hash(ps1);
    u32 h2 = ps_table_hash(ps2);
    TEST_EQ(h1, h2, "ps_table_hash('hello') = ps_table_hash('hello')");

    return failures;
}

// ============================================================================
// LOCK/UNLOCK TESTS
// ============================================================================

int test_lock_unlock() {
    section("Lock/Unlock Operations");
    int failures = 0;

    PackedString original = ps_pack("secret_data");
    PackedString key = ps_pack("key123");
    char buffer[PACKED_STRING_MAX_LEN + 1];

    PackedString locked = ps_lock(original, key);
    TEST(!ps_equal(original, locked), "ps_lock() != original");

    ps_unpack(locked, buffer);
    TEST(strcmp(buffer, "secret_data") != 0, "ps_unpack(locked) != 'secret_data'");

    PackedString unlocked = ps_unlock(locked, key);
    ps_unpack(unlocked, buffer);
    TEST_STR_EQ(buffer, "secret_data", "ps_unlock(locked, key) = 'secret_data'");

    PackedString wrong_key = ps_pack("wrong");
    PackedString still_locked = ps_unlock(locked, wrong_key);
    ps_unpack(still_locked, buffer);
    TEST(strcmp(buffer, "secret_data") != 0, "ps_unlock(locked, wrong_key) != 'secret_data'");

    return failures;
}

// ============================================================================
// VALIDATION TESTS
// ============================================================================

int test_validation() {
    section("Validation");
    int failures = 0;

    PackedString valid1 = ps_pack("hello");
    PackedString valid2 = ps_pack("var_name");
    PackedString valid3 = ps_pack("x123");
    PackedString invalid1 = ps_pack("123abc");
    PackedString invalid2 = ps_pack("0hello");

    TEST(ps_is_valid_identifier(valid1), "ps_is_valid_identifier('hello') = true");
    TEST(ps_is_valid_identifier(valid2), "ps_is_valid_identifier('var_name') = true");
    TEST(ps_is_valid_identifier(valid3), "ps_is_valid_identifier('x123') = true");
    TEST(!ps_is_valid_identifier(invalid1), "ps_is_valid_identifier('123abc') = false");
    TEST(!ps_is_valid_identifier(invalid2), "ps_is_valid_identifier('0hello') = false");

    return failures;
}

// ============================================================================
// DEBUGGING TESTS
// ============================================================================

int test_debugging() {
    section("Debugging Functions");
    int failures = 0;

    PackedString ps = ps_pack("Hello123");
    char buffer[1024];

    i32 hex_len = psd_hex(ps, buffer);
    TEST(hex_len > 0, "psd_hex() > 0");
    TEST(strlen(buffer) == 32, "psd_hex() length = 32");

    i32 bin_len = psd_binary(ps, buffer);
    TEST(bin_len > 0, "psd_binary() > 0");

    i32 enc_len = psd_encoding_binary(ps, buffer);
    TEST(enc_len > 0, "psd_encoding_binary() > 0");

    i32 info_len = psd_info(ps, buffer);
    TEST(info_len > 0, "psd_info() > 0");
    TEST(strstr(buffer, "Hello123") != NULL, "psd_info() contains 'Hello123'");

    i32 inspect_len = psd_inspect(ps, buffer);
    TEST(inspect_len > 0, "psd_inspect() > 0");

    i32 cstr_len = psd_cstr(ps, buffer);
    TEST(cstr_len > 0, "psd_cstr() > 0");
    TEST_STR_EQ(buffer, "Hello123", "psd_cstr() = 'Hello123'");

    char* warped = psd_warper(psd_info, ps);
    TEST(warped != NULL, "psd_warper() != NULL");
    TEST(strlen(warped) > 0, "psd_warper() length > 0");

    return failures;
}

// ============================================================================
// COMPILE-TIME HELPERS TESTS
// ============================================================================

int test_compile_time() {
    section("Compile-time Helpers");
    int failures = 0;

    PackedString literal = PS_LITERAL("Hello");
    char buffer[PACKED_STRING_MAX_LEN + 1];
    ps_unpack(literal, buffer);
    TEST_STR_EQ(buffer, "Hello", "PS_LITERAL('Hello') = 'Hello'");
    TEST(ps_is_case_sensitive(literal), "PS_LITERAL() flags |= CASE");

    PS_STATIC_ASSERT_LEN("valid");
    TEST(1, "PS_STATIC_ASSERT_LEN('valid') compiles");

    return failures;
}

// ============================================================================
// EDGE CASES TESTS
// ============================================================================

int test_edge_cases() {
    section("Edge Cases");
    int failures = 0;

    char buffer[PACKED_STRING_MAX_LEN + 1];

    PackedString empty = ps_pack("");
    TEST(ps_valid(empty), "ps_valid(ps_pack('')) = true");
    TEST_EQ(ps_length(empty), 0, "ps_length(ps_pack('')) = 0");
    TEST(ps_is_empty(empty), "ps_is_empty(ps_pack('')) = true");
    TEST_EQ(ps_unpack(empty, buffer), 0, "ps_unpack('') = 0");
    TEST_STR_EQ(buffer, "", "ps_unpack('') = ''");

    PackedString single = ps_pack("a");
    TEST_EQ(ps_length(single), 1, "ps_length('a') = 1");
    TEST_EQ(ps_first(single), ps_char('a'), "ps_first('a') = 'a'");
    TEST_EQ(ps_last(single), ps_char('a'), "ps_last('a') = 'a'");

    PackedString max = ps_pack("abcdefghijklmnopqrst");
    TEST_EQ(ps_length(max), 20, "ps_length(20-char) = 20");
    TEST(ps_valid(max), "ps_valid(20-char) = true");

    PackedString special = ps_pack("_$");
    TEST(ps_contains_special(special), "ps_contains_special('_$') = true");
    TEST_EQ(ps_at(special, 0), ps_char('_'), "ps_at('_$', 0) = '_'");
    TEST_EQ(ps_at(special, 1), ps_char('$'), "ps_at('_$', 1) = '$'");

    PackedString invalid_char = ps_pack("hello@world");
    TEST(!ps_valid(invalid_char), "ps_valid('hello@world') = false");

    PackedString null_ex = ps_pack_ex(NULL, 5, 0);
    TEST(!ps_valid(null_ex), "ps_valid(ps_pack_ex(NULL)) = false");

    return failures;
}

// ============================================================================
// PERFORMANCE TESTS (minimal)
// ============================================================================

void test_performance() {
    section("Performance (rough timing)");

    const int ITERATIONS = 1000000;
    clock_t start, end;

    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        PackedString ps = ps_pack("hello_world");
        (void)ps;
    }
    end = clock();
    printf("  ps_pack: %.2f ms per 1000 ops\n",
           (double)(end - start) * 1000 / CLOCKS_PER_SEC / ((double)ITERATIONS/1000));

    PackedString a = ps_pack("hello");
    PackedString b = ps_pack("hello");
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        bool eq = ps_equal(a, b);
        (void)eq;
    }
    end = clock();
    printf("  ps_equal: %.2f ns per op\n",
           (double)(end - start) * 1e9 / CLOCKS_PER_SEC / ITERATIONS);

    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        u32 h = ps_hash32(a);
        (void)h;
    }
    end = clock();
    printf("  ps_hash32: %.2f ns per op\n",
           (double)(end - start) * 1e9 / CLOCKS_PER_SEC / ITERATIONS);
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    printf( "=================================================\n"
            "        PackedString Comprehensive Tests\n"
            "=================================================\n");

    int failed = 0;

    failed += test_static_functions();
    failed += test_core_operations();
    failed += test_flags();
    failed += test_character_access();
    failed += test_comparisons();
    failed += test_string_operations();
    failed += test_case_conversion();
    failed += test_padding();
    failed += test_search();
    failed += test_hashing();
    failed += test_lock_unlock();
    failed += test_validation();
    failed += test_debugging();
    failed += test_compile_time();
    failed += test_edge_cases();

    section("Summary");

    if (failed == 0) {
        printf("✅ All tests passed!\n");
    } else {
        printf("❌ %d test(s) failed\n", failed);
    }

    test_performance();

    return failed > 0 ? 1 : 0;
}