# Packed16

## Memory Layout

`PackedString` = 128 bits:

```
[ lo: 64 bits ][ hi: 64 bits ]
```

### Character Storage (120 bits)

| Character Index | Bit Location        |
|-----------------|---------------------|
| 0–9             | lo[0:59]            |
| 10              | lo[60:63] + hi[0:1] |
| 11–19           | hi[2:55]            |

### Metadata (8 bits in hi)

```
hi[56:58]  → flags (3 bits)
hi[59:63]  → length (5 bits)
```

---

## Flags

| Bit | Meaning          |
|-----|------------------|
| 0   | CASE_SENSITIVE   |
| 1   | CONTAINS_DIGIT   |
| 2   | CONTAINS_SPECIAL |

Flags are advisory and allow O(1) checks for:

* identifier validity
* nocase fast path
* lexer optimizations

---

## Equality Model

### Fast Exact Equality

```
a.lo == b.lo && a.hi == b.hi
```

Two 64-bit comparisons.

---

### Packed Compare (ignore metadata)

Compare 120-bit region only.

Used for sorting or ordering.

---

### Case-Insensitive Equality

Algorithm:

1. If both have CASE_SENSITIVE=0 → fast equal
2. Else fallback to per-sixbit compare using lookup table

Max 20 iterations.

---

## Why Not Use `uint128`?

* Not portable
* Not standard C
* Not available in many compilers
* Harder to port to other languages

Design uses two u64 for:

* Maximum portability
* Cross-language implementability
* Clear layout specification

---

## Why Not Use `memcmp`?

Because:

* Characters are 6-bit packed, not byte-aligned
* Metadata must be masked
* Comparison is semantic, not raw memory

For packed_compare, direct integer comparison is faster than `memcmp`.

---

## Intended Use Cases

* Bytecode tokenizers
* Identifiers in VM
* Symbol tables
* Keyword matching
* Hash table keys
* DSL engines
* Interpreters
* Embedded scripting

---

## Non-Goals

* UTF-8 support
* Arbitrary-length strings
* Unicode normalization
* Locale awareness

This is a **systems-level identifier string**, not a general-purpose text container.

---

## Stability Level

Packed16 is stable once:

* All API contracts documented
* All error states defined
* All operations tested
* Benchmarks published

After that:

* Packed 8 / 24 / 32 follow same design
