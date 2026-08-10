#include "movegen.h"
#include "board.h"
#include "attacks.h"


MoveList generate_moves(const Board& board){
    MoveList pseudoLegalMoves = MoveList();
    Color col = board.get_side_to_move();
    
    Bitboard empty = ~(board.get_all_pieces());
    Bitboard enemies = board.get_color_pieces(col == WHITE ? BLACK : WHITE);

    // pawns
    Bitboard pawns = board.get_pieces(col, PAWN);
    Bitboard pushed = col == WHITE ? (pawns<<8) & empty : (pawns>>8) & empty;
    Bitboard doublepushed = WHITE ? (pushed<<8) & empty & RankBB[6] : (pushed>>8) & empty & RankBB[4];
    while(pushed) {
        int to_sq = lsb(pushed);
        pop_lsb(pushed);
        int from_sq = col == WHITE ? to_sq -8 : to_sq +8;
        pseudoLegalMoves.add(encode_move(from_sq,to_sq,0));
    }
    while(doublepushed) {
        int to_sq = lsb(pushed);
        pop_lsb(pushed);
        int from_sq = col == WHITE ? to_sq -16 : to_sq +16;
        pseudoLegalMoves.add(encode_move(from_sq,to_sq,4)); //En-Passant Capture, Promotion Captures
    }
    while(pawns){
        Square from_sq = lsb(pawns);
        pop_lsb(pawns);
        Bitboard pawn_attacks = get_pawn_attacks(from_sq,board.get_all_pieces());
        Bitboard captures = pawn_attacks & enemies;
        while(captures){
            int to_sq = lsb(captures);
            pop_lsb(captures);
            pseudoLegalMoves.add(encode_move(from_sq,to_sq,4));
        }
    }


    // king
    int king_sq = lsb(board.get_pieces(col, KING));
    Bitboard king_attacks = KingAttacks[king_sq]; // ~(board.get_color_pieces(col)) &
    
    Bitboard captures = king_attacks & enemies;
    while(captures){
        int to_sq = lsb(captures);
        pop_lsb(captures);
        pseudoLegalMoves.add(encode_move(king_sq,to_sq,4));
    }
    Bitboard quiets = king_attacks & empty;
    while(quiets){
        int to_sq = lsb(quiets);
        pop_lsb(quiets);
        pseudoLegalMoves.add(encode_move(king_sq,to_sq,0));
    }

    //rook
    Bitboard rooks = board.get_pieces(col, ROOK);
    while(rooks){
        Square from_sq = lsb(rooks);
        pop_lsb(rooks);
        Bitboard rook_attacks = get_rook_attacks(from_sq,board.get_all_pieces());
        Bitboard quiets = rook_attacks & empty;
        while(quiets){
            int to_sq = lsb(quiets);
            pop_lsb(quiets);
            pseudoLegalMoves.add(encode_move(from_sq,to_sq,0));
        }
        Bitboard captures = rook_attacks & enemies;
        while(captures){
            int to_sq = lsb(captures);
            pop_lsb(captures);
            pseudoLegalMoves.add(encode_move(from_sq,to_sq,4));
        }
    }
    
    // bishop, queen

    // knight

    // castling

    return pseudoLegalMoves;
}