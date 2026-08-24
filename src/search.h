#pragma once
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "evaluation.h"

constexpr int INFINITY_SCORE = 999999;
constexpr int MATE_SCORE = 900000;

template <Color Us>
Move get_best_move(Board& board, int depth){
    constexpr Color Them = Us == WHITE ? BLACK : WHITE; 
    Move bestMove = 0;
    int max_score = -INFINITY_SCORE;
    int alpha = -INFINITY_SCORE;
    int beta = INFINITY_SCORE;

    MoveList list = generate_moves<Us>(board);
    for(int i = 0; i<list.size(); i++){
        Move move = list.get(i);
        if(board.make_move<Us>(move)){
            int score = -search<Them>(board, depth-1, -beta, -alpha, 1);
            board.unmake_move<Us>(move);

            if(score > max_score || bestMove == 0){
                bestMove = move;
                max_score = score;
            }
            alpha = std::max(alpha, max_score);
        }
    }
    return bestMove;
}

const int MVV_LVT[6][6] = { // AI Generated!
    // Attacker: P,   N,   B,   R,   Q,   K
    { 15,  14,  13,  12,  11,  10 }, // Victim: Pawn (0)
    { 25,  24,  23,  22,  21,  20 }, // Victim: Knight (1)
    { 35,  34,  33,  32,  31,  30 }, // Victim: Bishop (2)
    { 45,  44,  43,  42,  41,  40 }, // Victim: Rook (3)
    { 55,  54,  53,  52,  51,  50 }, // Victim: Queen (4)
    { 0,   0,   0,   0,   0,   0  }  // Victim: King (5) - Kings aren't captured
};

const int FLAG_BONUS[16] = { // AI Generated!
    0,         // 0: Quiet
    0,         // 1: Double Push
    0,         // 2: King Castle
    0,         // 3: Queen Castle
    1'000'000, // 4: Capture
    1'000'000, // 5: EP Capture
    0, 0,      // 6-7: Unused
    0,         // 8: Promo N
    0,         // 9: Promo B
    0,         // 10: Promo R
    900'000,   // 11: Promo Q (Almost as good as a capture!)
    1'000'000, // 12: Promo N Cap
    1'000'000, // 13: Promo B Cap
    1'000'000, // 14: Promo R Cap
    1'900'000  // 15: Promo Q Cap (Capture + Queen Promo!)
};

template <Color Us>
void score_moves(MoveList& list, const Board& board, int ply, Move tt_move){
    constexpr Color Them = Us == WHITE ? BLACK : WHITE; 
    for(int i = 0; i<list.size(); i++){
        Move move = list.get(i);
        int score = 0;
        int flag = get_flags(move);
        int from = get_from_sq(move);
        int to = get_to_sq(move);

        
        // 1. TT Score: 10,000,000
        if(move == tt_move){
            list.scores[i] = 10'000'000;
            continue;
        }


        // 2. Good and Equal Captures & Queen Promotions (Score: 1,000,000 to 2,000,000)
        bool is_capture = flag == 4 || flag == 5 || flag >= 12;
        bool is_queen_promo = flag == 11;
        if(is_capture || is_queen_promo){ // else if TODO
            Piece attacker = board.get_piece_on_sq(static_cast<Square>(from));
            Piece victim = board.get_piece_on_sq(static_cast<Square>(to));

            victim = (flag == 5 ? PAWN : victim);
            list.scores[i] = FLAG_BONUS[flag] + MVV_LVT[victim][attacker];
        }


        // 3. Killer Moves (Score: 900,000)

        // 4. The History Heuristic (Score: 0 to ~800,000)

        // 5. Bad Captures (Score: Negative)

        list.scores[i] = score;
    }
}

template <Color Us>
int search(Board& board, int depth, int alpha, int beta, int ply){
    if(board.is_draw()) return 0; // "ply > 0 &&" important if we remove root search function
    
    if(depth == 0){
        return evaluate<Us>(board); // in evaluation.h
    }
    
    constexpr Color Them = Us == WHITE ? BLACK : WHITE; 
    int max_score = -INFINITY_SCORE;
    int legal_moves = 0;
    
    MoveList list = generate_moves<Us>(board);
    Move tt_move = 0;

    score_moves<Us>(list, board, ply, tt_move); // arguments incomplete todo

    for(int i = 0; i<list.size(); i++){
        Move move = list.get(i);
        if(board.make_move<Us>(move)){
            legal_moves++;
            int score = -search<Them>(board, depth-1, -beta, -alpha, ply + 1);
            board.unmake_move<Us>(move);
            max_score = std::max(score, max_score);
            alpha = std::max(alpha, max_score);
            
            if(beta <= alpha){
                break;
            }
        }
    }
    if(legal_moves == 0){
        int kingSq = lsb(board.get_pieces(Us,KING));
        if(is_square_attacked<Them>(static_cast<Square>(kingSq), board)){
            return - MATE_SCORE + ply; //checkmate
        } else {
            return 0; //Stalemate
        }
    }
    
    return max_score;
}