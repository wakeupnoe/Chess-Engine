#include "tt.h"
#include <cstring>

TranspositionTable TT;

void TranspositionTable::resize(size_t mb){
    delete[] table;
    table = nullptr;
    size_t target_entries = (mb * 1024 * 1024) / sizeof(TTEntry);
    
    num_entries = 1;
    while(num_entries <target_entries) num_entries *= 2;

    table = new TTEntry[num_entries]();
    clear();
};

void TranspositionTable::clear(){
    if(table != nullptr){
        std::memset(table, 0, num_entries * sizeof(TTEntry));
    }
};