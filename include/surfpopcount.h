/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _FASTRANK_POPCOUNT_H_
#define _FASTRANK_POPCOUNT_H_

#include <cstdio>
#include <cstdint>

namespace surf {

#define L8 0x0101010101010101UL // Every lowest 8th bit set: 00000001...
#define G2 0xAAAAAAAAAAAAAAAAUL // Every highest 2nd bit: 101010...
#define G4 0x3333333333333333UL // 00110011 ... used to group the sum of 4 bits.
#define G8 0x0F0F0F0F0F0F0F0FUL

#define ONES_STEP_4 ( 0x1111111111111111UL )
#define ONES_STEP_8 ( 0x0101010101010101UL )
#define MSBS_STEP_8 ( 0x80UL * ONES_STEP_8 )
#define INCR_STEP_8 ( 0x80UL << 56 | 0x40UL << 48 | 0x20UL << 40 | 0x10UL << 32 | 0x8UL << 24 | 0x4UL << 16 | 0x2UL << 8 | 0x1 )
	
#define LEQ_STEP_8(x,y) ( ( ( ( ( (y) | MSBS_STEP_8 ) - ( (x) & ~MSBS_STEP_8 ) ) ^ (x) ^ (y) ) & MSBS_STEP_8 ) >> 7 )
#define ZCOMPARE_STEP_8(x) ( ( ( x | ( ( x | MSBS_STEP_8 ) - ONES_STEP_8 ) ) & MSBS_STEP_8 ) >> 7 )

// Population count of a 64 bit integer in SWAR (SIMD within a register) style
// From Sebastiano Vigna, "Broadword Implementation of Rank/Select Queries"
// http://sux.dsi.unimi.it/paper.pdf p4
// This variant uses multiplication for the last summation instead of
// continuing the shift/mask/addition chain.
inline int suxpopcount(uint64_t x) {
    // Step 1:  00 - 00 = 0;  01 - 00 = 01; 10 - 01 = 01; 11 - 01 = 10;
    x = x - ((x & G2) >> 1);
    // step 2:  add 2 groups of 2.
    x = (x & G4) + ((x >> 2) & G4);
    // 2 groups of 4.
    x = (x + (x >> 4)) & G8;
    // Using a multiply to collect the 8 groups of 8 together.
    x = x * L8 >> 56;
    return static_cast<int>(x);
}

// Default to using the GCC builtin popcount.  On architectures
// with -march popcnt, this compiles to a single popcnt instruction.
static inline int popcount(unsigned long x) {
    return __builtin_popcountl(x);
}

#define popcountsize 64UL
#define popcountmask (popcountsize - 1)

inline uint64_t popcountLinear(uint64_t *bits, uint64_t x, uint64_t nbits) {
    if (nbits == 0) { return 0; }
    uint64_t lastword = (nbits - 1) / popcountsize;
    uint64_t p = 0;

    __builtin_prefetch(bits + x + 7, 0); //huanchen
    for (uint64_t i = 0; i < lastword; i++) { /* tested;  manually unrolling doesn't help, at least in C */
        //__builtin_prefetch(bits + x + i + 3, 0);
        p += popcount(bits[x+i]); // note that use binds us to 64 bit popcount impls
    }

    // 'nbits' may or may not fall on a multiple of 64 boundary,
    // so we may need to zero out the right side of the last word
    // (accomplished by shifting it right, since we're just popcounting)
    uint64_t lastshifted = bits[x+lastword] >> (63 - ((nbits - 1) & popcountmask));
    p += popcount(lastshifted);
    return p;
}

// Return the index of the kth bit set in x 
inline int select64_naive(uint64_t x, int k) {
    int count = -1;
    for (int i = 63; i >= 0; i--) {
        count++;
        if (x & (1UL << i)) {
            k--;
            if (k == 0) {
                return count;
            }
        }
    }
    return -1;
}

inline int select64_popcount_search(uint64_t x, int k) {
    int loc = -1;
    // if (k > popcount(x)) { return -1; }

    for (int testbits = 32; testbits > 0; testbits >>= 1) {
        int lcount = popcount(x >> testbits);
        if (k > lcount) {
            x &= ((1UL << testbits)-1);
            loc += testbits;
            k -= lcount;
        } else {
            x >>= testbits;
        }
    }
    return loc+k;
}

inline int select64_broadword(uint64_t x, int k) {
    uint64_t word = x;
    int residual = k;
    uint64_t byte_sums;
    
    byte_sums = word - ( ( word & 0xa * ONES_STEP_4 ) >> 1 );
    byte_sums = ( byte_sums & 3 * ONES_STEP_4 ) + ( ( byte_sums >> 2 ) & 3 * ONES_STEP_4 );
    byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0f * ONES_STEP_8;
    byte_sums *= ONES_STEP_8;
    
    // Phase 2: compare each byte sum with the residual
    const uint64_t residual_step_8 = residual * ONES_STEP_8;
    const int place = ( LEQ_STEP_8( byte_sums, residual_step_8 ) * ONES_STEP_8 >> 53 ) & ~0x7;
    
    // Phase 3: Locate the relevant byte and make 8 copies with incremental masks
    const int byte_rank = residual - ( ( ( byte_sums << 8 ) >> place ) & 0xFF );
    
    const uint64_t spread_bits = ( word >> place & 0xFF ) * ONES_STEP_8 & INCR_STEP_8;
    const uint64_t bit_sums = ZCOMPARE_STEP_8( spread_bits ) * ONES_STEP_8;
    
    // Compute the inside-byte location and return the sum
    const uint64_t byte_rank_step_8 = byte_rank * ONES_STEP_8;
    
    return place + ( LEQ_STEP_8( bit_sums, byte_rank_step_8 ) * ONES_STEP_8 >> 56 );   
}

inline uint64_t select64(uint64_t x, int k) {
    return static_cast<uint64_t>(select64_popcount_search(x, k));
}

// x is the starting offset of the 512 bits;
// k is the thing we're selecting for.
inline int select512(uint64_t *bits, int x, int k) {
    __asm__ __volatile__ (
                          "prefetchnta (%0)\n"
                          : : "r" (&bits[x]) );
    int i = 0;
    int pop = popcount(bits[x+i]);
    while (k > pop && i < 7) {
        k -= pop;
        i++;
        pop = popcount(bits[x+i]);
    }
    if (i == 7 && popcount(bits[x+i]) < k) {
        return -1;
    }
    // We're now certain that the bit we want is stored in bv[x+i]
    return static_cast<int>(i*64 + select64(bits[x+i], k));
}

// brute-force linear select
// x is the starting offset of the bits in bv;
// k is the thing we're selecting for (starting from bv[x]).
// bvlen is the total length of bv
inline int selectLinear(uint64_t* bits, uint64_t length, uint64_t x, uint64_t k) {
    if (k > (length - x) * 64)
        return -1;
    uint64_t i = 0;
    uint64_t pop = popcount(bits[x+i]);
    while (k > pop && i < (length - 1)) {
        k -= pop;
        i++;
        pop = popcount(bits[x+i]);
    }
    if ((i == length - 1) && (pop < k)) {
        return -1;
    }
    // We're now certain that the bit we want is stored in bits[x+i]
    return static_cast<int>(i*64 + select64(bits[x+i], static_cast<int>(k)));
}

// Clean up macros to prevent namespace pollution
#undef L8
#undef G2
#undef G4
#undef G8
#undef ONES_STEP_4
#undef ONES_STEP_8
#undef MSBS_STEP_8
#undef INCR_STEP_8
#undef LEQ_STEP_8
#undef ZCOMPARE_STEP_8
#undef popcountsize
#undef popcountmask

} // namespace surf

#endif /* _FASTRANK_POPCOUNT_H_ */
