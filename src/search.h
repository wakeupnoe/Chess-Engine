#pragma once
#include "types.h"
#include "utils.h"
#include "board.h"
#include "movegen.h"
#include "evaluation.h"
#include "see.h"
#include "tt.h"
#include "move_picker.h"
#include "time_manager.h" // time mangement
#include "nnue.h"
//#include "thread_local_heuristics.h" // todo replace this with Worker class
#include <cstring>
#include <iostream>
#include <chrono> // for metrics
#include <atomic> // for multithreading
#include <thread> // for multithreading


extern std::atomic<bool> abort_search;

inline void check_time(){
    if(TM.hard_limit_reached()){
        abort_search = true;
    }
}

// Global
extern int LMR_table[64][64];
void init_lmr_table();

inline std::atomic<uint64_t> nodes_searched = 0;

struct SearchInfo {
    Move excluded_move = 0;
    Move move_played = 0;
};

class SearchWorker {
private:
    inline void update_history(int& entry, int bonus){
        entry += bonus - (entry * std::abs(bonus)) / HISTORY_MAX;
    }

    inline void clear_search_stack(){
        std::fill(std::begin(search_stack), std::end(search_stack), SearchInfo{});
    }
    template <Color Us> // add Nodetype later
    int search(int depth, int alpha, int beta, int ply);
    template <Color Us> // add Nodetype later
    int quiescence_search(int alpha, int beta, int ply);

public:
    int thread_id;
    Board board;
    Move killer_moves[MAX_PLY][NUM_KILLERS] = {};
    int history_heuristic[2][64][64] = {}; // color from to
    // later multi ply histories
    int counter_move_history[2][64][64][64] = {}; // color prev_to from to
    Accumulator accumulators[MAX_PLY];
    SearchInfo search_stack[MAX_PLY];
    int max_seldepth = 0;
    int local_counter = 0;
    template <Color Us> // add Nodetype later
    Move get_best_move(int target_depth);
    inline void clear_histories() {
        std::memset(history_heuristic,0,sizeof(history_heuristic));
        // later multi ply histories
        std::memset(counter_move_history,0,sizeof(counter_move_history));
    }
};
