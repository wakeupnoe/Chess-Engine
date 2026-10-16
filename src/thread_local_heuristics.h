#pragma once
#include "types.h"          // for Color, etc.
constexpr int MAX_PLY = 128; // was 64, extended for quiescence search; upper bound of search depth
constexpr int NUM_KILLERS = 2; // how many killermoves we store (this should be 2 if otherwise, we need to update move_picker!)

// Killer moves – two per ply
inline thread_local Move killer_moves[MAX_PLY][NUM_KILLERS] = {};
// History heuristic – [color][from][to]
inline thread_local int history_heuristic[2][64][64] = {};
// Counter‑move history – [color][prev_to][from][to]
inline thread_local int counter_move_history[2][64][64][64] = {};

  