#ifndef PACKED_ENCODING_H
#define PACKED_ENCODING_H

#include "types.h"

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

// Lookup table for uppercase conversion (6-bit values)
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

#endif // PACKED_ENCODING_H