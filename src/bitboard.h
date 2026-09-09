#pragma once
#include "types.h"
#include <bit>
#include <cassert>

// CONTRACT [Bitboards]:
// - ASSUMES: All bit-shift targets ('tar') are strictly 0-63. Passing NO_SQUARE (64) triggers Undefined Behavior.
// - ASSUMES: Bitboard 'b' is strictly non-zero for lsb() and pop_lsb() to prevent 64-bit overflow.
// - GUARANTEES: Fast, bounds-checked bit manipulation and mask generation.


inline void SetBit(Bitboard &b, uint8_t tar){
    assert(tar < 64 && "Bit shift out of bounds (NO_SQUARE passed?)");
    b |= (1ULL<<tar);
}
inline bool GetBit(Bitboard b, uint8_t tar){
    assert(tar < 64 && "Bit shift out of bounds (NO_SQUARE passed?)");
    return b & (1ULL<<tar);
}
inline void PopBit(Bitboard &b, uint8_t tar){
    assert(tar < 64 && "Bit shift out of bounds (NO_SQUARE passed?)");
    b &= ~(1ULL<<tar);
}
inline int popcount(Bitboard b){
    return std::popcount(b);
}
inline int lsb(Bitboard b){
    assert(b != 0ULL && "Attempted lsb() on an empty bitboard");
    return std::countr_zero(b);
}
inline int pop_lsb(Bitboard &b){
    int sq = lsb(b);
    b&=(b-1);
    return sq;
}

inline Bitboard SquareBB(uint8_t tar){
    assert(tar < 64 && "Bit shift out of bounds (NO_SQUARE passed?)");
    return 1ULL<<tar;
}


inline constexpr Bitboard FileBB[8] = {
    0x0101010101010101ULL,
    0x0202020202020202ULL,
    0x0404040404040404ULL,
    0x0808080808080808ULL,
    0x1010101010101010ULL,
    0x2020202020202020ULL,
    0x4040404040404040ULL,
    0x8080808080808080ULL
};

inline constexpr Bitboard RankBB[8] = {
    0x00000000000000FFULL,
    0x000000000000FF00ULL,
    0x0000000000FF0000ULL,
    0x00000000FF000000ULL,
    0x000000FF00000000ULL,
    0x0000FF0000000000ULL,
    0x00FF000000000000ULL,
    0xFF00000000000000ULL
};