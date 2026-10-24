#pragma once
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "see.h"
// temp
#include "utils.h"

enum MovePickerState : uint8_t {
    TT_MOVE, GENERATE_CAPTURES, GOOD_CAPTURES, KILLERS, GENERATE_QUIETS, QUIETS, BAD_CAPTURES
};

constexpr int SCORE_NONE = -10'000'000;

// CONTRACT [Move Picker]:
// - ASSUMES: Arrays are sized to 7x7 to safely absorb NO_PIECE_TYPE (6) without memory corruption.
// - GUARANTEES: Fast, bounds-checked move scoring.

const int MVV_LVT[7][7] = { // AI Generated!
    // Attacker: P,   N,   B,   R,   Q,   K,   NO_PIECE
    { 15,  14,  13,  12,  11,  10,  0 }, // Victim: Pawn (0)
    { 25,  24,  23,  22,  21,  20,  0 }, // Victim: Knight (1)
    { 35,  34,  33,  32,  31,  30,  0 }, // Victim: Bishop (2)
    { 45,  44,  43,  42,  41,  40,  0 }, // Victim: Rook (3)
    { 55,  54,  53,  52,  51,  50,  0 }, // Victim: Queen (4)
    { 0,   0,   0,   0,   0,   0,   0 }, // Victim: King (5) 
    { 0,   0,   0,   0,   0,   0,   0 }  // Victim: NO_PIECE_TYPE (6) - Failsafe
};

const int FLAG_BONUS[16] = { // AI Generated!
    0, 0, 0, 0,                // 0-3: Quiets, Double Push, Castles
    1'000'000, 1'000'000,      // 4-5: Capture, EP Capture
    0, 0,                      // 6-7: Unused
    0, 0, 0, 900'000,          // 8-11: Promo N, B, R, Q (Quiet)
    1'000'000, 1'000'000, 1'000'000, 1'900'000 // 12-15: Promo N, B, R, Q (Capture)
};


template <Color Us, bool QSearch = false>
class MovePicker{
private:
    MovePickerState current_state;
    const Board& board;
    Move tt_move;
    Move killers[2];
    int killer_index;
    int prev_to;

    const int (&history_heuristic)[2][64][64];
    const int (&counter_move_history)[2][64][64][64];

    MoveList captures;
    int captures_index;
    MoveList quiets;
    int quiets_index;


    void score_captures(){
        for(int i = 0; i<captures.size(); i++){
            Move m = captures.get(i);
            if(m == tt_move){
                captures.scores[i] = SCORE_NONE; // very negative? (not _score which is 32k)
                continue;
            }

            
            int flag = get_flags(m);
            int base_flag;
            Piece attacker = board.get_piece_on_sq(static_cast<Square>(get_from_sq(m)));
            Piece victim;

            assert(attacker >= PAWN && attacker <= KING && "MovePicker: Attacker is NO_PIECE_TYPE!");

            if(flag==5){
                victim = PAWN; // En passant
                base_flag = (static_exchange_evaluation(board, m) >= 0) ? FLAG_BONUS[flag] : -FLAG_BONUS[flag];
            } else if(flag >= 8 && flag <= 11){
                // quiet promotions, so victim pawn since we lose a pawn in the promotion?
                victim = PAWN;
                base_flag = FLAG_BONUS[flag];
            } else {
                victim = board.get_piece_on_sq(static_cast<Square>(get_to_sq(m)));
                assert(victim >= PAWN && victim <= KING && "MovePicker: Capture victim is NO_PIECE_TYPE!");
                base_flag = (static_exchange_evaluation(board, m) >= 0) ? FLAG_BONUS[flag] : -FLAG_BONUS[flag];
            }
            captures.scores[i] =  base_flag + MVV_LVT[victim][attacker];
        }
    }
    void score_quiets(){
        for(int i = 0; i<quiets.size(); i++){
            Move m = quiets.get(i);

            if(m == tt_move || m == killers[0] || m == killers[1]){
                quiets.scores[i] = SCORE_NONE;
                continue;
            }

            int from = get_from_sq(m);
            int to = get_to_sq(m);

            assert(from >= 0 && from < 64 && to >= 0 && to < 64 && "MovePicker: History heuristic out of bounds!");
            
            int score = history_heuristic[Us][from][to];

            if(prev_to != -1){
                score += counter_move_history[Us][prev_to][from][to];
            }
            quiets.scores[i] = score;
        }
    }

public:
    inline MovePickerState get_current_state(){ return current_state; }

    MovePicker(const Board& b, Move tt, Move killer1, Move killer2, const int (&hist_heur)[2][64][64], const int (&counter_m_hist)[2][64][64][64], int prev_move_to = -1)
        : current_state(TT_MOVE), board(b), tt_move(tt), killer_index(0), captures_index(0), quiets_index(0), history_heuristic(hist_heur), counter_move_history(counter_m_hist) {
        killers[0] = killer1;
        killers[1] = killer2;
        prev_to = prev_move_to;
    }

    Move next_move(){
        while(true){
            switch (current_state) {
            case TT_MOVE:{
                current_state = GENERATE_CAPTURES;

                if constexpr (QSearch) {if(get_flags(tt_move) < 4) continue;}
                if(is_pseudo_legal<Us>(board, tt_move)) return tt_move;
                continue;
            }
            case GENERATE_CAPTURES: {
                captures = generate_captures<Us>(board);
                score_captures();
                current_state = GOOD_CAPTURES;
                continue;
            }
            case GOOD_CAPTURES: {
                int best_score = SCORE_NONE;
                int best_index = -1;
                for(int i = captures_index; i<captures.size(); i++){
                    if(captures.scores[i] > best_score){
                        best_score = captures.scores[i];
                        best_index = i;
                    }
                }

                if(best_index == -1 || best_score < 0){
                    if constexpr (QSearch) return 0;
                    current_state = KILLERS;
                    continue;
                }

                std::swap(captures.moves[captures_index], captures.moves[best_index]);
                std::swap(captures.scores[captures_index], captures.scores[best_index]);
                Move move = captures.get(captures_index++);

                return move;
            }
            case KILLERS: {
                if(killer_index < 2){
                    Move move = killers[killer_index++];
                    if(move == 0 || move == tt_move) continue;
                    if(!is_pseudo_legal<Us>(board,move)) continue;
                    return move;
                }
                current_state = GENERATE_QUIETS;
                continue;
            }
            case GENERATE_QUIETS: {
                quiets = generate_quiets<Us>(board);
                score_quiets();
                current_state = QUIETS;
                continue;
            }
            case QUIETS: {
                int best_score = SCORE_NONE; // not _SCORE since that'd be 32k (too small)
                int best_index = -1;
                for(int i = quiets_index; i<quiets.size(); i++){
                    if(quiets.scores[i] > best_score){
                        best_score = quiets.scores[i];
                        best_index = i;
                    }
                }

                if(best_index == -1 || best_score == SCORE_NONE){
                    current_state = BAD_CAPTURES;
                    continue;
                }

                std::swap(quiets.moves[quiets_index], quiets.moves[best_index]);
                std::swap(quiets.scores[quiets_index], quiets.scores[best_index]);

                Move move = quiets.get(quiets_index++);

                // if(move == killers[0] || move == killers[1] || move == tt_move) continue; is not needed, since we only select things above - INFINITY
                return move;
            }
            case BAD_CAPTURES: {
                int best_score = SCORE_NONE;
                int best_index = -1;
                for(int i = captures_index; i<captures.size(); i++){
                    if(captures.scores[i] > best_score){
                        best_score = captures.scores[i];
                        best_index = i;
                    }
                }

                if(best_index == -1){ // no more moves to generate
                    return 0;
                }

                std::swap(captures.moves[captures_index], captures.moves[best_index]);
                std::swap(captures.scores[captures_index], captures.scores[best_index]);

                Move move = captures.get(captures_index++);

                //if(move == tt_move) continue; not needed, tt_move has score -Inf

                return move;
            }
            default:
                break;
            }
        }
        return 0;
    }
};