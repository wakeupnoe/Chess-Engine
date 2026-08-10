#pragma once
#include "types.h"

extern Bitboard KingAttacks[64];
extern Bitboard KnightAttacks[64];

extern Bitboard RookAttacks[102400]; // 64*(1024/2048/4096 depending on if the rook is in the middle, on the the corner, or on the edge of the board)
extern Bitboard BishopAttacks[5248]; // 64*(128/64/32 depending where the bishop is)


extern Bitboard rookMasks[64];
extern Bitboard rookMagic[64];
extern Bitboard rookOffsets[64];
extern int rookShifts[64];

extern Bitboard bishopMasks[64];
extern Bitboard bishopMagic[64];
extern Bitboard bishopOffsets[64];
extern int bishopShifts[64];

inline Bitboard get_rook_attacks(Square sq, Bitboard allPieces){
    Bitboard blockers = allPieces & rookMasks[sq];
    int index = (rookMagic[sq]*blockers) >> rookShifts[sq];
    return RookAttacks[rookOffsets[sq] + index];
}

inline Bitboard get_bishop_attacks(Square sq, Bitboard allPieces){
    Bitboard blockers = allPieces & bishopMasks[sq];
    int index = (bishopMagic[sq]*blockers) >> bishopShifts[sq];
    return BishopAttacks[bishopOffsets[sq] + index];
}

inline Bitboard get_queen_attacks(Square sq, Bitboard allPieces){
    return get_bishop_attacks(sq,allPieces) | get_rook_attacks(sq,allPieces);
}
