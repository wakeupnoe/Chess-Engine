#include "search.h"
#include <cmath>


std::atomic<bool> abort_search = false;

int LMR_table[64][64];

void init_lmr_table(){
    for(int depth = 0; depth < 64; depth++){
        for(int moves = 0; moves < 64; moves++){
            if(depth >= 2 && moves >= 4){
                double reduction = 0.75 + std::log(depth) * std::log(moves) / 2.25;
                LMR_table[depth][moves] = static_cast<int>(reduction);
            } else {
                LMR_table[depth][moves] = 0;
            }
        }
    }
}