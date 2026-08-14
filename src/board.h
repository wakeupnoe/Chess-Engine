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
    Piece pieceOnSquare[64];
    Color sideToMove;
    std::array<StateInfo, MAX_GAME_PLIES> HistoryStack;
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
    inline void move_piece(Color c, Piece p, Square from_sq, Square to_sq) { // this is still faster than calling toggle pieces two times, no?
        Bitboard move_mask = SquareBB(from_sq) | SquareBB(to_sq);
        pieces[c][p] ^= move_mask;
        colorPieces[c] ^= move_mask;
        allPieces ^= move_mask;
        pieceOnSquare[to_sq] = p; 
        pieceOnSquare[from_sq] = NO_PIECE_TYPE;
    }
    inline void toggle_piece(Color c, Piece p, Square sq) {
        Bitboard bb = SquareBB(sq);
        pieces[c][p] ^= bb;
        colorPieces[c] ^= bb;
        allPieces ^= bb;
    }
    inline void add_piece(Color c, Piece p, Square sq){ toggle_piece(c,p,sq); pieceOnSquare[sq] = p; }
    inline void remove_piece(Color c, Piece p, Square sq){ toggle_piece(c,p,sq); pieceOnSquare[sq] = NO_PIECE_TYPE; }

    inline Color get_side_to_move() const { return sideToMove; }
    inline Bitboard get_en_passant_Bitboard() const { return SquareBB(currentState.En_Passant_Target_Square); }
    inline int get_castling_rights() const { return currentState.castling_rights; }
};


template <Color Us> // of the side that makes the move
bool Board::make_move(Move move){
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    HistoryStack[gamePly++] = currentState;
    int from_sq = get_from_sq(move);
    int to_sq = get_to_sq(move);
    int flag = get_flags(move);
    Piece pieceType = pieceOnSquare[from_sq];
    Piece capturedType = pieceOnSquare[to_sq];
    
    if(capturedType!=NO_PIECE_TYPE){
        remove_piece(Them,capturedType,to_sq);
    }
    currentState.Captured_Piece_Type = capturedType;

    move_piece(Us,pieceType,from_sq,to_sq);

    //handle flags
    currentState.Half_move_Clock++;
    currentState.En_Passant_Target_Square = NO_SQUARE;
    if(flag){
        if(flag & 8){ // promotion
            remove_piece(Us,PAWN,to_sq);
            Piece promoted_to = static_cast<Piece>(KNIGHT + (flag & 3)); 
            add_piece(Us,promoted_to,to_sq); // last 3 bits? ROOK, BISHOP, KNIGHT, QUEEN (in some order)  (if we ordered Pieces correctly and matched with moveencoding)
        }
        
        if(flag == 5){ // enpassant
            int enemy_pawn_sq = to_sq + (Us == WHITE ? -8 : 8);
            remove_piece(Them,PAWN,enemy_pawn_sq);
            currentState.Captured_Piece_Type = PAWN;
        }

        if(flag == 1){ // double push
            currentState.En_Passant_Target_Square = to_sq + (Us == WHITE ? -8 : 8);
        }

        if(flag == 2){ // KING SIDE
            Square from_rook = Us == WHITE ? H1 : H8;
            Square to_rook = static_cast<Square>(from_rook - 2);
            move_piece(Us,ROOK,from_rook,to_rook);
        }

        if(flag == 3){ // QUEEN SIDE
            Square from_rook = Us == WHITE ? A1 : A8;
            Square to_rook = static_cast<Square>(from_rook + 3);
            move_piece(Us,ROOK,from_rook,to_rook);
        }
    }
    currentState.castling_rights &= CASTLING_MASK[from_sq] & CASTLING_MASK[to_sq];


    // legality check
    int king_sq = lsb(get_pieces(Us, KING));
    if(is_square_attacked<Them>(king_sq, *this)) {
        unmake_move<Us>(move);
        return false;
    }
    return true;
}

template <Color us>
void Board::unmake_move(Move move){
    // (step 0 unpacking the move)
    // step 1 unding the move in the Bitboards (Bit manipulation)

    // step 2 history poping/resotring

    // step 3 zobrist hashing fixing?
}