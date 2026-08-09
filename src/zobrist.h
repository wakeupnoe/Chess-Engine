#pragma once
#include "types.h"


extern Bitboard zobrist_pieces[2][6][64];
extern Bitboard zobrist_side;
extern Bitboard zobrist_castling[16];
extern Bitboard zobrist_ep[8];

void zobrist_init();