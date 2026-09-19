#include "attacks.h"
#include "tt.h"
#include "uci.h"
#include "search.h"
#include "zobrist.h"
#include "nnue.h"
#include <iostream>

int main(){
    init_attacks();
    zobrist_init();
    init_lmr_table();
    TT.resize(512); // 256 512 64 increased to see effect on multithreading performance

    load_nnue("C:/dev/Chess-Engine/src/nn-82215d0fd0df.nnue");
    UCI::loop();
    return 0;
}