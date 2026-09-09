#pragma once
#include "types.h"
#include <cstddef>
#include <cassert>
#include <algorithm>

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

// CONTRACT [Transposition Table]:
// - ASSUMES: resize() is called at least once before probe() or store() are used.
// - ASSUMES: num_entries is ALWAYS a power of 2 (required for bitwise modulo indexing).
// - GUARANTEES: Fast, crash-free hash indexing and score/ply conversions.

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
        // CONTRACT ENFORCEMENT
        assert(table != nullptr && "TT probe called before table allocated!");
        assert(num_entries > 0 && (num_entries & (num_entries - 1)) == 0 && "TT num_entries not a power of 2!");

        TTEntry* entry = &table[key & (num_entries -1)];
        if(entry->key == key) return entry;
        return nullptr;
    }
    inline void store(uint64_t key, Move move, int16_t score, uint8_t depth, TTFlag flag, uint8_t age){
        TTEntry* cur = &table[key & (num_entries -1)];
        // CONTRACT ENFORCEMENT
        assert(table != nullptr && "TT store called before table allocated!");

        if(cur->key != key || cur->age != age || cur->depth <= depth) {
            cur->key = key;
            cur->move = move;
            cur->score = score;
            cur->depth = depth;
            cur->flag = flag;
            cur->age = age;
        }
    }
    inline int get_hashfull() const {
        int active_entries = 0;
        size_t limit = std::min(size_t{1000}, num_entries);
        for(int i = 0; i<limit; i++){
            if(table[i].flag!=TT_NONE) active_entries++;
        }
        return active_entries;
    }
};

extern TranspositionTable TT;