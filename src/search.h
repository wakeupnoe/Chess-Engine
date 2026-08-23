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

template <Color Us>
void score_moves(MoveList& list, const Board& board){
    constexpr Color Them = Us == WHITE ? BLACK : WHITE; 
    for(int i = 0; i<list.size(); i++){
        Move move = list.get(i);
        int score = 0;
        int flag = get_flags(move);
        int from = get_from_sq(move);
        int to = get_to_sq(move);

        // 1. TT Score: 10,000,000

        // 2. Good and Equal Captures & Queen Promotions (Score: 1,000,000 to 2,000,000)

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

    score_moves<Us>(list, board); // arguments incomplete todo

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