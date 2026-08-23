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

    MoveList list = generate_moves<Us>(board);
    for(int i = 0; i<list.size(); i++){
        Move move = list.get(i);
        if(board.make_move<Us>(move)){
            int score = -search<Them>(board, depth-1);
            board.unmake_move<Us>(move);

            if(score > max_score || bestMove == 0){
                bestMove = move;
                max_score = score;
            }
        }
    }
    
    return bestMove;
}

template <Color Us>
int search(Board& board, int depth){
    constexpr Color Them = Us == WHITE ? BLACK : WHITE; 

    if(depth == 0){
        return evaluate<Us>(board); // in evaluation.h
    }
    
    int max_score = -INFINITY_SCORE;
    int legal_moves = 0;

    MoveList list = generate_moves<Us>(board);
    for(int i = 0; i<list.size(); i++){
        Move move = list.get(i);
        if(board.make_move<Us>(move)){
            legal_moves++;
            int score = -search<Them>(board, depth-1);
            board.unmake_move<Us>(move);
            if(score > max_score){
                max_score = score;
            }
        }
    }
    if(legal_moves == 0){
        int kingSq = lsb(board.get_pieces(Us,KING));
        if(is_square_attacked<Them>(static_cast<Square>(kingSq), board)){
            return - MATE_SCORE + depth; //checkmate
        } else {
            return 0; //Stalemate
        }
    }
    
    return max_score;
}