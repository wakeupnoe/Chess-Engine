#pragma once
#include "types.h"
#include "utils.h"
#include "board.h"
#include "movegen.h"
#include "evaluation.h"
#include "see.h"
#include "tt.h"
#include "move_picker.h"
#include <cstring>
#include <iostream>
#include <chrono>

constexpr int MAX_PLY = 128; // was 64, extended for quiescence search; upper bound of search depth
constexpr int NUM_KILLERS = 2; // how many killermoves we store (this should be 2 if otherwise, we need to update move_picker!)

// Global
extern int LMR_table[64][64];
void init_lmr_table();

inline uint64_t nodes_searched = 0;
inline Move killer_moves[MAX_PLY][NUM_KILLERS];
inline int history_heuristic[2][64][64];

inline void clear_history_heuristic(){
    std::memset(history_heuristic,0,sizeof(history_heuristic));
}

/*const int MVV_LVT[6][6] = { // AI Generated!
    // Attacker: P,   N,   B,   R,   Q,   K
    { 15,  14,  13,  12,  11,  10 }, // Victim: Pawn (0)
    { 25,  24,  23,  22,  21,  20 }, // Victim: Knight (1)
    { 35,  34,  33,  32,  31,  30 }, // Victim: Bishop (2)
    { 45,  44,  43,  42,  41,  40 }, // Victim: Rook (3)
    { 55,  54,  53,  52,  51,  50 }, // Victim: Queen (4)
    { 0,   0,   0,   0,   0,   0  }  // Victim: King (5) - Kings aren't captured
};*/

/*const int FLAG_BONUS[16] = { // AI Generated!
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
};*/

/*template <Color Us>
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
        if(is_capture || is_queen_promo){
            Piece attacker = board.get_piece_on_sq(static_cast<Square>(from));
            Piece victim = board.get_piece_on_sq(static_cast<Square>(to));
            
            victim = (flag == 5 ? PAWN : victim);
            score = (static_exchange_evaluation(board, move) >= 0 ? FLAG_BONUS[flag] : - FLAG_BONUS[flag])  + MVV_LVT[victim][attacker];
        } else {
            // 3. Killer Moves (Score: 900,000)
            bool is_killer = false;
            for(int k = 0; k<NUM_KILLERS; k++){
                if(move == killer_moves[ply][k]){
                    score = 900'000 - k*100'000;
                    is_killer = true;
                }
            }
            
            // 4. The History Heuristic (Score: 0 to ~800,000)
            if(!is_killer){
                score = history_heuristic[Us][from][to];
            }
        }
        
        // 5. Bad Captures (Score: Negative)
        
        list.scores[i] = score;
    }
}*/

template <Color Us>
Move get_best_move(Board& board, int target_depth){
    constexpr Color Them = Us == WHITE ? BLACK : WHITE; 
    Move best_move_overall = 0;
    clear_history_heuristic();
    nodes_searched = 0;

    auto start_time = std::chrono::steady_clock::now();
    int prev_score = 0;
    int king_sq = lsb(board.get_pieces(Us, KING));
    bool in_check = is_square_attacked<Them>(static_cast<Square>(king_sq),board);
    for(int current_depth = 1; current_depth <= target_depth; current_depth++){
        //std::cout << "DEBUG searching at depth " << current_depth << std::endl; 
        int window_alpha = current_depth==1 ?  -INFINITY_SCORE : std::max(-INFINITY_SCORE, prev_score - 50);
        int window_beta = current_depth==1 ?  INFINITY_SCORE : std::min(INFINITY_SCORE, prev_score + 50);
        
        int root_newdepth = in_check ? current_depth : current_depth - 1;

        //MoveList list = generate_moves<Us>(board);
        TTEntry* tt_entry = TT.probe(board.get_zobrist_key());
        Move tt_move = tt_entry != nullptr ? tt_entry->move : 0;
        //score_moves<Us>(list, board, 0, tt_move);
        Move current_best_move = 0;
        
        while(true){
            int max_score = -INFINITY_SCORE;
            int legal_moves = 0;
            int alpha = window_alpha;
            int beta = window_beta;

            MovePicker<Us> picker(board, tt_move, killer_moves[0][0], killer_moves[0][1]);
            Move move;
            
            while((move = picker.next_move()) != 0){
            /*for(int i = 0; i<list.size(); i++){
                // lazy selection sort
                int best_score_idx = i;
                for(int j = i+1; j<list.size(); j++){
                    if(list.scores[j] > list.scores[best_score_idx]){
                        best_score_idx = j;
                    }
                }
    
                std::swap(list.moves[i],list.moves[best_score_idx]);
                std::swap(list.scores[i],list.scores[best_score_idx]);
    
                Move move = list.get(i);
              */  
                if(board.make_move<Us>(move)){
                    legal_moves++;
                    int score;
                    if(legal_moves == 1){
                        score = -search<Them>(board, root_newdepth, -beta, -alpha, 1);
                    } else {
                        score = -search<Them>(board, root_newdepth, -alpha - 1, -alpha, 1);
                        if(score > alpha && score < beta){
                            score = -search<Them>(board, root_newdepth, -beta, -alpha, 1);
                        }
                    }
                    board.unmake_move<Us>(move);
            
                    if(score > max_score || current_best_move == 0){
                        current_best_move = move;
                        max_score = score;
                    }
                    alpha = std::max(alpha, max_score);
                    if(alpha >=window_beta){
                        break;
                    }
                }
            }

            if(legal_moves == 0){
                if(in_check) prev_score = -MATE_SCORE;
                else prev_score = 0;
                break;
            }

            if(max_score <= window_alpha && window_alpha > -INFINITY_SCORE){ // doing worse than expected
                window_alpha = -INFINITY_SCORE;
                continue;
            }

            if(max_score >= window_beta && window_beta < INFINITY_SCORE){ // doing better than expected
                window_beta = INFINITY_SCORE;
                continue;
            }


            prev_score = max_score;
            best_move_overall = current_best_move;
            break;
        }



        // UCI metrics:
        auto current_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = current_time - start_time;
        double seconds = elapsed.count();

        uint64_t nps = (seconds > 0.0001) ? static_cast<uint64_t>(nodes_searched / seconds) : 0;
        int time_ms = static_cast<int>(seconds * 1000);
        int hashfull = TT.get_hashfull();

        // UCI type output
        /**/
        std::cout << "info depth " << current_depth
                << " score cp " << prev_score
                << " nodes " << nodes_searched
                << " nps " << nps
                << " hashfull " << hashfull
                << " time " << time_ms
                << " pv "
                << std::endl;
        /**/
        //std::cout << " best_move " << move_to_string(board, current_best_move) << std::endl; // todo delete, not in the UCI 


        if(prev_score > MATE_SCORE - 100){
            break; // forced mate detected
        }
    }
    return best_move_overall;
}



template <Color Us>
int quiescence_search(Board& board, int alpha, int beta, int ply){
    nodes_searched++;
    
    int standard_pat = evaluate<Us>(board);
    if(ply >= MAX_PLY - 1) return standard_pat;

    if(standard_pat >= beta) return beta;
    alpha = (alpha < standard_pat) ? standard_pat : alpha;

    
    int original_alpha = alpha;
    
    TTEntry* tt_entry = TT.probe(board.get_zobrist_key());
    Move tt_move = 0;
    
    if(tt_entry != nullptr){
        tt_move = tt_entry->move;
        int tt_score = score_from_tt(tt_entry->score, ply);
        if(tt_entry->flag == TT_EXACT) return tt_score;
        if(tt_entry->flag == TT_ALPHA && tt_score <= alpha) return tt_score;
        if(tt_entry->flag == TT_BETA && tt_score >= beta) return tt_score;
    }
    
    constexpr Color Them = Us == WHITE ? BLACK : WHITE; 
    
    int max_score = standard_pat;
    Move best_move = 0;
    // MoveList list = generate_moves<Us>(board);
    // score_moves<Us>(list, board, ply, tt_move);
    MovePicker<Us> picker(board, tt_move, killer_moves[ply][0], killer_moves[ply][1]);
    Move move;
    while((move = picker.next_move()) != 0){
        if(picker.get_current_state() > GOOD_CAPTURES) break; // makes sense?
    /*for(int i = 0; i<list.size(); i++){
        // lazy selection sort
        int best_score_idx = i;
        for(int j = i+1; j<list.size(); j++){
            if(list.scores[j] > list.scores[best_score_idx]){
                best_score_idx = j;
            }
        }
        
        std::swap(list.moves[i],list.moves[best_score_idx]);
        std::swap(list.scores[i],list.scores[best_score_idx]);
        
        Move move = list.get(i);
    */
        int flag = get_flags(move);
        bool is_quiet = flag <= 3;
        if(is_quiet) continue; // we can use current_State in picker, right?
        //if(static_exchange_evaluation(board,move) < 0) continue;
        
        if(board.make_move<Us>(move)){
            int score = -quiescence_search<Them>(board, -beta, -alpha, ply + 1);
            board.unmake_move<Us>(move);

            best_move = (score > max_score ? move : best_move);
            max_score = std::max(score, max_score);
            alpha = std::max(alpha, max_score);
            if(beta <= max_score){
                break; // beta cutoff
            }
        }
    }

    TTFlag flag = (max_score <= original_alpha) ? TT_ALPHA : (max_score >= beta ? TT_BETA : TT_EXACT);
    TT.store(board.get_zobrist_key(), best_move, score_to_tt(max_score, ply), 0, flag, board.get_game_ply());
    
    return max_score;
}






template <Color Us>
int search(Board& board, int depth, int alpha, int beta, int ply){
    nodes_searched++;
    if(board.is_draw()) return 0; // "ply > 0 &&" important if we remove root search function
    if(ply >= MAX_PLY - 1) return evaluate<Us>(board);
    
    int original_alpha = alpha;
    constexpr Color Them = Us == WHITE ? BLACK : WHITE; 
    
    int kingSq = lsb(board.get_pieces(Us,KING));
    bool in_check = is_square_attacked<Them>(static_cast<Square>(kingSq), board);
    int newdepth = in_check ? depth : depth - 1;

    TTEntry* tt_entry = TT.probe(board.get_zobrist_key());
    Move tt_move = 0;
    
    if(tt_entry != nullptr){
        tt_move = tt_entry->move;
        int tt_score = score_from_tt(tt_entry->score, ply);
        if(tt_entry->depth >= depth || (std::abs(tt_score) > MATE_SCORE - 100)){
            //cutoff?
            if(tt_entry->flag == TT_EXACT) return tt_score;
            if(tt_entry->flag == TT_ALPHA && tt_score <= alpha) return tt_score;
            if(tt_entry->flag == TT_BETA && tt_score >= beta) return tt_score;
        }
    }
    
    // Internal Itterative Deepening
    if (tt_move == 0 && depth >= 4 && !in_check){
        search<Us>(board, depth - 2, alpha, beta, ply); // just to populate the TT table (with a shallow search)
        tt_entry = TT.probe(board.get_zobrist_key());
        if(tt_entry != nullptr) tt_move = tt_entry->move;
    }

    
    
    // Null-Move Pruning
    constexpr int R = 2; // Depth reduction if we are safe?
    if(!in_check && depth >= 3 && (board.get_color_pieces(Us) ^ board.get_pieces(Us,PAWN) ^ board.get_pieces(Us, KING)) != 0ULL){
        board.make_null_move<Us>();
        
        int null_score = -search<Them>(board, newdepth - R, -beta, -beta + 1, ply + 1);
        board.unmake_null_move<Us>();
        if(null_score >= beta){
            return beta;
        }
    }
    
    
    if(depth <= 0){ // originally "==", but LMR might lead to negative depth
        return quiescence_search<Us>(board, alpha, beta, ply);
    }  
    
    int max_score = -INFINITY_SCORE;
    int legal_moves = 0;
    Move best_move = 0;
    
    // MoveList list = generate_moves<Us>(board);
    
    // score_moves<Us>(list, board, ply, tt_move);
    MovePicker<Us> picker(board, tt_move, killer_moves[ply][0], killer_moves[ply][1]);
    Move move;
    while((move = picker.next_move()) != 0){
    /*for(int i = 0; i<list.size(); i++){
        // lazy selection sort
        int best_score_idx = i;
        for(int j = i+1; j<list.size(); j++){
            if(list.scores[j] > list.scores[best_score_idx]){
                best_score_idx = j;
            }
        }
        
        std::swap(list.moves[i],list.moves[best_score_idx]);
        std::swap(list.scores[i],list.scores[best_score_idx]);
        
        Move move = list.get(i);
        */
        int flag = get_flags(move);
        bool is_quiet = flag <= 3;
        bool is_capture = flag == 4 || flag == 5 || flag >= 12;
        //int SME = 0;
        //if(is_capture) SME = static_exchange_evaluation(board, move); // needed?
        bool is_bad_capture = (picker.get_current_state() == BAD_CAPTURES);
        if(board.make_move<Us>(move)){
            legal_moves++;
            int score;
            if(legal_moves == 1){
                // PV move
                score = -search<Them>(board, newdepth, -beta, -alpha, ply + 1);
            } else {
                int R_LMR = 0;

                if(!in_check && (is_quiet || is_bad_capture)){
                    R_LMR = LMR_table[std::min(newdepth,63)][std::min(legal_moves,63)];
                    if(picker.get_current_state() == KILLERS) R_LMR = R_LMR > 0 ? R_LMR - 1 : R_LMR;

                    /*bool is_killer = false;
                    for(int k = 0; k<NUM_KILLERS; k++){
                        if(move == killer_moves[ply][k]){
                            is_killer = true;
                        }
                    }
                    if(is_killer){
                        R_LMR = (R_LMR > 0) ? R_LMR - 1 : R_LMR;
                    }*/
                }

                score = -search<Them>(board, newdepth - R_LMR , -alpha - 1, -alpha, ply + 1);

                if(score > alpha && R_LMR > 0){
                    score = -search<Them>(board, newdepth, -alpha -1, -alpha, ply + 1);
                }

                if(score > alpha && score < beta){
                    score = -search<Them>(board, newdepth, -beta, -alpha, ply + 1);
                }
            }

            board.unmake_move<Us>(move);
            best_move = (score > max_score ? move : best_move);
            max_score = std::max(score, max_score);
            if(beta <= max_score){
                int flag = get_flags(move);

                if(is_quiet){
                    // killer moves
                    bool duplicate = false;
                    for(int k = 0; k<NUM_KILLERS; k++){
                        if(killer_moves[ply][k] == move){
                            duplicate = true;
                            break;
                        }
                    }
                    if(!duplicate){
                        for(int k = NUM_KILLERS - 1; k>0; k--){
                            killer_moves[ply][k] = killer_moves[ply][k-1];
                        }
                        killer_moves[ply][0] = move;
                    }

                    // history heuristic
                    int from = get_from_sq(move);
                    int to = get_to_sq(move);
                    history_heuristic[Us][from][to] += depth*depth;
                }

                break; // beta cutoff
            }
            alpha = std::max(alpha, max_score);
        }
    }
    if(legal_moves == 0){
        int kingSq = lsb(board.get_pieces(Us,KING));
        if(is_square_attacked<Them>(static_cast<Square>(kingSq), board)){
            max_score = - MATE_SCORE + ply; //checkmate
        } else {
            max_score = 0; //Stalemate
        }
    }
    TTFlag flag = (max_score <= original_alpha) ? TT_ALPHA : (max_score >= beta ? TT_BETA : TT_EXACT);
    TT.store(board.get_zobrist_key(), best_move, score_to_tt(max_score, ply), depth, flag, board.get_game_ply());
    return max_score;
}