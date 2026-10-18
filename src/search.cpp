#include "search.h"
#include <cmath>

std::atomic<bool> abort_search = false;

int LMR_table[64][64];

template <Color Us>
Move SearchWorker::get_best_move(int target_depth){
    assert(target_depth > 0 && target_depth <= MAX_PLY && "get_best_move: target_depth out of bounds!");
    assert(board.get_pieces(Us, KING) != 0ULL && "get_best_move: Root position has no King!");

    Move best_move_overall = 0;
    clear_search_stack();

    max_seldepth = 0;
    root_best_move = 0;

    refresh_accumulators(board,accumulators[0]);

    int prev_score = 0;
    int actual_target = std::min(target_depth + thread_id % 2, MAX_PLY - 1);

    for(int current_depth = 1; current_depth <= actual_target; current_depth++){
        int window_alpha = current_depth==1 ?  -INFINITY_SCORE : std::max(-INFINITY_SCORE, prev_score - 50);
        int window_beta = current_depth==1 ?  INFINITY_SCORE : std::min(INFINITY_SCORE, prev_score + 50);
        while(true){
            int max_score = search<Us,NodeType::Root>(current_depth,window_alpha, window_beta, 0);

            if(abort_search) break;

            if(root_best_move != 0) best_move_overall = root_best_move;

            if(max_score <= window_alpha && window_alpha > -INFINITY_SCORE){ // doing worse than expected
                window_alpha = -INFINITY_SCORE;
                continue;
            }

            if(max_score >= window_beta && window_beta < INFINITY_SCORE){ // doing better than expected
                window_beta = INFINITY_SCORE;
                continue;
            }

            prev_score = max_score;
            break;
        }
        
        if(abort_search) break;
        if(thread_id == 0) print_uci_info<Us>(current_depth,prev_score, best_move_overall);

        if(prev_score > MATE_SCORE - 100){
            break; // forced mate detected
        }
        if(TM.soft_limit_reached()) break;
    }
    return best_move_overall;
}

template <Color Us, NodeType NT>
int SearchWorker::quiescence_search(int alpha, int beta, int ply){
    assert(ply >= 0 && ply < MAX_PLY && "qsearch: Ply out of bounds!");
    
    constexpr bool is_pv = (NT != NodeType::NonPV);
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;

    if(local_counter >= 2048){
        nodes_searched += local_counter;
        local_counter = 0;
        check_time();
    }
    
    if(abort_search) return 0;

    local_counter++;
    if constexpr (is_pv) max_seldepth = std::max(max_seldepth, ply);
    int standard_pat = nnue_evaluate<Us>(accumulators[ply]);
    if(ply >= MAX_PLY - 1) return standard_pat;

    if(standard_pat >= beta) return beta;
    alpha = (alpha < standard_pat) ? standard_pat : alpha;

    
    int original_alpha = alpha;

    Move tt_move = 0;
    int16_t tt_score = 0;
    TTFlag tt_flag = TT_NONE;
    // dummy variables, will get optimized away:
    uint8_t tt_depth = 0;
    uint8_t tt_age= 0;
    bool tt_hit = TT.probe(board.get_zobrist_key(), tt_move, tt_score,  tt_depth,  tt_flag,  tt_age);

    if constexpr (!is_pv){
        if(tt_hit){
            int real_tt_score = score_from_tt(tt_score, ply);
            if(tt_flag == TT_EXACT) return real_tt_score;
            if(tt_flag == TT_ALPHA && real_tt_score <= alpha) return real_tt_score;
            if(tt_flag == TT_BETA && real_tt_score >= beta) return real_tt_score;
        }
    }
    
    
    int max_score = standard_pat;
    Move best_move = 0;
    MovePicker<Us> picker(board, tt_move, killer_moves[ply][0], killer_moves[ply][1], history_heuristic, counter_move_history); // prev_to is defaulted to -1
    Move move;
    while((move = picker.next_move()) != 0){
        if(picker.get_current_state() > GOOD_CAPTURES) break;
        int flag = get_flags(move);
        bool is_quiet = flag <= 3;
        if(is_quiet) continue;
        
        if(board.make_move<Us>(move, accumulators[ply], accumulators[ply+1])){
            int score = -quiescence_search<Them, NT>(-beta, -alpha, ply + 1);
            board.unmake_move<Us>(move);
            if(abort_search) return 0;
            best_move = (score > max_score ? move : best_move);
            max_score = std::max(score, max_score);
            alpha = std::max(alpha, max_score);
            if(beta <= max_score){
                break; // beta cutoff
            }
        }
    }
    if(!abort_search){
        TTFlag flag = (max_score <= original_alpha) ? TT_ALPHA : (max_score >= beta ? TT_BETA : TT_EXACT);
        TT.store(board.get_zobrist_key(), best_move, score_to_tt(max_score, ply), 0, flag, board.get_game_ply());
    }
    return max_score;
}




// CONTRACT [Main Search]:
// - ASSUMES: ply is strictly between 0 and MAX_PLY.
// - GUARANTEES: Safe recursive depth management without overflowing killer_moves or history heuristics.

template <Color Us, NodeType NT>
int SearchWorker::search(int depth, int alpha, int beta, int ply){
    assert(ply >= 0 && ply < MAX_PLY && "search: Ply out of bounds!");
    assert(board.get_pieces(Us, KING) != 0ULL && "search: Board has no King!");

    constexpr bool is_root = (NT == NodeType::Root);
    constexpr bool is_pv = (NT != NodeType::NonPV);
    constexpr Color Them = Us == WHITE ? BLACK : WHITE; 


    if(local_counter >= 2048){
        nodes_searched += local_counter;
        local_counter = 0;
        check_time();
    }
    local_counter++;
    if constexpr (is_pv) max_seldepth = std::max(max_seldepth, ply);
    
    if(abort_search) return 0;

    int kingSq = lsb(board.get_pieces(Us,KING));
    bool in_check = is_square_attacked<Them>(static_cast<Square>(kingSq), board);
    int newdepth = in_check ? depth : depth - 1;

    if constexpr (!is_root){
        if(board.is_draw()) return 0;
        if(ply >= MAX_PLY - 1) return nnue_evaluate<Us>(accumulators[ply]);
        if(!in_check && depth <= 0){ // originally "==", but LMR might lead to negative depth
            return quiescence_search<Us, NT>(alpha, beta, ply);
        }
    }

    int original_alpha = alpha;

    Move tt_move = 0;
    int16_t tt_score = 0;
    uint8_t tt_depth = 0;
    // dummy variables, will get optimized away:
    TTFlag tt_flag = TT_NONE;
    uint8_t tt_age = 0;
    
    bool tt_hit = false;
    int extension = 0;
    // TT probe
    if(search_stack[ply].excluded_move == 0){
        tt_hit = TT.probe(board.get_zobrist_key(), tt_move, tt_score,  tt_depth,  tt_flag,  tt_age);
        if(tt_hit){
            int real_tt_score = score_from_tt(tt_score, ply);
            if constexpr (!is_pv){
                if((tt_depth >= depth || (std::abs(real_tt_score) > MATE_SCORE - 100))){
                    //cutoff?
                    if(tt_flag == TT_EXACT) return real_tt_score;
                    if(tt_flag == TT_ALPHA && real_tt_score <= alpha) return real_tt_score;
                    if(tt_flag == TT_BETA && real_tt_score >= beta) return real_tt_score;
                }
            }
            
            if constexpr (!is_root){
                // singular extension
                if(depth >= 6 && search_stack[ply].excluded_move == 0 && tt_move != 0 && tt_depth >= depth - 3 && tt_flag != TT_ALPHA && std::abs(real_tt_score) < MATE_SCORE - 100) {
                    int singular_beta = real_tt_score - (depth * 2);
                    int exclusion_depth = (depth -1) / 2;
                    search_stack[ply].excluded_move = tt_move;
                    int exclusion_score = search<Us, NodeType::NonPV>(exclusion_depth, singular_beta - 1, singular_beta, ply);
                    search_stack[ply].excluded_move = 0;
                    if(exclusion_score < singular_beta) extension = 1;
                }
            }
        }
    }
    if constexpr (is_root){
        if(root_best_move != 0) tt_move = root_best_move;
    }

    // For Reverse Futility Pruning: evalute board
    int board_evaluation = nnue_evaluate<Us>(accumulators[ply]);

    if constexpr (!is_pv){
        if(!in_check && search_stack[ply].excluded_move == 0){
            // Reverse Futility Pruning
            constexpr int RFP_MAX_DEPTH = 3;
            constexpr int BASE_MARGIN = 50;
            constexpr int MARGIN_MULTIPLIER = 100;
            if(!in_check && depth <= RFP_MAX_DEPTH){
                int margin = BASE_MARGIN + depth * MARGIN_MULTIPLIER;
                if(board_evaluation - margin >= beta) return board_evaluation;
            }
        
            // Null-Move Pruning
            int R = 3 + depth / 3;//5+depth/3; // was 2 OR 5 + depth/4;// 2; // Depth reduction if we are safe
            bool prev_was_null = (ply > 0 && search_stack[ply - 1].move_played == 0);
            if(!prev_was_null && depth >= R+1 && board_evaluation >= beta && (board.get_color_pieces(Us) ^ board.get_pieces(Us,PAWN) ^ board.get_pieces(Us, KING)) != 0ULL){
                search_stack[ply].move_played = 0;
                board.make_null_move<Us>(accumulators[ply], accumulators[ply+1]);
                int null_score = -search<Them, NodeType::NonPV>(newdepth - R, -beta, -beta + 1, ply + 1);
                board.unmake_null_move<Us>();
                if(abort_search) return 0;
                if(null_score >= beta) return beta;
            }
        }
    }
    
    // former Internal Itterative Deepening, now replaced by IIR (iterative deepeningat root will fix the rest)
    if constexpr (!is_root){
        if (tt_move == 0 && depth >= 4 && !in_check && search_stack[ply].excluded_move == 0){
            depth--;
            newdepth--; //IIR needs good move ordering even when we have no TT move!
        }
    }
    
    int max_score = -INFINITY_SCORE;
    int legal_moves = 0;
    Move best_move = 0;

    constexpr int LMP_MAX_DEPTH = 4;
    
    Move prev_move = (ply > 0) ? search_stack[ply - 1].move_played : 0;
    int prev_to = (prev_move != 0) ? get_to_sq(prev_move) : -1;

    MovePicker<Us> picker(board, tt_move, killer_moves[ply][0], killer_moves[ply][1], history_heuristic, counter_move_history, prev_to);
    Move move;

    Move quiet_searched[64];
    int num_quiets = 0;


    while((move = picker.next_move()) != 0){
        if(move == search_stack[ply].excluded_move) continue; // part of singular extension
        int flag = get_flags(move);
        bool is_quiet = flag <= 3;
        bool is_capture = flag == 4 || flag == 5 || flag >= 12;
        
        bool is_bad_capture = (picker.get_current_state() == BAD_CAPTURES);

        constexpr int FP_MAX_DEPTH = 4;
        constexpr int FP_BASE_MARGIN = 100;
        constexpr int FP_MARGIN_MULTIPLIER = 100;

       if constexpr (!is_root){
           // Futility Pruning
           if(!in_check && legal_moves>0 && depth <= FP_MAX_DEPTH && is_quiet){
               int margin = FP_BASE_MARGIN + depth * FP_MARGIN_MULTIPLIER;
               if(board_evaluation + margin <= alpha) continue; // skip this bad move
           }
   
           // Late Move Pruning
           if(!in_check && legal_moves>0 && depth <= LMP_MAX_DEPTH && is_quiet){
               int lmp_limit = 3 + (depth * depth);
               if(legal_moves > lmp_limit){ // already searched a lot of quiet moves 
                   continue;
               }
           }
       }

        search_stack[ply].move_played = move;
        if(board.make_move<Us>(move, accumulators[ply], accumulators[ply+1])){
            legal_moves++;
            if constexpr (is_root){
                if(TM.elapsed_seconds() > 3.0 && thread_id == 0)
                std::cout << "info depth " << depth
                          << " currmove " << move_to_string(board, move)
                          << " currmovenumber " << legal_moves
                          << " time " << TM.elapsed_ms()
                          << " nodes " << nodes_searched << std::endl;
            }

            int score;
            int current_depth = newdepth + (move==tt_move ? extension : 0);

            if(legal_moves == 1){
                constexpr NodeType NextNT = is_pv ? NodeType::PV : NodeType::NonPV;
                score = -search<Them, NextNT>(current_depth, -beta, -alpha, ply + 1);
            } else {
                int R_LMR = 0;

                if(!in_check && (is_quiet || is_bad_capture)){
                    R_LMR = LMR_table[std::min(current_depth,63)][std::min(legal_moves,63)];
                    if(picker.get_current_state() == KILLERS) R_LMR = R_LMR > 0 ? R_LMR - 1 : R_LMR;
                }

                score = -search<Them, NodeType::NonPV>(std::max(0,current_depth - R_LMR), -alpha - 1, -alpha, ply + 1);

                if(score > alpha && R_LMR > 0){
                    score = -search<Them, NodeType::NonPV>(current_depth, -alpha -1, -alpha, ply + 1);
                }
                if constexpr (is_pv) {
                    if(score > alpha && score < beta){
                        score = -search<Them, NodeType::PV>(current_depth, -beta, -alpha, ply + 1);
                    }
                }
            }

            board.unmake_move<Us>(move);
            if(abort_search) return 0;
            best_move = (score > max_score ? move : best_move);
            if constexpr (is_root) root_best_move = (score > max_score ? move : root_best_move);
            max_score = std::max(score, max_score);
            if(beta <= max_score){
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
                    int bonus = std::min(current_depth * current_depth, 400);

                    int from = get_from_sq(move);
                    int to = get_to_sq(move);
                    // reward move that caused cutoff
                    update_history(history_heuristic[Us][from][to], bonus);
                    if(prev_to != -1) update_history(counter_move_history[Us][prev_to][from][to], bonus);
                    //punish moves that failed earlier
                    for(int i = 0; i<num_quiets; i++){
                        Move penalty_move = quiet_searched[i];
                        int p_from = get_from_sq(penalty_move);
                        int p_to = get_to_sq(penalty_move);
                        update_history(history_heuristic[Us][p_from][p_to], -bonus);
                        if(prev_to != -1) update_history(counter_move_history[Us][prev_to][p_from][p_to], -bonus);

                    }


                }

                break; // beta cutoff
            }
            alpha = std::max(alpha, max_score);

            if(is_quiet && num_quiets < 64){
                quiet_searched[num_quiets++] = move;
            }
        }
    }
    if(legal_moves == 0){
        max_score = (in_check) ? - MATE_SCORE + ply : 0;
    }
    if(!abort_search && search_stack[ply].excluded_move == 0){
        TTFlag flag = (max_score <= original_alpha) ? TT_ALPHA : (max_score >= beta ? TT_BETA : TT_EXACT);
        TT.store(board.get_zobrist_key(), best_move, score_to_tt(max_score, ply), depth, flag, static_cast<uint8_t>(board.get_game_ply() & 0xFF));
    }
    return max_score;
}

template <Color Us>
void SearchWorker::print_uci_info(int current_depth, int prev_score, Move current_best_move){
    // UCI metrics:
    Move tt_move = 0;
    // dummy variables, will get optimized away:
    int16_t tt_score = 0;
    uint8_t tt_depth = 0;
    TTFlag tt_flag = TT_NONE;
    uint8_t tt_age = 0;

    double seconds = TM.elapsed_seconds(); //elapsed.count();
    
    uint64_t nps = (seconds > 0.0001) ? static_cast<uint64_t>(nodes_searched / seconds) : 0;
    int time_ms = static_cast<int>(seconds * 1000);
    int hashfull = TT.get_hashfull();
    
    std::string score_string;
    if(std::abs(prev_score) > MATE_SCORE - 100){
        int plies_to_mate = MATE_SCORE - std::abs(prev_score);
        int moves_to_mate = (plies_to_mate + 1) / 2;
        if(prev_score < 0) moves_to_mate = -moves_to_mate;
        score_string = "mate " + std::to_string(moves_to_mate);
    } else {
        score_string = "cp " + std::to_string(prev_score);
    }
    
    // UCI type output
    std::cout << "info depth " << current_depth
    << " seldepth " << max_seldepth
    << " score " << score_string
    << " nodes " << nodes_searched
    << " nps " << nps
    << " hashfull " << hashfull
    << " time " << time_ms
    << " pv ";
    Move pv_move;
    if(current_best_move != 0){
        std::cout << move_to_string(board,current_best_move) << " ";
        Board temp_board = board;
        if(Us == WHITE) temp_board.make_move<WHITE>(current_best_move, accumulators[0], accumulators[1]);
        else temp_board.make_move<BLACK>(current_best_move, accumulators[0], accumulators[1]);
        for(int i = 1; i<63; i++){
            bool tt_hit = TT.probe(temp_board.get_zobrist_key(), pv_move, tt_score,  tt_depth,  tt_flag,  tt_age);

            if(tt_hit && pv_move != 0){
                Color c = temp_board.get_side_to_move();
                
                std::cout << move_to_string(temp_board,pv_move) << " ";
                
                bool legal = (c==WHITE) ? temp_board.make_move<WHITE>(pv_move, accumulators[i], accumulators[i+1]) : temp_board.make_move<BLACK>(pv_move, accumulators[i], accumulators[i+1]);
                if(!legal) break;
            } else {
                break;
            }
        }
        
    }
    std::cout << std::endl;
}



void init_lmr_table(){
    for(int depth = 0; depth < 64; depth++){
        for(int moves = 0; moves < 64; moves++){
            if(depth >= 2 && moves >= 4){
                double reduction = 0.75 + std::log(depth) * std::log(moves) / 2.25;
                LMR_table[depth][moves] = static_cast<int>(reduction);
            } else {
                LMR_table[depth][moves] = 0;
            }
        }
    }
}

template Move SearchWorker::get_best_move<WHITE>(int);
template Move SearchWorker::get_best_move<BLACK>(int);