#pragma once
#include "types.h"

// CONTRACT [Zobrist Keys]:
// - ASSUMES: zobrist_pieces is indexed STRICTLY with piece 0-5 (PAWN to KING). Passing NO_PIECE_TYPE (6) WILL cause a fatal memory overflow.
// - ASSUMES: zobrist_castling is indexed STRICTLY with a 4-bit integer (0-15).
// - ASSUMES: zobrist_ep is indexed STRICTLY with 0-63 or NO_SQUARE (64).
// - GUARANTEES: zobrist_ep[NO_SQUARE] is 0ULL, meaning XORing it safely results in a no-op.

extern Bitboard zobrist_pieces[2][6][64];
extern Bitboard zobrist_side;
extern Bitboard zobrist_castling[16];
extern Bitboard zobrist_ep[65];

void zobrist_init();