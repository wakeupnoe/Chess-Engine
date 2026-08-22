#include "AIPerft.h"
#include "movegen.h"

#include <iostream>
#include <chrono> 

namespace Perft {

    // --- Templated Driver ---
    template <Color Us>
    uint64_t perft_driver(Board& board, int depth) {
        if (depth == 0) {
            return 1ULL;
        }

        constexpr Color Them = (Us == WHITE) ? BLACK : WHITE;
        uint64_t nodes = 0;

        MoveList list;
        list = generate_moves<Us>(board); 

        for (int i = 0; i < list.size(); i++) { 
            Move move = list.get(i);

            if (board.make_move<Us>(move)) {
                nodes += perft_driver<Them>(board, depth - 1);
                board.unmake_move<Us>(move);
            }
        }

        return nodes;
    }

    // --- Main Perft Call ---
    uint64_t execute(Board& board, int depth) {
        if (board.get_side_to_move() == WHITE) {
            return perft_driver<WHITE>(board, depth);
        } else {
            return perft_driver<BLACK>(board, depth);
        }
    }

    // --- Divide (The Debugging Tool) ---
    void divide(Board& board, int depth) {
        if (depth == 0) return;

        std::cout << "--- Perft Divide Depth " << depth << " ---" << std::endl;

        auto start_time = std::chrono::high_resolution_clock::now();
        uint64_t total_nodes = 0;
        MoveList list;
        
        if (board.get_side_to_move() == WHITE) {
            //std::cout << "DEBUG: Generating root moves for WHITE..." << std::endl;
            list = generate_moves<WHITE>(board);
            //std::cout << "DEBUG: Generated " << list.size() << " moves!" << std::endl;
            
            for (int i = 0; i < list.size(); i++) {
                Move move = list.get(i);
                //std::cout << "DEBUG: Trying to make move index " << i << "..." << std::endl;
                
                if (board.make_move<WHITE>(move)) {
                    //std::cout << "DEBUG: Move " << i << " made successfully. Searching deeper..." << std::endl;
                    uint64_t nodes = perft_driver<BLACK>(board, depth - 1);
                    total_nodes += nodes;
                    
                    //std::cout << "DEBUG: Search finished for move " << i << ". Unmaking..." << std::endl;
                    board.unmake_move<WHITE>(move);
                    //std::cout << "DEBUG: Unmake successful for move " << i << "." << std::endl;
                    
                    std::cout << "Move " << i << ": " << nodes << " nodes\n";
                } else {
                    //std::cout << "DEBUG: Move " << i << " was illegal (King left in check)." << std::endl;
                }
            }
        } else {
            //std::cout << "DEBUG: Generating root moves for BLACK..." << std::endl;
            list = generate_moves<BLACK>(board);
            //std::cout << "DEBUG: Generated " << list.size() << " moves!" << std::endl;
            
            for (int i = 0; i < list.size(); i++) {
                Move move = list.get(i);
                //std::cout << "DEBUG: Trying to make move index " << i << "..." << std::endl;
                
                if (board.make_move<BLACK>(move)) {
                    //std::cout << "DEBUG: Move " << i << " made successfully. Searching deeper..." << std::endl;
                    uint64_t nodes = perft_driver<WHITE>(board, depth - 1);
                    total_nodes += nodes;
                    
                    //std::cout << "DEBUG: Search finished for move " << i << ". Unmaking..." << std::endl;
                    board.unmake_move<BLACK>(move);
                    //std::cout << "DEBUG: Unmake successful for move " << i << "." << std::endl;
                    
                    std::cout << "Move " << i << ": " << nodes << " nodes\n";
                } else {
                    //std::cout << "DEBUG: Move " << i << " was illegal (King left in check)." << std::endl;
                }
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;

        std::cout << "\nTotal Nodes: " << total_nodes << std::endl;
        std::cout << "Time elapsed: " << elapsed.count() << " seconds" << std::endl;
        
        if (elapsed.count() > 0) {
            std::cout << "NPS: " << static_cast<uint64_t>(total_nodes / elapsed.count()) << " nodes/sec\n";
        }
        std::cout << "---------------------------" << std::endl;
    }
}