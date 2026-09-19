#pragma once
#include "types.h"
#include "board.h"
#include "tt.h"
#include "search.h"


namespace UCI {
    void loop();
}


template <Color Us>
Move parse_move(const Board& board, const std::string& move_str){
    MoveList list;
    MoveList caps = generate_captures<Us>(board);
    MoveList quiets = generate_quiets<Us>(board);
    for(int i = 0; i<caps.size(); i++) list.add(caps.get(i));
    for(int i = 0; i<quiets.size(); i++) list.add(quiets.get(i));
    for(int i = 0; i<list.size(); i++){
        Move move = list.get(i);
        if(move_to_string(board,move) == move_str){
            bool is_legal = false;
            Board copy = board;
            Accumulator dummy_accs[2];
            is_legal = copy.make_move<Us>(move,dummy_accs[0], dummy_accs[1]);
            if(is_legal) return move;
        }
    }
    return 0;
}