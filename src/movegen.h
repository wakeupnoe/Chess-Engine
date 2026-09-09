#pragma once
#include "types.h"
#include "board.h"
#include "attacks.h"
#include <cmath>
#include <iostream>

// CONTRACT [Move Generation Utilities]:
// - ASSUMES: A chess position will never generate more than 256 moves (theoretical max is 218).
// - ASSUMES: 'from', 'to', and 'flags' passed to encode_move are strictly within bounds.
// - GUARANTEES: Buffer overflow protection for the MoveList and bit-leak protection during move encoding.

class MoveList {
    private:
    int count;
    
    public:
    Move moves[256]; // bad practice to make this public, but OOP ommited in favor of readability in search.h
    int scores[256];
    
    MoveList() : count(0) {}
    
    inline void add(Move m) { assert(count < 256 && "MoveList overflow: too many moves generated!"); moves[count] = m; scores[count++] = m; }
    inline int size() { return count; }
    inline Move get(int index) { assert(index >= 0 && index < count && "MoveList: Index out of bounds!"); return moves[index]; }
    inline const Move* begin() { return &moves[0]; }
    inline const Move* end() { return &moves[count]; }
};

inline Move encode_move(int from, int to, int flags = 0){
    assert(from >= 0 && from <= 63 && "encode_move: from_sq out of bounds!");
    assert(to >= 0 && to <= 63 && "encode_move: to_sq out of bounds!");
    assert(flags >= 0 && flags <= 15 && "encode_move: flag out of bounds!");
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

// CONTRACT [Pseudo-Legality Verification]:
// - ASSUMES: move is a properly encoded 16-bit integer (0 is safely handled).
// - GUARANTEES: Returns true ONLY if the move physically exists according to piece geometry.
// - GUARANTEES: Filters out friendly-fire, empty-square moves, and invalid piece-flag combinations.

template <Color Us>
bool is_pseudo_legal(const Board& board, Move move){
    if(move == 0) return false;

    int from = get_from_sq(move);
    int to = get_to_sq(move);
    int flag = get_flags(move);
    
    Piece p = board.get_piece_on_sq(static_cast<Square>(from));
    Piece target_p = board.get_piece_on_sq(static_cast<Square>(to));
    
    if(p==NO_PIECE_TYPE) return false;
    if(!(board.get_color_pieces(Us) & SquareBB(from))) return false; // move our piece?
    assert(p >= PAWN && p <= KING && "is_pseudo_legal: colorPieces desynced from pieceOnSquare!");
    if(board.get_color_pieces(Us) & SquareBB(to)) return false; // capture their piece?

    bool is_capture_sq = (target_p) != NO_PIECE_TYPE;

    if(flag >= 8){
        if(p!=PAWN) return false;
        if(Us == WHITE && (to/8)!=7) return false;
        if(Us == BLACK && (to/8)!=0) return false;
        if(is_capture_sq && flag < 12) return false;
        if(!is_capture_sq && flag >= 12) return false;
    } else if(flag == 5){
        if(p!=PAWN) return false;
        if(SquareBB(to) != board.get_en_passant_Bitboard()) return false;
        if(is_capture_sq) return false; // there is a piece on the target
        int enemy_pawn_sq = to + (Us == WHITE ? -8 : 8);
        Piece victim = board.get_piece_on_sq(static_cast<Square>(enemy_pawn_sq));
        if(victim != PAWN){
            std::cout << "New Check cought something!" << std::endl;
            return false;
        }
        if(!(board.get_color_pieces(Us == WHITE ? BLACK : WHITE) & SquareBB(enemy_pawn_sq))){
            std::cout << "New Check 2 cought something!" << std::endl;
            return false;
        }
    } else if(flag == 4 && !is_capture_sq) return false;
    else if(flag<=3 && is_capture_sq) return false;
    if(flag == 1) {
        if(p != PAWN || to != (from + (Us == WHITE ? 16 : -16))) {
            return false;
        }
    }

    if((flag == 2 || flag == 3) && p!=KING) return false;

    Bitboard occ = board.get_all_pieces();
    Bitboard to_bb = SquareBB(to);

    switch(p){
        case KNIGHT : return KnightAttacks[from] & to_bb;
        case BISHOP : return get_bishop_attacks(static_cast<Square>(from),occ) & to_bb;
        case ROOK : return get_rook_attacks(static_cast<Square>(from),occ) & to_bb;
        case QUEEN : return get_queen_attacks(static_cast<Square>(from),occ) & to_bb;
        case KING : {
            if(flag == 2 || flag == 3) {
                // STRICT ORIGIN: King must start on its home square
                if (from != (Us == WHITE ? E1 : E8)) return false;
                
                if(flag == 2){ // King Side
                    if (!(board.get_castling_rights() & (Us == WHITE ? WK : BK))) return false;

                    if(to != (Us == WHITE ? G1 : G8)) return false;
                    Bitboard path = Us == WHITE ? (SquareBB(F1) | SquareBB(G1)) : (SquareBB(F8) | SquareBB(G8));
                    if(occ & path) return false;
                    // STRICT ROOK EXISTENCE
                    Square rook_sq = Us == WHITE ? H1 : H8;
                    if(board.get_piece_on_sq(rook_sq) != ROOK) return false;
                    if(!(board.get_color_pieces(Us) & SquareBB(rook_sq))) return false;
                    return true;
                }
                if(flag == 3){ // Queen Side
                    if (!(board.get_castling_rights() & (Us == WHITE ? WQ : BQ))) return false;
                    if(to != (Us == WHITE ? C1 : C8)) return false;
                    Bitboard path = Us == WHITE ? (SquareBB(D1) | SquareBB(C1) | SquareBB(B1)) : (SquareBB(D8) | SquareBB(C8) | SquareBB(B8));
                    if(occ & path) return false;
                    // STRICT ROOK EXISTENCE
                    Square rook_sq = Us == WHITE ? A1 : A8;
                    if(board.get_piece_on_sq(rook_sq) != ROOK) return false;
                    if(!(board.get_color_pieces(Us) & SquareBB(rook_sq))) return false;
                    return true;
                }
            }
            return KingAttacks[from] & to_bb;
        }
        case PAWN : {
            constexpr int Up = Us == WHITE ? 8 : -8;
            // single push
            if(to == from + Up && !(occ & to_bb)){
                return flag == 0;
            }
            // double push
            if(to == from + Up + Up && !(occ & SquareBB(from + Up)) && !(occ & to_bb)){
                if(flag != 1) return false;
                return Us == WHITE ? from / 8 == 1 : from / 8 == 6;
            }

            //captures
            constexpr int UpLeft = (Us == WHITE) ? 7 : -7;
            constexpr int UpRight = (Us == WHITE) ? 9 : -9;
            if(to == from + UpLeft || to == from + UpRight){
                if(std::abs((from % 8) - (to % 8)) == 1){
                    if(board.get_en_passant_Bitboard() & to_bb) return flag == 5;
                    if(board.get_color_pieces(Us == WHITE ? BLACK : WHITE) & to_bb) return flag == 4 || flag >= 8;
                }
            }
            return false;
        }
        default : return false;
    }
}



template <Color Us>
MoveList generate_captures(const Board& board){
    MoveList pseudoLegalMoves = MoveList();
    Bitboard empty = ~(board.get_all_pieces());
    Bitboard enemies = board.get_color_pieces(Us == WHITE ? BLACK : WHITE);
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    
    constexpr int Up = (Us == WHITE) ? 8 : -8;
    constexpr int UpLeft = (Us == WHITE) ? 7 : -7; // UpLeft from blacks perspective is -7?? (or rather +7 but offsets are substracted so -7) 
    constexpr int UpRight = (Us == WHITE) ? 9 : -9;

    //PAWNS
    Bitboard pawns = board.get_pieces(Us, PAWN);
    Bitboard non_promoters = pawns & ~(Us == WHITE ? RankBB[6] : RankBB[1]);
    Bitboard promoters = pawns & (Us == WHITE ? RankBB[6] : RankBB[1]);
    
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

    // (capture) promotions
    Bitboard pro_capture_left = Us == WHITE ? (promoters<<7) & enemies & ~FileBB[FILE_H] : (promoters>>7) & enemies & ~FileBB[FILE_A];
    serialize_pawn_promotions(pseudoLegalMoves,pro_capture_left, UpLeft, 12);
    Bitboard pro_capture_right = Us == WHITE ? (promoters<<9) & enemies & ~FileBB[FILE_A]: (promoters>>9) & enemies & ~FileBB[FILE_H];
    serialize_pawn_promotions(pseudoLegalMoves,pro_capture_right, UpRight, 12);
    
    // (quiet) promotions STILL IN CAPTURES, since very noisy
    Bitboard pro_single_pushes = Us == WHITE ? (promoters<<8) & empty : (promoters>>8) & empty;
    serialize_pawn_promotions(pseudoLegalMoves,pro_single_pushes, Up, 8);


    assert(board.get_pieces(Us, KING) != 0ULL && "generate_captures: Board has no King!");
    // KING
    int king_sq = lsb(board.get_pieces(Us, KING));
    Bitboard king_attacks = KingAttacks[king_sq];
    Bitboard captures = king_attacks & enemies;
    serialize_moves(pseudoLegalMoves,king_sq,captures,4);


    // ROCK
    Bitboard rooks = board.get_pieces(Us, ROOK);
    while(rooks){
        int from_sq = pop_lsb(rooks);
        Bitboard rook_attacks = get_rook_attacks(static_cast<Square>(from_sq),board.get_all_pieces());
        Bitboard captures = rook_attacks & enemies;
        serialize_moves(pseudoLegalMoves,from_sq,captures,4);
    }

    // BISHOP
    Bitboard bishops = board.get_pieces(Us, BISHOP);
    while(bishops){
        int from_sq = pop_lsb(bishops);
        Bitboard bishop_attacks = get_bishop_attacks(static_cast<Square>(from_sq),board.get_all_pieces());
        Bitboard captures = bishop_attacks & enemies;
        serialize_moves(pseudoLegalMoves,from_sq,captures,4);
    }

    // QUEEN
    Bitboard queens = board.get_pieces(Us, QUEEN);
    while(queens){
        int from_sq = pop_lsb(queens);
        Bitboard queen_attacks = get_queen_attacks(static_cast<Square>(from_sq),board.get_all_pieces());
        Bitboard captures = queen_attacks & enemies;
        serialize_moves(pseudoLegalMoves,from_sq,captures,4);
    }

    // KNIGHT
    Bitboard knights = board.get_pieces(Us, KNIGHT);
    while(knights){
        int from_sq = pop_lsb(knights);
        Bitboard knight_attacks = KnightAttacks[from_sq];
        Bitboard captures = knight_attacks & enemies;
        serialize_moves(pseudoLegalMoves,from_sq,captures,4);
    }

    return pseudoLegalMoves;
}

template <Color Us>
MoveList generate_quiets(const Board& board){
    MoveList pseudoLegalMoves = MoveList();
    Bitboard empty = ~(board.get_all_pieces());
    //Bitboard enemies = board.get_color_pieces(Us == WHITE ? BLACK : WHITE);
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    constexpr int Up = (Us == WHITE) ? 8 : -8;


    // PAWNS
    Bitboard pawns = board.get_pieces(Us, PAWN);
    Bitboard non_promoters = pawns & ~(Us == WHITE ? RankBB[6] : RankBB[1]);
    Bitboard promoters = pawns & (Us == WHITE ? RankBB[6] : RankBB[1]);
    
    Bitboard single_pushes = Us == WHITE ? (non_promoters<<8) & empty : (non_promoters>>8) & empty;
    serialize_pawn_moves(pseudoLegalMoves,single_pushes, Up, 0);
    
    Bitboard double_pushed = Us == WHITE ? (single_pushes<<8) & empty & RankBB[3] : (single_pushes>>8) & empty & RankBB[4];
    serialize_pawn_moves(pseudoLegalMoves,double_pushed, 2*Up, 1);

    assert(board.get_pieces(Us, KING) != 0ULL && "generate_quiets: Board has no King!");
    // KING
    int king_sq = lsb(board.get_pieces(Us, KING));
    Bitboard king_attacks = KingAttacks[king_sq];
    Bitboard quiets = king_attacks & empty;
    serialize_moves(pseudoLegalMoves,king_sq,quiets,0);

    // ROOK
    Bitboard rooks = board.get_pieces(Us, ROOK);
    while(rooks){
        int from_sq = pop_lsb(rooks);
        Bitboard rook_attacks = get_rook_attacks(static_cast<Square>(from_sq),board.get_all_pieces());
        Bitboard quiets = rook_attacks & empty;
        serialize_moves(pseudoLegalMoves,from_sq,quiets,0);
    }


    // BISHOP
    Bitboard bishops = board.get_pieces(Us, BISHOP);
    while(bishops){
        int from_sq = pop_lsb(bishops);
        Bitboard bishop_attacks = get_bishop_attacks(static_cast<Square>(from_sq),board.get_all_pieces());
        Bitboard quiets = bishop_attacks & empty;
        serialize_moves(pseudoLegalMoves,from_sq,quiets,0);
    }


    // QUEEN
    Bitboard queens = board.get_pieces(Us, QUEEN);
    while(queens){
        int from_sq = pop_lsb(queens);
        Bitboard queen_attacks = get_queen_attacks(static_cast<Square>(from_sq),board.get_all_pieces());
        Bitboard quiets = queen_attacks & empty;
        serialize_moves(pseudoLegalMoves,from_sq,quiets,0);
    }


    // KNIGHT
    Bitboard knights = board.get_pieces(Us, KNIGHT);
    while(knights){
        int from_sq = pop_lsb(knights);
        Bitboard knight_attacks = KnightAttacks[from_sq];
        Bitboard quiets = knight_attacks & empty;
        serialize_moves(pseudoLegalMoves,from_sq,quiets,0);
    }

    // CASTLING
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

template <Color Us>
MoveList generate_moves(const Board& board){
    MoveList pseudoLegalMoves = MoveList();
    MoveList captures =  generate_captures<Us>(board);
    for(int i = 0; i<captures.size(); i++){
        pseudoLegalMoves.add(captures.get(i));
    }
    MoveList quiets =  generate_quiets<Us>(board);
    for(int i = 0; i<quiets.size(); i++){
        pseudoLegalMoves.add(quiets.get(i));
    }

    return pseudoLegalMoves;
}