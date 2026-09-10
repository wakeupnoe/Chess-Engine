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

// CONTRACT [Search Globals & Limits]:
// - ASSUMES: Search ply NEVER exceeds MAX_PLY (128). Accessing killer_moves at ply >= 128 is a fatal overflow.
// - ASSUMES: LMR_table indexing (depth, move_count) is clamped to < 64.
// - GUARANTEES: Safe history heuristic initialization.

constexpr int MAX_PLY = 128; // was 64, extended for quiescence search; upper bound of search depth
constexpr int NUM_KILLERS = 2; // how many killermoves we store (this should be 2 if otherwise, we need to update move_picker!)

// Global
extern int LMR_table[64][64];
void init_lmr_table();

inline uint64_t nodes_searched = 0;
inline int max_seldepth = 0;
inline Move killer_moves[MAX_PLY][NUM_KILLERS];
inline int history_heuristic[2][64][64];
inline int counter_move_history[2][64][64][64];

struct SearchInfo {
    Move excluded_move = 0;
    Move move_played = 0;
};

inline SearchInfo search_stack[MAX_PLY];


inline void clear_history_heuristic(){
    std::memset(history_heuristic,0,sizeof(history_heuristic));
}

inline void clear_counter_move_history(){
    std::memset(counter_move_history,0,sizeof(counter_move_history));
}

inline void clear_search_stack(){
    std::fill(std::begin(search_stack), std::end(search_stack), SearchInfo{});
}

constexpr int HISTORY_MAX = 16384; // 2^14

inline void update_history(int& entry, int bonus){
    entry += bonus - (entry * std::abs(bonus)) / HISTORY_MAX;
}



// CONTRACT [Root Search]:
// - ASSUMES: The board has a King for the side to move (Us).
// - ASSUMES: target_depth is reasonable (<= MAX_PLY) so search doesn't overflow ply arrays.
// - GUARANTEES: Safe PV extraction that aborts on illegal TT moves.

template <Color Us>
Move get_best_move(Board& board, int target_depth){
    assert(target_depth > 0 && target_depth <= MAX_PLY && "get_best_move: target_depth out of bounds!");
    assert(board.get_pieces(Us, KING) != 0ULL && "get_best_move: Root position has no King!");

    constexpr Color Them = Us == WHITE ? BLACK : WHITE; 
    Move best_move_overall = 0;
    clear_history_heuristic();
    clear_counter_move_history();
    clear_search_stack();
    nodes_searched = 0;
    max_seldepth = 0;

    auto start_time = std::chrono::steady_clock::now();
    int prev_score = 0;
    int king_sq = lsb(board.get_pieces(Us, KING));
    bool in_check = is_square_attacked<Them>(static_cast<Square>(king_sq),board);
    for(int current_depth = 1; current_depth <= target_depth; current_depth++){
        //std::cout << "DEBUG searching at depth " << current_depth << std::endl; 
        int window_alpha = current_depth==1 ?  -INFINITY_SCORE : std::max(-INFINITY_SCORE, prev_score - 50);
        int window_beta = current_depth==1 ?  INFINITY_SCORE : std::min(INFINITY_SCORE, prev_score + 50);
        
        int root_newdepth = in_check ? current_depth : current_depth - 1;

        TTEntry* tt_entry = TT.probe(board.get_zobrist_key());
        Move tt_move = tt_entry != nullptr ? tt_entry->move : 0;
        Move current_best_move = 0;
        
        while(true){
            int max_score = -INFINITY_SCORE;
            int legal_moves = 0;
            int alpha = window_alpha;
            int beta = window_beta;

            MovePicker<Us> picker(board, tt_move, killer_moves[0][0], killer_moves[0][1]); // do not use Coutermovehistory, so prev_to is defaulted to -1
            Move move;
            
            while((move = picker.next_move()) != 0){
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
        /**/
        std::cout << "info depth " << current_depth
                << " seldepth " << max_seldepth
                << " score " << score_string
                << " nodes " << nodes_searched
                << " nps " << nps
                << " hashfull " << hashfull
                << " time " << time_ms
                << " pv ";
        /**/
        Board temp_board = board;
        for(int i = 0; i< 64; i++){
            TTEntry* entry = TT.probe(temp_board.get_zobrist_key());
            if(entry != nullptr && entry->move!=0){
                Move pv_move = entry->move;
                Color c = temp_board.get_side_to_move();
                bool is_valid = (c == WHITE) ? is_pseudo_legal<WHITE>(temp_board, pv_move) :  is_pseudo_legal<BLACK>(temp_board, pv_move);
                if(!is_valid) break;
                std::cout << move_to_string(temp_board, pv_move) << " ";
                bool legal = (c==WHITE) ? temp_board.make_move<WHITE>(pv_move) : temp_board.make_move<BLACK>(pv_move);
                if(!legal) break;
            } else {
                break;
            }
        }
        std::cout << std::endl;

        if(prev_score > MATE_SCORE - 100){
            break; // forced mate detected
        }
    }
    return best_move_overall;
}

// CONTRACT [Quiescence Search]:
// - ASSUMES: ply is strictly between 0 and MAX_PLY - 1.
// - GUARANTEES: Safe access to killer_moves array and prevents infinite capture recursion.

template <Color Us>
int quiescence_search(Board& board, int alpha, int beta, int ply){
    assert(ply >= 0 && ply < MAX_PLY && "qsearch: Ply out of bounds!");

    nodes_searched++;
    max_seldepth = std::max(max_seldepth, ply);
    // board.check_state_integrity();
    int standard_pat = evaluate<Us>(board);
    // board.check_state_integrity();
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
    MovePicker<Us> picker(board, tt_move, killer_moves[ply][0], killer_moves[ply][1]); // prev_to is defaulted to -1
    Move move;
    while((move = picker.next_move()) != 0){
        if(picker.get_current_state() > GOOD_CAPTURES) break; // makes sense?
        int flag = get_flags(move);
        bool is_quiet = flag <= 3;
        if(is_quiet) continue;
        
        if(board.make_move<Us>(move)){
            // board.check_state_integrity();
            int score = -quiescence_search<Them>(board, -beta, -alpha, ply + 1);
            // board.check_state_integrity();
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




// CONTRACT [Main Search]:
// - ASSUMES: ply is strictly between 0 and MAX_PLY.
// - GUARANTEES: Safe recursive depth management without overflowing killer_moves or history heuristics.

template <Color Us>
int search(Board& board, int depth, int alpha, int beta, int ply){
    assert(ply >= 0 && ply < MAX_PLY && "search: Ply out of bounds!");
    assert(board.get_pieces(Us, KING) != 0ULL && "search: Board has no King!");

    //TEMP DEBUGGING: // AI GENERATED
    // uint64_t pre_move_key = board.get_zobrist_key();
    // uint64_t hardcalc_pre = board.generate_zobrist_key();
    // DELETE TILL HERE

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

    if(depth <= 0){ // originally "==", but LMR might lead to negative depth
        return quiescence_search<Us>(board, alpha, beta, ply);
    }
    int extension = 0;
    // TT probe
    if(tt_entry != nullptr){
        tt_move = tt_entry->move;
        int tt_score = score_from_tt(tt_entry->score, ply);
        int tt_depth = tt_entry->depth;
        if(tt_depth >= depth || (std::abs(tt_score) > MATE_SCORE - 100)){
            //cutoff?
            if(tt_entry->flag == TT_EXACT) return tt_score;
            if(tt_entry->flag == TT_ALPHA && tt_score <= alpha) return tt_score;
            if(tt_entry->flag == TT_BETA && tt_score >= beta) return tt_score;
        }

        // singular extension
        if(search_stack[ply].excluded_move == 0 && tt_move != 0 && tt_depth >= depth - 3 && tt_entry->flag != TT_ALPHA && std::abs(tt_score) < MATE_SCORE - 100) {
            int singular_beta = tt_score - (depth * 2);
            int exclusion_depth = (depth -1) / 2;
            search_stack[ply].excluded_move = tt_move;
            int exclusion_score = search<Us>(board, exclusion_depth, singular_beta - 1, singular_beta, ply);
            search_stack[ply].excluded_move = 0;
            if(exclusion_score < singular_beta) extension = 1;
        }
    }

    // For Reverse Futility Pruning: evalute board
    int board_evaluation = evaluate<Us>(board);
    constexpr int RFP_MAX_DEPTH = 3;
    constexpr int BASE_MARGIN = 50;
    constexpr int MARGIN_MULTIPLIER = 100;
    // Reverse Futility Pruning
    if(!in_check && depth <= RFP_MAX_DEPTH){
        int margin = BASE_MARGIN + depth * MARGIN_MULTIPLIER;
        if(board_evaluation - margin >= beta) return board_evaluation;
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
    
    // Internal Itterative Deepening
    if (tt_move == 0 && depth >= 4 && !in_check){
        search<Us>(board, depth - 2, alpha, beta, ply); // just to populate the TT table (with a shallow search)
        tt_entry = TT.probe(board.get_zobrist_key());
        if(tt_entry != nullptr) tt_move = tt_entry->move;
    }

    
    int max_score = -INFINITY_SCORE;
    int legal_moves = 0;
    Move best_move = 0;

    constexpr int LMP_MAX_DEPTH = 4;
    
    Move prev_move = (ply > 0) ? search_stack[ply - 1].move_played : 0;
    int prev_to = (prev_move != 0) ? get_to_sq(prev_move) : -1;

    MovePicker<Us> picker(board, tt_move, killer_moves[ply][0], killer_moves[ply][1], prev_to);
    Move move;

    Move quiet_searched[64];
    int num_quiets = 0;


    while((move = picker.next_move()) != 0){
        if(move == search_stack[ply].excluded_move) continue; // part of singular extension
        int flag = get_flags(move);
        bool is_quiet = flag <= 3;
        bool is_capture = flag == 4 || flag == 5 || flag >= 12;
        
        bool is_bad_capture = (picker.get_current_state() == BAD_CAPTURES);
        
        // AI GENERATED DEBUG INFO
        /*MoveList test_list = generate_quiets<Us>(board);
        MoveList test_caps = generate_captures<Us>(board);
        bool move_exists = false;
        for(int i = 0; i < test_list.size(); i++) if(test_list.get(i) == move) move_exists = true;
        for(int i = 0; i < test_caps.size(); i++) if(test_caps.get(i) == move) move_exists = true;
        Board pre_move_board = board;
        */
       constexpr int FP_MAX_DEPTH = 4;
       constexpr int FP_BASE_MARGIN = 100;
       constexpr int FP_MARGIN_MULTIPLIER = 100;

       
        // Futility Pruning
        if(!in_check && depth <= FP_MAX_DEPTH && is_quiet){
            int margin = FP_BASE_MARGIN + depth * FP_MARGIN_MULTIPLIER;
            if(board_evaluation + margin <= alpha) continue; // skip this bad move
        }

        // Late Move Pruning
        if(!in_check && depth <= LMP_MAX_DEPTH && is_quiet){
            int lmp_limit = 3 + (depth * depth);
            if(legal_moves > lmp_limit){ // already searched a lot of quiet moves 
                continue;
            }
        }

        search_stack[ply].move_played = move;
        if(board.make_move<Us>(move)){
            legal_moves++;
            int score;

            int current_depth = newdepth + (move==tt_move ? extension : 0);

            if(legal_moves == 1){
                // PV move
                score = -search<Them>(board, current_depth, -beta, -alpha, ply + 1);
            } else {
                int R_LMR = 0;

                if(!in_check && (is_quiet || is_bad_capture)){
                    R_LMR = LMR_table[std::min(current_depth,63)][std::min(legal_moves,63)];
                    if(picker.get_current_state() == KILLERS) R_LMR = R_LMR > 0 ? R_LMR - 1 : R_LMR;
                }

                score = -search<Them>(board, current_depth - R_LMR , -alpha - 1, -alpha, ply + 1);

                if(score > alpha && R_LMR > 0){
                    score = -search<Them>(board, current_depth, -alpha -1, -alpha, ply + 1);
                }

                if(score > alpha && score < beta){
                    score = -search<Them>(board, current_depth, -beta, -alpha, ply + 1);
                }
            }

            board.unmake_move<Us>(move);
            best_move = (score > max_score ? move : best_move);
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
        int kingSq = lsb(board.get_pieces(Us,KING));
        if(is_square_attacked<Them>(static_cast<Square>(kingSq), board)){
            max_score = - MATE_SCORE + ply; //checkmate
        } else {
            max_score = 0; //Stalemate
        }
    }
    TTFlag flag = (max_score <= original_alpha) ? TT_ALPHA : (max_score >= beta ? TT_BETA : TT_EXACT);
    TT.store(board.get_zobrist_key(), best_move, score_to_tt(max_score, ply), depth, flag, static_cast<uint8_t>(board.get_game_ply() & 0xFF));
    return max_score;
}


/*if (!move_exists) {
                std::cout << "GHOST MOVE INJECTED BY PICKER AND PLAYED: " << move_to_string(board, move) << std::endl;
                //exit(1);
                assert(false && "GHOST MOVE INJECTED BY PICKER AND PLAYED");
            }
            // TEMP AI GENERATED DEBUG INFO
            // 🚨 THE DEEP DECODER
            uint64_t incremental_post = board.get_zobrist_key();
            uint64_t hardcalc_post = board.generate_zobrist_key();
            if (incremental_post != hardcalc_post) {
                std::cout << "\n--- FATAL ZOBRIST DESYNC ---" << std::endl;
                std::cout << "Move Made: " << move_to_string(board, move) << " (Flag: " << get_flags(move) << ")" << std::endl;
                
                std::cout << "\n--- BOARD BEFORE MOVE ---\n";
                std::cout << print_board(pre_move_board);
                
                std::cout << "\n--- BOARD AFTER MOVE ---\n";
                std::cout << print_board(board);
                
                // Print the raw piece array data for the corrupted square (e.g. b7 = 49)
                std::cout << "\nPre-move b7 array: " << pre_move_board.get_piece_on_sq(static_cast<Square>(49)) << "\n";
                std::cout << "Post-move b7 array: " << board.get_piece_on_sq(static_cast<Square>(49)) << "\n";
                
                if (!move_exists) std::cout << "GHOST MOVE INJECTED BY PICKER AND PLAYED: " << move_to_string(board, move) << std::endl;
                uint64_t diff = incremental_post ^ hardcalc_post;
                std::cout << "\n--- FATAL ZOBRIST DESYNC ---" << std::endl;
                std::cout << "Move Made: " << move_to_string(board, move) << " (Flag: " << get_flags(move) << ")" << std::endl;
                
                bool found = false;
                
                // 1. Check for single key corruptions
                if (diff == zobrist_side) { std::cout << "CORRUPTION: Side to Move\n"; found = true; }
                for(int i=0; i<16; i++) if(diff == zobrist_castling[i]) { std::cout << "CORRUPTION: Castling Right " << i << "\n"; found = true; }
                for(int i=0; i<65; i++) if(diff == zobrist_ep[i]) { std::cout << "CORRUPTION: EP Square " << i << "\n"; found = true; }
                for(int c=0; c<2; c++) for(int p=0; p<6; p++) for(int sq=0; sq<64; sq++) 
                    if(diff == zobrist_pieces[c][p][sq]) { std::cout << "CORRUPTION: Piece (C:" << c << " P:" << p << " Sq:" << sq << ")\n"; found = true; }

                // 2. Check for DOUBLE key corruptions (e.g. Color Desync, EP math mismatch, Castling mask error)
                if (!found) {
                    std::cout << "Running Deep Decode for Multi-Key Desync...\n";
                    
                    // Check if it's an EP file vs Exact Square mismatch
                    for(int ep1=0; ep1<65; ep1++) {
                        for(int ep2=0; ep2<65; ep2++) {
                            if (diff == (zobrist_ep[ep1] ^ zobrist_ep[ep2])) {
                                std::cout << "CORRUPTION: En Passant Mismatch! (EP1: " << ep1 << " vs EP2: " << ep2 << ")\n";
                                found = true;
                            }
                        }
                    }

                    // Check if it's a Castling Mask mismatch
                    for(int c1=0; c1<16; c1++) {
                        for(int c2=0; c2<16; c2++) {
                            if (diff == (zobrist_castling[c1] ^ zobrist_castling[c2])) {
                                std::cout << "CORRUPTION: Castling Rights Mismatch! (Right 1: " << c1 << " vs Right 2: " << c2 << ")\n";
                                found = true;
                            }
                        }
                    }

                    // Check if it's a Piece Color/Square mismatch
                    for(int c1=0; c1<2; c1++) for(int p1=0; p1<6; p1++) for(int sq1=0; sq1<64; sq1++) {
                        for(int c2=0; c2<2; c2++) for(int p2=0; p2<6; p2++) for(int sq2=0; sq2<64; sq2++) {
                            if (diff == (zobrist_pieces[c1][p1][sq1] ^ zobrist_pieces[c2][p2][sq2])) {
                                std::cout << "CORRUPTION: Piece Sync Error!\n";
                                std::cout << "   Key A: Color " << c1 << ", Piece " << p1 << ", Sq " << sq1 << "\n";
                                std::cout << "   Key B: Color " << c2 << ", Piece " << p2 << ", Sq " << sq2 << "\n";
                                found = true;
                            }
                        }
                    }
                }

                if (!found) std::cout << "CORRUPTION: 3+ Keys desynced (Massive Bitboard Failure)\n";
                assert(false && "FATAL ZOBRIST DESYNC CAUGHT");
            }*/
            // DELETE TILL HERE