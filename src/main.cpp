#include "AIPerft.h"
#include "attacks.h"
#include "search.h"
#include "board.h"
#include "movegen.h"
#include "utils.h"
#include "zobrist.h"
#include <iostream>


int main(){
    std::cout << "Starting Setup" << std::endl;
    init_attacks();
    zobrist_init();
    std::cout << "Setup done" << std::endl;

    Board board;
    board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // board.load_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"); // works
    // board.load_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"); // works
    // board.load_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"); //works

    TT.resize(64);

    // int test_depth = 6;
    // std::cout << "Running Perft Divide at Depth" << test_depth << "..." << std::endl;

    // Perft::divide(board,test_depth);

    // std::cout << "Perft Test finished" << std::endl;

    //std::cout << "Start running e3 bug Test" << std::endl;

    //trace_e3_bug(board);

    //std::cout << "Finished running e3 bug Test" << std::endl;
    
    Color col = WHITE;
    int depth = 15;
    bool condition = true;
    while(condition == true){
        if(col == WHITE){
            Move m = get_best_move<WHITE>(board, depth);
            if(m==0) condition = false;
            std::cout << move_to_string(board,m) << std::endl;
            board.make_move<WHITE>(m);
            col = BLACK;
        } else {
            Move m = get_best_move<BLACK>(board, depth);
            if(m==0) condition = false;
            std::cout << move_to_string(board,m) << std::endl;
            board.make_move<BLACK>(m);
            col = WHITE;
        }
    }

    return 0;
}