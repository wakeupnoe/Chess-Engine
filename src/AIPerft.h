#pragma once
#include "board.h"
#include <cstdint>

namespace Perft {
    // The recursive node-counting function
    uint64_t execute(Board& board, int depth);

    // The debugging wrapper: prints root moves, timing, and NPS
    void divide(Board& board, int depth);
}