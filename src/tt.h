#pragma once
#include "types.h"


inline int score_to_tt(int16_t score, int ply){
    if(score >=   MATE_SCORE - 100) return score + ply;
    if(score <= - MATE_SCORE + 100) return score - ply;
    return score;
}

inline int16_t score_from_tt(int score, int ply){
    if(score >=   MATE_SCORE - 100) return score - ply;
    if(score <= - MATE_SCORE + 100) return score + ply;
    return score;
}


class TranspositionTable{
private:
TTEntry* table = nullptr;
size_t num_entries = 0;

public:
TranspositionTable() = default;
~TranspositionTable() { delete[] table; }

void resize(size_t mb);
void clear();

    inline TTEntry* probe(uint64_t key){
        TTEntry* entry = &table[key & (num_entries -1)];
        if(entry->key == key) return entry;
        return nullptr;
    }
    inline void store(uint64_t key, Move move, int16_t score, uint8_t depth, TTFlag flag, uint8_t age){
        TTEntry* cur = &table[key & (num_entries -1)];
        if(cur->key != key || cur->age != age || cur->depth <= depth) {
            cur->key = key;
            cur->move = move;
            cur->score = score;
            cur->depth = depth;
            cur->flag = flag;
            cur->age = age;
        }
    }
    
};

extern TranspositionTable TT;