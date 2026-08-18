#include "AIPerft.h"
#include "attacks.h"
#include "board.h"
#include "movegen.h"
#include "zobrist.h"    
#include <iostream>

int main(){
    std::cout << "Starting Setup" << std::endl;
    init_attacks();
    zobrist_init();
    std::cout << "Setup done" << std::endl;

    Board board;
    board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    int test_depth = 3;
    std::cout << "Running Perft Divide at Depth" << test_depth << "..." << std::endl;

    Perft::divide(board,test_depth);

    std::cout << "Test finished" << std::endl;
    return 0;
}