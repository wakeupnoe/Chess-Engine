#pragma once
#include "board.h"
#include "types.h"
#include <cassert>

// CONTRACT [Attacks]:
// - ASSUMES: All 'sq' and 'tar_sq' parameters are strictly 0-63 (A1 to H8). Passing NO_SQUARE (64) WILL cause out-of-bounds array reads (magics, masks, offsets).
// - GUARANTEES: Constant-time attack bitboard generation using Magic Bitboards.

extern Bitboard KingAttacks[64];
extern Bitboard KnightAttacks[64];

extern Bitboard RookAttacks[102400]; // 64*(1024/2048/4096 depending on if the rook is in the middle, on the the corner, or on the edge of the board)
extern Bitboard BishopAttacks[5248]; // 64*(128/64/32 depending where the bishop is)


extern Bitboard rookMasks[64];
//extern Bitboard const rookMagic[64];
extern Bitboard const myRookMagics[64];
extern Bitboard rookOffsets[64];
extern int rookShifts[64];

extern Bitboard bishopMasks[64];
//extern Bitboard const bishopMagic[64];
extern Bitboard const myBishopMagics[64];
extern Bitboard bishopOffsets[64];
extern int bishopShifts[64];


inline Bitboard get_king_attacks(Square sq, Bitboard allPieces){
    assert(sq >= A1 && sq <= H8);
    return KingAttacks[sq] & allPieces;
}

inline Bitboard get_knight_attacks(Square sq, Bitboard allPieces){
    assert(sq >= A1 && sq <= H8);
    return KnightAttacks[sq] & allPieces;
}

template <Color Us>
inline Bitboard get_pawn_attacks(Square sq, Bitboard allPieces){ // single pawn attack (not like moving all at once like in movegen)
    assert(sq >= A1 && sq <= H8);
    Bitboard pbb = SquareBB(sq);
    Bitboard raw_attacks = Us == WHITE ? (pbb << 9 & ~FileBB[FILE_A]) | (pbb << 7 & ~FileBB[FILE_H]) : (pbb >> 9 & ~FileBB[FILE_H]) | (pbb >> 7 & ~FileBB[FILE_A]);
    return raw_attacks & allPieces;
}




inline Bitboard get_rook_attacks(Square sq, Bitboard allPieces){
    assert(sq >= A1 && sq <= H8);
    Bitboard blockers = allPieces & rookMasks[sq];
    int index = (myRookMagics[sq]*blockers) >> rookShifts[sq];
    return RookAttacks[rookOffsets[sq] + index];
}

inline Bitboard get_bishop_attacks(Square sq, Bitboard allPieces){
    assert(sq >= A1 && sq <= H8);
    Bitboard blockers = allPieces & bishopMasks[sq];
    int index = (myBishopMagics[sq]*blockers) >> bishopShifts[sq];
    return BishopAttacks[bishopOffsets[sq] + index];
}

inline Bitboard get_queen_attacks(Square sq, Bitboard allPieces){
    assert(sq >= A1 && sq <= H8);
    return get_bishop_attacks(sq,allPieces) | get_rook_attacks(sq,allPieces);
}


template <Color Attacker>
bool is_square_attacked(Square tar_sq, const Board& board){
    assert(tar_sq >= A1 && tar_sq <= H8);
    Bitboard sq_bb = SquareBB(tar_sq);
    return  (KnightAttacks[tar_sq] & board.get_pieces(Attacker,KNIGHT))
        || (KingAttacks[tar_sq] & board.get_pieces(Attacker,KING))
        || ((Attacker == WHITE ? (sq_bb >> 7) & ~FileBB[FILE_A] : (sq_bb << 7) & ~FileBB[FILE_H]) & board.get_pieces(Attacker,PAWN))
        || ((Attacker == WHITE ? (sq_bb >> 9) & ~FileBB[FILE_H] : (sq_bb << 9) & ~FileBB[FILE_A]) & board.get_pieces(Attacker,PAWN))
        || (get_bishop_attacks(tar_sq,board.get_all_pieces()) & (board.get_pieces(Attacker,BISHOP) | board.get_pieces(Attacker,QUEEN)))
        || (get_rook_attacks(tar_sq,board.get_all_pieces()) & (board.get_pieces(Attacker,ROOK) | board.get_pieces(Attacker,QUEEN)));
}

void init_attacks();