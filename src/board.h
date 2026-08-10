#pragma once
#include "bitboard.h"
#include "types.h"
#include <array>

const int MAX_GAME_PLIES = 1024;


class Board {
private:
    Bitboard pieces[2][6]; // pieces[COLOR][PIECE]
    Bitboard colorPieces[2]; // colorPieces[COLOR]
    Bitboard allPieces;
    StateInfo currentState;
    Color sideToMove;
    std::array<StateInfo, MAX_GAME_PLIES> HistroyStack;
    int gamePly;

public:
    Board();

    void load_fen(const char* fen);
    void make_move(Move move);
    void unmake_move(Move move);
    void clear();

    inline Bitboard get_pieces(Color c, Piece p) const { return pieces[c][p]; }
    inline Bitboard get_color_pieces(Color c) const { return colorPieces[c]; }
    inline Bitboard get_all_pieces() const { return allPieces; }
    inline Color get_side_to_move() const { return sideToMove; }
};

