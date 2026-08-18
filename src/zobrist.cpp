#include "zobrist.h"
#include <random>

Bitboard zobrist_pieces[2][6][64];
Bitboard zobrist_side;
Bitboard zobrist_castling[16];
Bitboard zobrist_ep[65];

void zobrist_init(){
    std::mt19937_64 rng(1070372ULL); // number stockfish also uses ^^
    
    for(int c = 0; c<2; c++){
        for(int p = 0; p<6; p++){
            for(int sq = 0; sq<64; sq++) zobrist_pieces[c][p][sq] = rng();
        }    
    }
    zobrist_side = rng();
    for(int cs = 0; cs<16; cs++) zobrist_castling[cs] = rng();
    for(int sq = 0; sq<64; sq++) zobrist_ep[sq] = rng();
    zobrist_ep[64] = 0ULL; // NO_SQUARE target En_passant square should not alter state 
}