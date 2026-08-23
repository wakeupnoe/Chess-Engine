#pragma once
#include "types.h"
#include "board.h"
#include "attacks.h"

class MoveList {
    private:
    Move moves[256];
    int count;
    public:
    MoveList() : count(0) {}
    
    inline void add(Move m) { moves[count++] = m; }
    
    inline int size() { return count; }
    inline Move get(int index) { return moves[index]; }
    inline const Move* begin() { return &moves[0]; }
    inline const Move* end() { return &moves[count]; }
};

inline Move encode_move(int from, int to, int flags = 0){
    return (flags<<12) | (to<<6) | from;
}

inline void serialize_moves(MoveList& list, int from_sq, Bitboard targets, int flag){
    while(targets){
        int to_sq = pop_lsb(targets);
        list.add(encode_move(from_sq,to_sq,flag));
    }
}

inline void serialize_pawn_moves(MoveList& list, Bitboard targets, int offset, int flag){
    while(targets){
        int to_sq = pop_lsb(targets);
        list.add(encode_move(to_sq - offset,to_sq,flag));
    }
}

inline void serialize_pawn_promotions(MoveList& list, Bitboard targets, int offset, int base_flag){
    while(targets){
        int to_sq = pop_lsb(targets);
        int from_sq = to_sq - offset;
        list.add(encode_move(from_sq,to_sq, base_flag));        // Knight
        list.add(encode_move(from_sq,to_sq, base_flag + 1));    // Bishop
        list.add(encode_move(from_sq,to_sq, base_flag + 2));    // Rook
        list.add(encode_move(from_sq,to_sq, base_flag + 3));    // Queen
    }
}



template <Color Us>
MoveList generate_moves(const Board& board){
    MoveList pseudoLegalMoves = MoveList();
    
    Bitboard empty = ~(board.get_all_pieces());
    Bitboard enemies = board.get_color_pieces(Us == WHITE ? BLACK : WHITE);
    
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    constexpr int Up = (Us == WHITE) ? 8 : -8;
    constexpr int UpLeft = (Us == WHITE) ? 7 : -7; // UpLeft from blacks perspective is -7?? (or rather +7 but offsets are substracted so -7) 
    constexpr int UpRight = (Us == WHITE) ? 9 : -9;

    // pawns
    Bitboard pawns = board.get_pieces(Us, PAWN);
    Bitboard non_promoters = pawns & ~(Us == WHITE ? RankBB[6] : RankBB[1]);
    Bitboard promoters = pawns & (Us == WHITE ? RankBB[6] : RankBB[1]);
    
    Bitboard single_pushes = Us == WHITE ? (non_promoters<<8) & empty : (non_promoters>>8) & empty;
    serialize_pawn_moves(pseudoLegalMoves,single_pushes, Up, 0);
    
    Bitboard double_pushed = Us == WHITE ? (single_pushes<<8) & empty & RankBB[3] : (single_pushes>>8) & empty & RankBB[4];
    serialize_pawn_moves(pseudoLegalMoves,double_pushed, 2*Up, 1);

    Bitboard pawn_captures_left = Us == WHITE ? ((non_promoters << 7) & enemies & ~FileBB[FILE_H]) : (non_promoters >> 7) & enemies & ~FileBB[FILE_A];
    serialize_pawn_moves(pseudoLegalMoves,pawn_captures_left, UpLeft, 4);

    Bitboard pawn_captures_right = Us == WHITE ? ((non_promoters << 9) & enemies & ~FileBB[FILE_A]) : (non_promoters >> 9) & enemies & ~FileBB[FILE_H];
    serialize_pawn_moves(pseudoLegalMoves,pawn_captures_right, UpRight, 4);

    //en passant
    Bitboard target_sq = board.get_en_passant_Bitboard();
    Bitboard ep_pawn_captures_left = Us == WHITE ? ((non_promoters << 7) & target_sq & ~FileBB[FILE_H]) : (non_promoters >> 7) & target_sq & ~FileBB[FILE_A];
    serialize_pawn_moves(pseudoLegalMoves,ep_pawn_captures_left,UpLeft,5);
    Bitboard ep_pawn_captures_right = Us == WHITE ? ((non_promoters << 9) & target_sq & ~FileBB[FILE_A]) : (non_promoters >> 9) & target_sq & ~FileBB[FILE_H];
    serialize_pawn_moves(pseudoLegalMoves,ep_pawn_captures_right,UpRight,5);

    //promotion moves
    Bitboard pro_single_pushes = Us == WHITE ? (promoters<<8) & empty : (promoters>>8) & empty;
    serialize_pawn_promotions(pseudoLegalMoves,pro_single_pushes, Up, 8);
    Bitboard pro_capture_left = Us == WHITE ? (promoters<<7) & enemies & ~FileBB[FILE_H] : (promoters>>7) & enemies & ~FileBB[FILE_A];
    serialize_pawn_promotions(pseudoLegalMoves,pro_capture_left, UpLeft, 12);
    Bitboard pro_capture_right = Us == WHITE ? (promoters<<9) & enemies & ~FileBB[FILE_A]: (promoters>>9) & enemies & ~FileBB[FILE_H];
    serialize_pawn_promotions(pseudoLegalMoves,pro_capture_right, UpRight, 12);

    // king
    int king_sq = lsb(board.get_pieces(Us, KING));
    Bitboard king_attacks = KingAttacks[king_sq];
    Bitboard quiets = king_attacks & empty;
    serialize_moves(pseudoLegalMoves,king_sq,quiets,0);
    Bitboard captures = king_attacks & enemies;
    serialize_moves(pseudoLegalMoves,king_sq,captures,4);

    //rook
    Bitboard rooks = board.get_pieces(Us, ROOK);
    while(rooks){
        int from_sq = pop_lsb(rooks);
        Bitboard rook_attacks = get_rook_attacks(static_cast<Square>(from_sq),board.get_all_pieces());
        Bitboard quiets = rook_attacks & empty;
        Bitboard captures = rook_attacks & enemies;
        serialize_moves(pseudoLegalMoves,from_sq,quiets,0);
        serialize_moves(pseudoLegalMoves,from_sq,captures,4);
    }
    
    // bishop
    Bitboard bishops = board.get_pieces(Us, BISHOP);
    while(bishops){
        int from_sq = pop_lsb(bishops);
        Bitboard bishop_attacks = get_bishop_attacks(static_cast<Square>(from_sq),board.get_all_pieces());
        Bitboard quiets = bishop_attacks & empty;
        Bitboard captures = bishop_attacks & enemies;
        serialize_moves(pseudoLegalMoves,from_sq,quiets,0);
        serialize_moves(pseudoLegalMoves,from_sq,captures,4);
    }

    // queen
    Bitboard queens = board.get_pieces(Us, QUEEN);
    while(queens){
        int from_sq = pop_lsb(queens);
        Bitboard queen_attacks = get_queen_attacks(static_cast<Square>(from_sq),board.get_all_pieces());
        Bitboard quiets = queen_attacks & empty;
        Bitboard captures = queen_attacks & enemies;
        serialize_moves(pseudoLegalMoves,from_sq,quiets,0);
        serialize_moves(pseudoLegalMoves,from_sq,captures,4);
    }

    // knight
    Bitboard knights = board.get_pieces(Us, KNIGHT);
    while(knights){
        int from_sq = pop_lsb(knights);
        Bitboard knight_attacks = KnightAttacks[from_sq];
        Bitboard quiets = knight_attacks & empty;
        Bitboard captures = knight_attacks & enemies;
        serialize_moves(pseudoLegalMoves,from_sq,quiets,0);
        serialize_moves(pseudoLegalMoves,from_sq,captures,4);
    }

    // castling
    constexpr int Castling_Right_KS = (Us == WHITE) ? 1 : 4;
    constexpr int Castling_Right_QS = (Us == WHITE) ? 2 : 8;
    
    constexpr Bitboard KS_empty = (Us == WHITE) ? 0x60ULL : 0x6000000000000000ULL;
    constexpr Bitboard QS_empty = (Us == WHITE) ? 0x0EULL : 0x0E00000000000000ULL;
    
    constexpr int KingStartSq = (Us == WHITE) ? 4 : 60;
    constexpr int KS_TargetSq = (Us == WHITE) ? 6 : 62;
    constexpr int QS_TargetSq = (Us == WHITE) ? 2 : 58;
    
    int rights = board.get_castling_rights();
    
    if((rights & Castling_Right_KS) && (KS_empty == (empty & KS_empty))){
        if(!is_square_attacked<Them>(static_cast<Square>(KingStartSq),board) && !is_square_attacked<Them>(static_cast<Square>(KingStartSq + 1),board)){
            pseudoLegalMoves.add(encode_move(KingStartSq,KS_TargetSq,2));
        }
    }
    if((rights & Castling_Right_QS) && (QS_empty == (empty & QS_empty))){
        if(!is_square_attacked<Them>(static_cast<Square>(KingStartSq),board) && !is_square_attacked<Them>(static_cast<Square>(KingStartSq -1),board)){
            pseudoLegalMoves.add(encode_move(KingStartSq,QS_TargetSq,3));
        }
    }

    return pseudoLegalMoves;
}