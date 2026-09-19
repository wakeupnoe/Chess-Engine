#include "tt.h"
#include <cstring>

TranspositionTable TT;

void TranspositionTable::resize(size_t mb){
    delete[] table;
    table = nullptr;
    size_t target_entries = (mb * 1024 * 1024) / sizeof(TTEntry);
    if(target_entries == 0) target_entries = 1;
    num_entries = 1;
    while(num_entries * 2 <= target_entries) num_entries *= 2;

    table = new TTEntry[num_entries]();
    clear();
};

void TranspositionTable::clear(){
    if(table != nullptr){
        for(size_t i = 0; i<num_entries; i++){
            table[i].data.store(0, std::memory_order_relaxed);
            table[i].key_xor_data.store(0, std::memory_order_relaxed);
        }
    }
};