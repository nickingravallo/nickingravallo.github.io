#pragma once

#include <cstdint>
#include <array>

namespace ompeval {

/**
 * Pre-computed lookup tables for fast hand evaluation.
 * These tables are generated at startup and used for O(1) hand ranking.
 */
namespace tables {

// Bit manipulation utilities
inline int popcount(uint32_t v) {
    return __builtin_popcount(v);
}

inline int popcount64(uint64_t v) {
    return __builtin_popcountll(v);
}

// Find highest set bit (0-indexed)
inline int highBit(uint32_t v) {
    return 31 - __builtin_clz(v);
}

// Find lowest set bit (0-indexed)
inline int lowBit(uint32_t v) {
    return __builtin_ctz(v);
}

// Clear highest bit
inline uint32_t clearHighBit(uint32_t v) {
    return v & ~(1u << highBit(v));
}

// Get nth highest bit position
inline int nthHighBit(uint32_t v, int n) {
    for (int i = 0; i < n; ++i) {
        v = clearHighBit(v);
    }
    return highBit(v);
}

// Prime numbers for rank hashing (one for each rank 2-A)
constexpr std::array<uint32_t, 13> RANK_PRIMES = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41
};

// Compute a hash value for a set of cards based on rank products
inline uint32_t rankHash(uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4, uint32_t r5) {
    return RANK_PRIMES[r1] * RANK_PRIMES[r2] * RANK_PRIMES[r3] * 
           RANK_PRIMES[r4] * RANK_PRIMES[r5];
}

} // namespace tables
} // namespace ompeval
