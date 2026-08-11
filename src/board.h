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
    void clear();
    template <Color Us>
    bool make_move(Move move);
    template <Color Us>
    void unmake_move(Move move);

    inline Bitboard get_pieces(Color c, Piece p) const { return pieces[c][p]; }
    inline Bitboard get_color_pieces(Color c) const { return colorPieces[c]; }
    inline Bitboard get_all_pieces() const { return allPieces; }
    inline Color get_side_to_move() const { return sideToMove; }
    inline Bitboard get_en_passant_Bitboard() const { return SquareBB(currentState.En_Passant_Target_Square); }
    inline int get_castling_rights() const { return currentState.castling_rights; }
};


template <Color Us>
bool Board::make_move(Move move){
    HistroyStack[gamePly++] = currentState;
    // (step 0 unpacking the move)
    int from_sq = get_from_sq(move);
    int to_sq = get_to_sq(move);
    int flag = get_flags(move);

    // need to find piecetype
    // update bitboards of board
    // king/rock moves -> update castling rights
    
    // check if move is legal ? (king sq attacked)

    /*switch(flag){
        case 0 : __; break;
        case 1 : __; break;

        case 15 : ; break;
    }*/

    // legality check

    int king_sq = lsb(get_pieces(sideToMove, KING));
    //is_square_attacked<Them>

    return true;
}

template <Color us>
void Board::unmake_move(Move move){
    // (step 0 unpacking the move)
    // step 1 unding the move in the Bitboards (Bit manipulation)

    // step 2 history poping/resotring

    // step 3 zobrist hashing fixing?
}