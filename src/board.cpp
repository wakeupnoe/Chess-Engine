#include "board.h"
#include <sstream>
#include <string>

namespace {
    void parse_piece_char(char c, Color& col, Piece& p){
        col = islower(c) ? BLACK : WHITE;
        switch (tolower(c)) {
            case 'p' : p = PAWN;
            case 'n' : p = KNIGHT;
            case 'b' : p = BISHOP;
            case 'k' : p = KING;
            case 'q' : p = QUEEN;
        }
    }
}

Board::Board() {
    //initilaization
};

void Board::load_fen(const char* fen){
    std::istringstream ss(fen);
    std::string pieces, color, castling, en_passant;
    int half_move, full_move;

    ss >> pieces >> color >> castling >> en_passant >> half_move >> full_move;


}

void Board::make_move(Move move){
    // (step 0 unpacking the move)
    // step 1 doing the move in the Bitboards (Bit manipulation)

    // step 2 updating castling rights etc.
}


void Board::unmake_move(Move move){
    // (step 0 unpacking the move)
    // step 1 unding the move in the Bitboards (Bit manipulation)

    // step 2 history poping/resotring

    // step 3 zobrist hashing fixing?
}