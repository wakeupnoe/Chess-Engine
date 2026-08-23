#pragma once
// AI Generated

#include "board.h"
#include "bitboard.h"
#include "types.h"

// Standard piece values in centipawns
constexpr int PIECE_VALUES[6] = {
    100, // PAWN
    300, // KNIGHT
    320, // BISHOP (Slightly higher than knight is standard!)
    500, // ROOK
    900, // QUEEN
    0    // KING (King is infinite, evaluated via checkmate logic instead)
};

template <Color Us>
int evaluate(Board& board) {
    constexpr Color Them = (Us == WHITE) ? BLACK : WHITE;
    int score = 0;

    // Loop through all piece types except the King
    for (int p = PAWN; p <= QUEEN; p++) {
        Piece piece = static_cast<Piece>(p);
        
        // Add the value of OUR pieces
        int our_count = popcount(board.get_pieces(Us, piece));
        score += our_count * PIECE_VALUES[p];

        // Subtract the value of THEIR pieces
        int their_count = popcount(board.get_pieces(Them, piece));
        score -= their_count * PIECE_VALUES[p];
    }

    return score;
}