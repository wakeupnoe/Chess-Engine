#pragma once
#include "types.h"

extern Bitboard KingAttacks[64];
extern Bitboard KnightAttacks[64];

extern Bitboard RookAttacks[102400]; // 64*(1024/2048/4096 depending on if the rook is in the middle, on the the corner, or on the edge of the board)
extern Bitboard rookMasks[64];
extern Bitboard rookMagic[64];
extern Bitboard rookOffsets[64];
extern int rookShifts[64];

inline Bitboard get_rook_attacks(Square sq, Bitboard allPieces){
    Bitboard blockers = allPieces & rookMasks[sq];
    int index = (rookMagic[sq]*blockers) >> rookShifts[sq];
    return RookAttacks[rookOffsets[sq] + index];
}
