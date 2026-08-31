#include "see.h"
#include "attacks.h"

namespace
{
    Bitboard get_dynamic_attackers(int to, Bitboard occ, const Board& board){
        Square sq = static_cast<Square>(to);
        Bitboard attackers = 0ULL;
        
        Bitboard knights = board.get_pieces(WHITE, KNIGHT) | board.get_pieces(BLACK, KNIGHT);
        attackers |= get_knight_attacks(sq, knights);
        Bitboard kings = board.get_pieces(WHITE, KING) | board.get_pieces(BLACK, KING);
        attackers |= get_king_attacks(sq, kings);

        attackers |= get_pawn_attacks<BLACK>(sq, board.get_pieces(WHITE, PAWN));
        attackers |= get_pawn_attacks<WHITE>(sq, board.get_pieces(BLACK, PAWN));

        Bitboard bq = board.get_pieces(WHITE,BISHOP) | board.get_pieces(BLACK,BISHOP) | board.get_pieces(WHITE,QUEEN) | board.get_pieces(BLACK,QUEEN);
        attackers |= get_bishop_attacks(sq, occ) & bq;

        Bitboard rq = board.get_pieces(WHITE,ROOK) | board.get_pieces(BLACK,ROOK) | board.get_pieces(WHITE,QUEEN) | board.get_pieces(BLACK,QUEEN);
        attackers |= get_rook_attacks(sq, occ) & rq;

        return attackers;
    }
}



constexpr int PIECE_VALUES[6] = {
    100, // PAWN
    300, // KNIGHT
    320, // BISHOP
    500, // ROOK
    900, // QUEEN
    30000 // KING
};

int static_exchange_evaluation(const Board& board, Move move){
    int from = get_from_sq(move);
    int to = get_to_sq(move);
    int flag = get_flags(move);

    Piece attacker = board.get_piece_on_sq(from);
    Piece victim = board.get_piece_on_sq(to);

    if(flag == 5) victim = PAWN; // en passant
    int promo_bonus = 0;
    if(flag >= 8){
        Piece promoted = static_cast<Piece>(KNIGHT + (3 & flag));
        promo_bonus = PIECE_VALUES[promoted]-PIECE_VALUES[PAWN];
        attacker = promoted; // promotion
    }

    int gain[32];
    int d = 0;
    gain[0] = PIECE_VALUES[victim] + promo_bonus;

    Bitboard occ = board.get_all_pieces();
    occ ^= SquareBB(from);

    Bitboard attackers = get_dynamic_attackers(to, occ, board);

    Color next_player = board.get_side_to_move() == WHITE ? BLACK : WHITE;

    while(true) {
        d++;
        gain[d] = PIECE_VALUES[attacker] - gain[d-1];

        if(std::max(-gain[d-1], gain[d]) < 0) break; // early exit if potential is too low to compenesate for losses

        Bitboard our_attackers = attackers & board.get_color_pieces(next_player);
        if(our_attackers == 0ULL) break; // no more attackers

        int next_from = -1;
        for(int p = PAWN; p <= KING; p++){
            Bitboard overlap = our_attackers & board.get_pieces(next_player, static_cast<Piece>(p));
            if(overlap){
                next_from = lsb(overlap);
                attacker = static_cast<Piece>(p);
                break;
            }
        }

        if(attacker == KING){
            Bitboard opponent_attackers = attackers & board.get_color_pieces(next_player == WHITE ? BLACK : WHITE);
            if(opponent_attackers) break;
        }

        occ ^= SquareBB(next_from);

        attackers = get_dynamic_attackers(to, occ, board);

        attackers &= occ;

        next_player = next_player == WHITE ? BLACK : WHITE;
    }

    while(--d > 0){
        gain[d-1] = -std::max(-gain[d-1], gain[d]);
    }

    return gain[0];
}