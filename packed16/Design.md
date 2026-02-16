# Packed16 — Internal Usage & Design Document

## 1. Purpose

`Packed16` is a fixed-width, 128-bit string representation designed for:

* High-performance hash table keys
* VM identifiers
* Token storage
* Constant-time equality checks
* Cache-efficient symbol systems

It replaces heap-allocated strings for short identifiers (≤ 20 chars).

It is not intended for general text storage.

---

# 2. Storage Model

## 2.1 Physical Layout

```c
typedef struct {
    u64 lo;
    u64 hi;
} Packed16;
```

Total size: 16 bytes.

---

## 2.2 Bit Allocation

```
Total bits: 128

Payload: 120 bits
Metadata: 8 bits (top of hi)
```

### Payload

20 characters × 6 bits = 120 bits

Characters are packed sequentially:

```
char0 → lowest 6 bits of lo
char1 → next 6 bits
...
char10 crosses lo/hi boundary
...
char19 → highest payload bits in hi
```

### Metadata (8 bits)

Located in the highest 8 bits of `hi`.

```
[ flags:3 ][ length:5 ]
```

* Length range: 0–20
* Flags: semantic hints (digit/special/case)

---

# 3. Encoding Model

## 3.1 6-bit Alphabet

Character → 6-bit value

| Value | Character |
|-------|-----------|
| 0–9   | '0'–'9'   |
| 10–35 | 'a'–'z'   |
| 36–61 | 'A'–'Z'   |
| 62    | '_'       |
| 63    | '$'       |

Only these characters are valid.

Encoding is deterministic and reversible.

---

# 4. Invariants

These must always hold:

1. Length ≤ 20
2. Metadata length matches number of packed characters
3. Characters are contiguous (no gaps)
4. Unused bits are zeroed
5. Payload does not overlap metadata

Violating these leads to undefined behavior.

---

# 5. Core Operations — Internal Behavior

---

## 5.1 ps_pack

**Purpose:** Encode ASCII string into 6-bit packed format.

### Internally:

1. Validate length ≤ 20
2. Map each char → 6-bit value
3. Insert sequentially into 128-bit region
4. Compute flags (digit/special/case)
5. Store metadata in top 8 bits of `hi`

Time: O(N), N ≤ 20 (bounded constant)

Failure returns invalid structure.

---

## 5.2 ps_unpack

**Purpose:** Restore ASCII string.

### Internally:

1. Read length from metadata
2. Extract each 6-bit segment
3. Map back to ASCII
4. Write null-terminated output

Time: O(N), N ≤ 20

---

## 5.3 ps_scan

**Purpose:** Fix string flags.

Scan all characters and set the correct flags.

---

## 5.4 ps_equal

**Purpose:** Constant-time equality.

### Internally:

```
return (a.lo == b.lo) && (a.hi == b.hi);
```

No loops.
Compares full 128 bits including metadata.

This is the primary performance advantage.

---

## 5.5 ps_equal_nometa

Same as ps_equal but ignores metadata bits.

Used when semantic flags are irrelevant.

---

## 5.6 ps_equal_nocase

**Purpose:** Case-insensitive comparison.

### Internally:

1. Check lengths
2. For each character:

    * Extract 6-bit value
    * Normalize via lookup table
    * Compare

Uses precomputed lowercase table for speed.

Time: O(N), N ≤ 20

---

## 5.7 ps_hash32 / ps_hash64

**Purpose:** Produce stable hash for hash tables.

### Internally:

1. Combine `lo` and `hi`
2. Apply mixing (avalanche)
3. Return 32-bit or 64-bit value

No character iteration required.

---

## 5.8 ps_compare

**Purpose:** Lexicographic ordering.

### Internally:

Iterates character-by-character until difference.

Used for ordered containers.

---

## 5.9 ps_substring

**Purpose:** Extract contiguous region.

### Internally:

1. Shift payload right by (start × 6)
2. Mask required length × 6 bits
3. Insert new metadata

No character loops required.

---

## 5.10 ps_concat

**Purpose:** Combine two Packed16 values.

### Internally:

1. Check combined length ≤ 20
2. Shift second string left by (lenA × 6)
3. OR into first
4. Merge flags
5. Write new metadata

Constant-time bit manipulation.

---

## 5.11 ps_shl / ps_shr

**Purpose:** Bit-level shift across 128-bit boundary.

### Internally:

Performs manual 128-bit shift using:

```
new_lo = (lo << n)
new_hi = (hi << n) | (lo >> (64 - n))
```

Used in:

* Substring
* Concat
* Low-level encoding ops

---

## 5.12 ps_set

**Purpose:** Replace character at index.

### Internally:

1. Clear 6-bit segment at index
2. Insert new 6-bit value
3. Does NOT automatically update flags

Used mainly in transformation utilities.

---

## 5.13 ps_find / ps_contains

**Purpose:** Search for character or substring.

### Internally:

Iterates per character and compares 6-bit segments.

Worst-case O(N²), but N ≤ 20.

---

## 5.14 ps_to_lower / ps_to_upper

**Purpose:** Case transformation.

### Internally:

Loop over characters and remap via table.

Metadata case flag updated accordingly.

---

## 5.15 ps_pad_left / right / center

**Purpose:** Extend string to target length.

### Internally:

* Compute required padding
* Shift payload
* Insert fill character segments
* Update metadata

---

## 5.16 ps_lock / ps_unlock

**Purpose:** Lightweight reversible transform.

### Internally:

```
rotateleft(&lo, &hi, len(key))
lo ^= key.lo
hi ^= key.hi
```

Reversible via same operation.

Not cryptographically secure.

---

# 6. Performance Properties

* Size: 16 bytes
* No dynamic memory
* Equality: 2 integer compares
* Hash: constant-time
* Excellent cache locality
* Predictable branch behavior

Designed specifically for hash-table keys.

---

# 7. When To Use

Use Packed16 when:

* Identifiers ≤ 20 chars
* Hash table performance matters
* Memory footprint matters
* You need constant-time equality
* You control the character set

Do NOT use for:

* Arbitrary Unicode text
* Long strings
* User-facing text
* Network serialization without format definition

---

# 8. Integration Guidelines

For hash tables:

* Use ps_hash64
* Use ps_equal
* Store Packed16 directly in bucket
* Avoid heap allocations

For language types:

* Make immutable
* Avoid exposing bit-level mutation
* Keep metadata format stable

---

# 9. Stability Contract

Once exposed as language type:

* Bit layout must never change
* Metadata layout must never change
* Hash algorithm must remain stable
* Alphabet must remain fixed

Otherwise, serialized bytecode or symbol tables break.

---

# 10. Summary

Packed16 is:

* A fixed 128-bit identifier container
* Deterministic and branch-minimal
* Cache-efficient
* Optimized for hash keys

It trades flexibility for predictable performance.
