#pragma once
#include "types.h"
#include "board.h"

#include <string>

inline std::string move_to_string(Board& board, Move m){
    std::string res = "";
    int from = get_from_sq(m);
    int to = get_to_sq(m);
    res += static_cast<char>('a' + (from % 8));
    res += static_cast<char>('1' + (from / 8));

    res += static_cast<char>('a' + (to % 8));
    res += static_cast<char>('1' + (to / 8));

    return res;
    
}