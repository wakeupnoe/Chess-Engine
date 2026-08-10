#include "attacks.h"
#include "bitboard.h"

Bitboard KnightAttacks[64];
Bitboard KingAttacks[64];
Bitboard RookAttacks[102400];    // 64*(1024/2048/4096 depending on if the rook is in the middle, on the the corner, or on the edge of the board)
                                        // no need to index this since we use inline function in attacks.h for the index calculation
Bitboard rookMasks[64];
Bitboard rookMagic[64];
Bitboard rookOffsets[64];
int rookShifts[64];

void init_attacks(){
    for(int sq = 0; sq<64; sq++){ // KING
        Bitboard curPos = SquareBB(sq);
        Bitboard attackBoard = 0ULL;
        attackBoard |= (curPos<<1) & ~FileBB[FILE_A]; //right
        attackBoard |= (curPos>>1) & ~FileBB[FILE_H]; //left
        attackBoard |= (curPos<<8); //up
        attackBoard |= (curPos>>8); //down
        attackBoard |= (curPos<<9) & ~FileBB[FILE_A]; //rightup
        attackBoard |= (curPos<<7) & ~FileBB[FILE_H]; //leftup
        attackBoard |= (curPos>>7) & ~FileBB[FILE_A]; //rightdown
        attackBoard |= (curPos>>9) & ~FileBB[FILE_H]; //leftdown
        KingAttacks[sq]=attackBoard;
    }

    for(int sq = 0; sq<64; sq++){ // KNIGHT
        Bitboard curPos = SquareBB(sq);
        Bitboard attackBoard = 0ULL;

        attackBoard |= (curPos<<17) & ~FileBB[FILE_A]; //right 1 up 2
        attackBoard |= (curPos>>15) & ~FileBB[FILE_A]; //right 1 down 2
        attackBoard |= (curPos<<15) & ~FileBB[FILE_H]; //left 1 up 2
        attackBoard |= (curPos>>17) & ~FileBB[FILE_H]; //left 1 down 2
        
        attackBoard |= (curPos<<10) & ~FileBB[FILE_A] & ~FileBB[FILE_B]; //right 2 up 1
        attackBoard |= (curPos>>6)  & ~FileBB[FILE_A] & ~FileBB[FILE_B]; //right 2 down 1
        attackBoard |= (curPos<<6) & ~FileBB[FILE_H] & ~FileBB[FILE_G]; //left 2 up 1
        attackBoard |= (curPos>>10) & ~FileBB[FILE_H] & ~FileBB[FILE_G]; //left 2 down 1
        
        KnightAttacks[sq]=attackBoard;
    }

    // TODO rook
}