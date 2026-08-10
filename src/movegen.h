#pragma once
#include "types.h"
#include "Board.h"


inline Move encode_move(int from, int to, int flags = 0){
    return (flags<<12) | (to<<6) | from;
}

class MoveList {
private:
    Move moves[256];
    int count;
public:
    MoveList() : count(0) {}

    inline void add(Move m) { moves[count++] = m; }

    inline int size() { return count; }
    inline Move get(int index) { return moves[index]; }
    inline const Move* begin() { return &moves[0]; }
    inline const Move* end() { return &moves[count]; }
};

MoveList generate_moves(const Board& board);