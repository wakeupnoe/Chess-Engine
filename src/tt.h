#pragma once
#include "types.h"
#include <cstddef>
#include <cassert>
#include <atomic>
#include <algorithm>

// updated for multithreading
struct TTEntry{
    std::atomic<uint64_t> key_xor_data;
    std::atomic<uint64_t> data;
};

inline uint64_t pack_tt_data(Move move, int16_t score, uint8_t depth, TTFlag flag, uint8_t age){
    return (static_cast<uint64_t>(move)) |
           (static_cast<uint64_t>(static_cast<uint16_t>(score)) << 16) |
           (static_cast<uint64_t>(depth) << 32) |
           (static_cast<uint64_t>(flag) << 40) |
           (static_cast<uint64_t>(age) << 48);
}

inline void unpack_tt_data(uint64_t data, Move& move, int16_t& score, uint8_t& depth, TTFlag& flag, uint8_t& age){
    move = static_cast<Move>(data & 0xFFFF);
    score = static_cast<int16_t>((data >> 16) & 0xFFFF);
    depth = static_cast<uint8_t>((data >> 32) & 0xFF);
    flag = static_cast<TTFlag>((data >> 40) & 0xFF);
    age = static_cast<uint8_t>((data >> 48) & 0xFF);   
}

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

    inline bool probe(uint64_t key, Move& move, int16_t& score, uint8_t& depth, TTFlag& flag, uint8_t& age){
        // CONTRACT ENFORCEMENT
        assert(table != nullptr && "TT probe called before table allocated!");
        assert(num_entries > 0 && (num_entries & (num_entries - 1)) == 0 && "TT num_entries not a power of 2!");

        TTEntry* entry = &table[key & (num_entries -1)];
        uint64_t d = entry->data.load(std::memory_order_relaxed);
        uint64_t k_xor_d = entry->key_xor_data.load(std::memory_order_relaxed);
        if((d ^ k_xor_d) == key){
            unpack_tt_data(d, move, score, depth, flag, age);
            return true;
        }
        return false;
    }

    inline void store(uint64_t key, Move move, int16_t score, uint8_t depth, TTFlag flag, uint8_t age){
        TTEntry* cur = &table[key & (num_entries -1)];
        // CONTRACT ENFORCEMENT
        assert(table != nullptr && "TT store called before table allocated!");

        // current data extraction
        uint64_t c_data = cur->data.load(std::memory_order_relaxed);
        uint64_t c_key_xor_data = cur->key_xor_data.load(std::memory_order_relaxed);

        uint64_t c_key = c_data ^ c_key_xor_data;
        uint8_t c_depth = static_cast<uint8_t>((c_data >> 32) & 0xFF);
        uint8_t c_age = static_cast<uint8_t>((c_data >> 48) & 0xFF);
        
        if(c_key != key || c_age != age || c_depth <= depth) {
            uint64_t n_data = pack_tt_data(move, score, depth, flag, age);
            uint64_t n_key_xor_data = n_data ^ key;
            cur->data.store(n_data, std::memory_order_relaxed);
            cur->key_xor_data.store(n_key_xor_data, std::memory_order_relaxed);
            
        }
    }
    inline int get_hashfull() const {
        int active_entries = 0;
        size_t limit = std::min(size_t{1000}, num_entries);
        for(int i = 0; i<limit; i++){
            uint64_t d = table[i].data.load(std::memory_order_relaxed);
            TTFlag flag = static_cast<TTFlag>((d>>40) & 0xFF);
            if(flag!=TT_NONE) active_entries++;
        }
        return active_entries;
    }
};

extern TranspositionTable TT;