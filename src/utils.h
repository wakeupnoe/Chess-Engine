#pragma once
#include "types.h"
#include "board.h"
#include <iostream>
#include <string>

inline std::string move_to_string(const Board& board, Move m){
    if(m==0) return "0000";

    std::string res = "";
    int from = get_from_sq(m);
    int to = get_to_sq(m);
    res += static_cast<char>('a' + (from % 8));
    res += static_cast<char>('1' + (from / 8));
    res += static_cast<char>('a' + (to % 8));
    res += static_cast<char>('1' + (to / 8));
    int flag = get_flags(m);
    if(flag >= 8){
        int promo_type = flag & 3;
        const char promo_chars[] = {'n','b','r','q'};
        res += promo_chars[promo_type];
    }
    return res;
    
}