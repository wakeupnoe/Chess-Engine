#pragma once
#include "board.h"

// CONTRACT [Static Exchange Evaluation]:
// - ASSUMES: move is at least pseudo-legal.
// - GUARANTEES: Will not overflow piece value arrays even if called on quiet moves.
// - GUARANTEES: Bounded attacker loop (max 32 depths) prevents infinite loops.

int static_exchange_evaluation(const Board& board, Move move);