#include "AIPerft.h"
#include "attacks.h"
#include "board.h"
#include "movegen.h"
#include "zobrist.h"    
#include <iostream>

void trace_e3_bug(Board& board) {
    std::cout << "\n--- TRACING 1. e3 BUG ---" << std::endl;
    
    // 1. Play 1. e3
    // Assuming E2 is 12 and E3 is 20 based on standard A1=0 mapping
    Move e3 = encode_move(12, 20, 0); 
    board.make_move<WHITE>(e3);
    
    // 2. Play 1... a6
    // Assuming A7 is 48 and A6 is 40
    Move a6 = encode_move(48, 40, 0); 
    board.make_move<BLACK>(a6);
    
    // 3. Generate White's Ply 3 Moves
    MoveList list;
    list = generate_moves<WHITE>(board);
    
    std::cout << "Generated " << list.size() << " pseudo-legal moves for White.\n";
    
    int legal_count = 0;
    for(int i = 0; i < list.size(); i++) {
        Move m = list.get(i);
        int from = get_from_sq(m);
        int to = get_to_sq(m);
        
        // Print the move we are trying
        std::cout << "Trying move: " << from << " -> " << to;
        
        // If it targets e2 (index 12), flag it so we can see it clearly!
        if (to == 12) std::cout << "  <-- TARGETS E2!";
        
        if (board.make_move<WHITE>(m)) {
            std::cout << " (LEGAL)\n";
            legal_count++;
            board.unmake_move<WHITE>(m);
        } else {
            std::cout << " (ILLEGAL - King in check!)\n";
        }
    }
    
    std::cout << "\nTotal LEGAL moves found: " << legal_count << " (Expected ~29-30)" << std::endl;
}

int main(){
    std::cout << "Starting Setup" << std::endl;
    init_attacks();
    zobrist_init();
    std::cout << "Setup done" << std::endl;

    Board board;
    // board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    // board.load_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"); // works
    // board.load_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"); // works
    board.load_fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"); //works


    int test_depth = 6;
    std::cout << "Running Perft Divide at Depth" << test_depth << "..." << std::endl;

    Perft::divide(board,test_depth);

    std::cout << "Perft Test finished" << std::endl;

    //std::cout << "Start running e3 bug Test" << std::endl;

    //trace_e3_bug(board);

    //std::cout << "Finished running e3 bug Test" << std::endl;
    
    return 0;
}