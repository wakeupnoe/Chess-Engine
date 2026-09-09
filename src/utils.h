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

// AI GENERATED HELPER DEBUG FUNCTION
inline std::string print_board(const Board& board) {
    const char piece_chars[2][7] = {
        {'P', 'N', 'B', 'R', 'Q', 'K', '.'}, // White (0)
        {'p', 'n', 'b', 'r', 'q', 'k', '.'}  // Black (1)
    };

    std::string res;

    res += "  a b c d e f g h\n";

    for (int rank = 7; rank >= 0; --rank) {
        res += std::to_string(rank + 1);
        res += " ";

        for (int file = 0; file < 8; ++file) {
            int sq = rank * 8 + file;
            Piece p = board.get_piece_on_sq(static_cast<Square>(sq));

            if (p == NO_PIECE_TYPE) {
                res += ". ";
            } else {
                Color c = (board.get_color_pieces(WHITE) & (1ULL << sq))
                              ? WHITE
                              : BLACK;

                res += piece_chars[c][p];
                res += " ";
            }
        }

        res += std::to_string(rank + 1);
        res += "\n";
    }

    res += "  a b c d e f g h\n";

    return res;
}