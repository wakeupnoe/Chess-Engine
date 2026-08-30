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
    init_lmr_table();
    TT.resize(64);
    std::cout << "Setup done" << std::endl;

    Board board;
    // board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // board.load_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"); // works
    // board.load_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"); // works
    // board.load_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"); //works

    // board.load_fen("5rk1/ppq3pp/2nNR3/2p2Q2/3p3b/3P4/PPP2PPP/6K1 w - - 5 24"); solved??
    // board.load_fen("8/3P1Q2/3q2pk/5P1p/p6P/Pp6/1P1r4/KR6 b - - 0 47");
    //board.load_fen("4r2k/1p3rbp/2p1N1pn/p3n3/P2NB3/1P4q1/4R1P1/B1Q2RK1 b - - 4 32"); // solved!
    //Color col = BLACK;
    
    // board.load_fen("2rn1rk1/p1q1p1p1/1p2b3/4N2Q/1n2N3/P2B4/1PP2PPP/R2K3R w - - 4 13");
    // Color col = WHITE;

    board.load_fen("8/1pB1rnbk/6pn/7q/P3B2P/1P6/6P1/2Q1R2K b - - 0 34"); // Mate in 10 - solved?
    Color col = BLACK;
    
    
    // int test_depth = 6;
    // std::cout << "Running Perft Divide at Depth" << test_depth << "..." << std::endl;

    // Perft::divide(board,test_depth);

    // std::cout << "Perft Test finished" << std::endl;

    //std::cout << "Start running e3 bug Test" << std::endl;

    //trace_e3_bug(board);

    //std::cout << "Finished running e3 bug Test" << std::endl;
    
    
    int depth = 16;
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