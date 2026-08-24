#pragma once
#include "types.h"


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