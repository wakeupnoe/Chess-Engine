#pragma once
#include "types.h"          // for Color, etc.
#include <algorithm>
#include <cstring>
#include <vector>

// Killer moves – two per ply otherwise we need to change move picker!
inline thread_local Move killer_moves[MAX_PLY][NUM_KILLERS] = {};
// History heuristic – [color][from][to]
inline thread_local int history_heuristic[2][64][64] = {};
// Counter‑move history – [color][prev_to][from][to]
inline thread_local int counter_move_history[2][64][64][64] = {};

struct SearchInfo {
    Move excluded_move = 0;
    Move move_played = 0;
};

inline thread_local Accumulator accumulators[MAX_PLY];
inline thread_local SearchInfo search_stack[MAX_PLY];
inline thread_local int max_seldepth = 0;

inline thread_local int local_counter = 0; //nodes searched

inline void clear_history_heuristic(){
    std::memset(history_heuristic,0,sizeof(history_heuristic));
}

inline void clear_counter_move_history(){
    std::memset(counter_move_history,0,sizeof(counter_move_history));
}

inline void clear_search_stack(){
    std::fill(std::begin(search_stack), std::end(search_stack), SearchInfo{});
}

inline void update_history(int& entry, int bonus){
    entry += bonus - (entry * std::abs(bonus)) / HISTORY_MAX;
}