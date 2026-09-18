#include "uci.h"
#include <iostream>
#include <sstream>

extern TimeManager TM;
extern std::atomic<bool> abort_search;

namespace UCI {
    std::thread search_thread;

    void setup_time_controle(Color Us, int wtime, int btime, int winc, int binc, int moves_to_go = 40){
        TM.time_control_enabled = true;
        TM.start_time = std::chrono::steady_clock::now();
        int time_left = (Us == WHITE) ? wtime : btime;
        int inc = (Us == WHITE) ? winc : binc;
        time_left = std::max(1, time_left - 50);
        if(moves_to_go == 0) moves_to_go = 40;

        int base_time = time_left / moves_to_go;
        TM.soft_limit = base_time + (inc * 3 / 4);
        TM.hard_limit = std::max(1, (base_time*3 + inc));
        TM.hard_limit = std::min(TM.hard_limit, time_left);
    }
    void loop(){
        std::string line;
        Board board;
        board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        while(std::getline(std::cin, line)){
            if(line.empty()) continue;
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            if(token == "quit"){
                abort_search = true;
                if(search_thread.joinable()) search_thread.join();
                break;
            } else if(token == "stop"){
                abort_search = true;
            } else if(token == "uci"){
                std::cout << "id name Chess-Engine" << std::endl;
                std::cout << "id author ms1000" << std::endl;
                std::cout << "uciok" << std::endl;   
            } else if(token == "isready"){
                std::cout << "readyok" << std::endl;
            } else if(token == "ucinewgame"){
                    TT.clear();
                    clear_history_heuristic();
            } else if(token == "position"){
                iss >> token;
                if(token == "startpos"){
                    board.load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                    iss >> token;
                } else if(token == "fen"){
                    std::string fen = "";
                    while(iss >> token && token != "moves"){
                        fen += token + " ";
                    }
                    board.load_fen(fen);
                }
                if(token == "moves"){
                    while(iss >> token){

                        Move move = (board.get_side_to_move() == WHITE) ? parse_move<WHITE>(board, token) : parse_move<BLACK>(board, token);
                        if(move != 0){
                            if(board.get_side_to_move() == WHITE){
                                board.make_move<WHITE>(move);
                            } else {
                                board.make_move<BLACK>(move);
                            }
                        }
                    }
                }
            } else if(token == "go"){
                if(search_thread.joinable()) search_thread.join();

                int depth = MAX_PLY;
                int wtime = 0, btime = 0, winc = 0, binc = 0, moves_to_go = 0;
                int movetime = 0;
                while(iss >> token){
                    if(token == "depth") iss >> depth;
                    else if (token == "wtime") iss >> wtime;
                    else if (token == "btime") iss >> btime;
                    else if (token == "winc") iss >> winc;
                    else if (token == "binc") iss >> binc;
                    else if (token == "movestogo") iss >> moves_to_go;
                    else if (token == "movetime") iss >> movetime;
                }
                if(movetime > 0){
                    TM.time_control_enabled = true;
                    TM.start_time = std::chrono::steady_clock::now();
                    TM.hard_limit = std::max(1,movetime - 50);
                    TM.soft_limit = TM.hard_limit;
                } else if(wtime > 0 && btime > 0){
                    setup_time_controle(board.get_side_to_move(),wtime,btime,winc,binc,moves_to_go);
                } else {
                    TM.time_control_enabled = false;
                }
                //TEMP till we implement time management:
                /*if(depth == MAX_PLY && (wtime > 0 || btime > 0 || movetime > 0)){
                    depth = 15;
                }*/
                abort_search = false;
                search_thread = std::thread([board, depth]() mutable{
                    Move best_move = (board.get_side_to_move() == WHITE) ? get_best_move<WHITE>(board, depth) : get_best_move<BLACK>(board, depth);
                    std::cout << "bestmove " << move_to_string(board, best_move) << std::endl;
                });
            }
        }
    }
}