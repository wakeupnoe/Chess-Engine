#pragma once
// AI Generated

#include "board.h"
#include "bitboard.h"
#include "types.h"

// CONTRACT [Evaluation]:
// - ASSUMES: Board bitboards only contain bits in the 0-63 range.
// - GUARANTEES: Safe evaluation without array out-of-bounds reads.

// Standard piece values in centipawns
constexpr int PIECE_VALUES[7] = {
    100, // PAWN
    300, // KNIGHT
    320, // BISHOP (Slightly higher than knight is standard!)
    500, // ROOK
    900, // QUEEN
    0,    // KING (King is infinite, evaluated via checkmate logic instead)
    0    // NO_PIECE_TYPE
};

// Pawn: Strongly encourages central control and advancing, penalizes a/h files slightly.
const int mg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
    -6,    7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};

// Knight: Hates the edges (-50+), loves the center.
const int mg_knight_table[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23, -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12, -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};

// Bishop: Loves the main diagonals and center, hates corners.
const int mg_bishop_table[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};

// Rook: Loves the 7th rank (index 8-15) and central files.
const int mg_rook_table[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};

// Queen: Generally kept safe, slight bonus for centralization.
const int mg_queen_table[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};

// King: Heavily penalizes moving toward the center (Middle-game safety).
const int mg_king_table[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49, -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};


template <Color Us>
int evaluate(Board& board) {
    constexpr Color Them = (Us == WHITE) ? BLACK : WHITE;
    int score = 0;

    for (int p = PAWN; p <= KING; p++) {
        Piece piece = static_cast<Piece>(p);

        // --- Evaluate OUR pieces ---
        uint64_t our_bitboard = board.get_pieces(Us, piece);
        while (our_bitboard) {
            int sq = lsb(our_bitboard);
            assert(sq >= 0 && sq <= 63 && "evaluate: sq out of bounds!");
            our_bitboard &= our_bitboard - 1; // Clear LS1B

            score += PIECE_VALUES[p];
            
            // Flip the square vertically if we are Black!
            int table_sq = (Us == WHITE) ? sq : sq ^ 56;
            assert(table_sq >= 0 && table_sq <= 63 && "evaluate: table_sq out of bounds!");
            
            if (p == PAWN)   score += mg_pawn_table[table_sq];
            if (p == KNIGHT) score += mg_knight_table[table_sq];
            if (p == BISHOP) score += mg_bishop_table[table_sq];
            if (p == ROOK)   score += mg_rook_table[table_sq];
            if (p == QUEEN)  score += mg_queen_table[table_sq];
            if (p == KING)   score += mg_king_table[table_sq];
        }

        // --- Evaluate THEIR pieces ---
        uint64_t their_bitboard = board.get_pieces(Them, piece);
        while (their_bitboard) {
            int sq = lsb(their_bitboard);
            assert(sq >= 0 && sq <= 63 && "evaluate: sq out of bounds!");
            their_bitboard &= their_bitboard - 1;

            score -= PIECE_VALUES[p];
            
            int table_sq = (Them == WHITE) ? sq : sq ^ 56;
            assert(table_sq >= 0 && table_sq <= 63 && "evaluate: table_sq out of bounds!");
            
            if (p == PAWN)   score -= mg_pawn_table[table_sq];
            if (p == KNIGHT) score -= mg_knight_table[table_sq];
            if (p == BISHOP) score -= mg_bishop_table[table_sq];
            if (p == ROOK)   score -= mg_rook_table[table_sq];
            if (p == QUEEN)  score -= mg_queen_table[table_sq];
            if (p == KING)   score -= mg_king_table[table_sq];
        }
    }

    return score;
}