#include "attacks.h"
#include "bitboard.h"


namespace {
    Bitboard set_blockers(int index, int bitcount, Bitboard attack_mask){
        Bitboard blockers = 0ULL;
        for(int i = 0; i<bitcount; i++){
            int lowest_position = lsb(attack_mask);
            pop_lsb(attack_mask);
            if(index & (1<<i)){
                SetBit(blockers, lowest_position);
            }
        }
        return blockers;
    }
}


Bitboard KnightAttacks[64];
Bitboard KingAttacks[64];
Bitboard RookAttacks[102400];   // 64*(1024/2048/4096 depending on if the rook is in the middle, on the the corner, or on the edge of the board)
                                // no need to index this since we use inline function in attacks.h for the index calculation
Bitboard BishopAttacks[5248];

Bitboard rookMasks[64];
Bitboard rookOffsets[64];
int rookShifts[64];

Bitboard bishopMasks[64];
Bitboard bishopOffsets[64];
int bishopShifts[64];

const Bitboard bishopMagic[64] = {
    0x40040844404084ULL,   0x2004208a004208ULL,   0x10190041080202ULL,   0x108060845042010ULL,
    0x581104180800210ULL,  0x211208044620004ULL,  0x41105040825080ULL,   0x41040810a4002000ULL,
    0x140002012040000ULL,  0x10c22204082004ULL,   0x228004808a4104ULL,   0x220104010005084ULL,
    0x801020400822084ULL,  0x802220404411084ULL,  0x4010201020050c4ULL,  0x10500a0284422ULL,
    0x42001156080802ULL,   0x820c2201010c2ULL,    0x41110008404284ULL,   0x40200810444102ULL,
    0x2101008082210ULL,    0x201014200840100ULL,  0x101201240a202ULL,    0x800a00222200402ULL,
    0x1102010420204ULL,    0x408102004011400ULL,  0x100411208104ULL,     0x20140a204010aULL,
    0x210401020824ULL,     0x20010820104402ULL,   0x40224010804ULL,      0x420084080084204ULL,
    0x4010120020004ULL,    0x20001020240102ULL,   0x140410402024ULL,     0x1001080801010ULL,
    0x2020810800214ULL,    0x8400044810284ULL,    0x4000140220882ULL,    0x410101080200cULL,
    0x81010108410202ULL,   0x20082004112ULL,      0x800100202084ULL,     0x201004400408ULL,
    0x421008201082ULL,     0x1000804008201ULL,    0x40210000a20ULL,      0x10204100401ULL,
    0x4010808202004ULL,    0x20110000a0020ULL,    0x8110040210104ULL,    0x40080040040402ULL,
    0x8102040008801ULL,    0x21010040810ULL,      0x2000110201002ULL,    0x4080100104ULL,
    0x402020210444ULL,     0x10080a0100404ULL,    0x4002081084ULL,       0x8001001002ULL,
    0x1004020020022ULL,    0x2080001008ULL,       0x40010020100ULL,      0x840010104ULL
};

const Bitboard rookMagic[64] = {
    0x8a80104000800020ULL, 0x140002000100040ULL, 0x2801880a0017001ULL, 0x100081001000420ULL,
    0x200020010080420ULL,  0x3001c0002010008ULL, 0x8480008002000100ULL, 0x2080088004402900ULL,
    0x800098204000ULL,     0x2024401000200040ULL, 0x100802000801000ULL, 0x120800800801000ULL,
    0x208808088000400ULL,  0x2802200800400ULL,   0x2200800100020080ULL, 0x801000060821100ULL,
    0x80044006422000ULL,   0x100808020004000ULL, 0x12108a0010204200ULL, 0x140848010000802ULL,
    0x481828014002800ULL,  0x8094004002004100ULL, 0x4010040010010802ULL, 0x20008806104ULL,
    0x100400080208000ULL,  0x2040002120081000ULL, 0x21200680100081ULL,   0x20100080080080ULL,
    0x2000a00200410ULL,    0x20080800400ULL,     0x80088400100102ULL,   0x80004600042881ULL,
    0x4040008040800020ULL, 0x440003000200801ULL, 0x4200011004500ULL,    0x188020010100100ULL,
    0x14800401802800ULL,   0x2080040080800200ULL, 0x124080204001001ULL,  0x200046502000484ULL,
    0x480400080088020ULL,  0x1000422010034000ULL, 0x30200100110040ULL,   0x100021010009ULL,
    0x2002080100110004ULL, 0x202008004008002ULL, 0x20020004010100ULL,   0x2048440040820001ULL,
    0x101002200408200ULL,  0x40802000401080ULL,  0x4008142004410100ULL, 0x206082030100200ULL,
    0x1001004080111ULL,    0x1000800811202000ULL, 0x2020010411000200ULL, 0x2008000810800400ULL,
    0x112000a0011002ULL,   0x202004010101ULL,    0x200801011100100ULL,  0x200401101011ULL,
    0x41110048102ULL,      0x410408041004ULL,    0x1022020212ULL,       0x8400a00820ULL
};

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

    for(int sq= 0; sq<64; sq++){ // ROOK
        rookMasks[sq] = 0ULL;
        int file = sq%8, rank = sq/8;
        for(int r = rank + 1; r<7; r++) rookMasks[sq] |= 1ULL << (r*8+file); // up
        for(int r = rank - 1; r>0; r--) rookMasks[sq] |= 1ULL << (r*8+file); // down
        for(int f = file + 1; f<7; f++) rookMasks[sq] |= 1ULL << (rank*8+f); // right
        for(int f = file - 1; f>0; f--) rookMasks[sq] |= 1ULL << (rank*8+f); // left
        rookShifts[sq] = 64 - popcount(rookMasks[sq]);
        rookOffsets[sq] = sq>0 ? rookOffsets[sq-1] + (1ULL << (64-rookShifts[sq-1])) : 0;
        
        int bitcount = popcount(rookMasks[sq]);
        for(int c = 0; c < (1ULL << bitcount); c++){
            Bitboard blockers = set_blockers(c, bitcount, rookMasks[sq]); // generate a blockers layout;
            Bitboard attacks = 0ULL;
            for(int r = rank + 1; r<8; r++){ // up
                int cursq = r*8+file;
                SetBit(attacks,cursq);
                if(GetBit(blockers, cursq)) break;
            }
            for(int r = rank - 1; r>=0; r--){ // down
                int cursq = r*8+file;
                SetBit(attacks,cursq);
                if(GetBit(blockers, cursq)) break;
            }
            for(int f = file + 1; f<8; f++){
                int cursq = rank*8+f;
                SetBit(attacks, cursq);
                if(GetBit(blockers, cursq)) break;
            }
            for(int f = file - 1; f>=0; f--){
                int cursq = rank*8+f;
                SetBit(attacks, cursq);
                if(GetBit(blockers, cursq)) break;
            }
            
            int magic_index = (rookMagic[sq]*blockers) >> rookShifts[sq];
            RookAttacks[rookOffsets[sq] + magic_index] = attacks;
        }
    }

    for(int sq= 0; sq<64; sq++){ // BISHOP
        bishopMasks[sq] = 0ULL;
        int file = sq%8, rank = sq/8;
        for(int r = rank + 1, f = file + 1; r<7 && f<7; r++, f++) bishopMasks[sq] |= 1ULL << (r*8+f); // up right
        for(int r = rank - 1, f = file + 1; r>0 && f<7; r--, f++) bishopMasks[sq] |= 1ULL << (r*8+f); // down right
        for(int r = rank + 1, f = file - 1; r<7 && f>0; r++, f--) bishopMasks[sq] |= 1ULL << (r*8+f); // up left
        for(int r = rank - 1, f = file - 1; r>0 && f>0; r--, f--) bishopMasks[sq] |= 1ULL << (r*8+f); // down left

        bishopShifts[sq] = 64 - popcount(bishopMasks[sq]);
        bishopOffsets[sq] = sq>0 ? bishopOffsets[sq-1] + (1ULL << (64-bishopShifts[sq-1])) : 0;
        
        int bitcount = popcount(bishopMasks[sq]);
        for(int c = 0; c < (1ULL << bitcount); c++){
            Bitboard blockers = set_blockers(c, bitcount, bishopMasks[sq]); // generate a blockers layout;
            Bitboard attacks = 0ULL;
            for(int r = rank + 1, f = file + 1; r<=7 && f<=7; r++, f++){ // up right
                int cursq = r*8+f;
                SetBit(attacks,cursq);
                if(GetBit(blockers, cursq)) break;
            }
            for(int r = rank - 1, f = file + 1; r>=0 && f<7; r--, f++){ // down right
                int cursq = r*8+f;
                SetBit(attacks,cursq);
                if(GetBit(blockers, cursq)) break;
            }
            for(int r = rank + 1, f = file - 1; r<=7 && f>=0; r++, f--){
                int cursq = r*8+f;
                SetBit(attacks, cursq);
                if(GetBit(blockers, cursq)) break;
            }
            for(int r = rank - 1, f = file - 1; r>=0 && f>=0; r--, f--){
                int cursq = r*8+f;
                SetBit(attacks, cursq);
                if(GetBit(blockers, cursq)) break;
            }
            
            int magic_index = (bishopMagic[sq]*blockers) >> bishopShifts[sq];
            BishopAttacks[bishopOffsets[sq] + magic_index] = attacks;
        }
    }
}