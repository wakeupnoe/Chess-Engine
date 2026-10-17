#include "uci.h"
#include <iostream>
#include <sstream>
#include <vector>

extern TimeManager TM;
extern std::atomic<bool> abort_search;

namespace UCI {
    std::vector<std::thread> search_threads;
    std::vector<SearchWorker> workers;
    int num_threads = 8;

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
                for(auto& t : search_threads){
                    if(t.joinable()) t.join();
                }
                search_threads.clear();
                break;
            } else if(token == "stop"){
                abort_search = true;
            } else if(token == "uci"){
                std::cout << "id name Chess-Engine" << std::endl;
                std::cout << "id author ms1000" << std::endl;
                std::cout << "uciok" << std::endl;   
            } else if(token == "isready"){
                std::cout << "readyok" << std::endl;
            } else if(token == "eval"){
                Accumulator local_acc;
                refresh_accumulators(board, local_acc);
                Color stm = board.get_side_to_move();
                int score_stm = 0;
                if(stm == WHITE){
                    score_stm = nnue_evaluate<WHITE>(local_acc);
                } else {
                    score_stm = nnue_evaluate<BLACK>(local_acc);
                }
                int score_white = (stm == WHITE) ? score_stm : - score_stm;
                std::cout << "----------------------------------------" << std::endl;
                std::cout << "Side to move : " << (stm == WHITE ? "White" : "Black") << std::endl;
                std::cout << "NNUE (stm)   : " << score_stm << " cp (" 
                        << std::showpos << (score_stm / 100.0) << " pawns)" << std::noshowpos << std::endl;
                std::cout << "NNUE (white) : " << score_white << " cp (" 
                        << std::showpos << (score_white / 100.0) << " pawns)" << std::noshowpos << std::endl;
                std::cout << "----------------------------------------" << std::endl;
            } else if(token == "ucinewgame"){
                    TT.clear();
                    for(int i = 0; i<workers.size(); i++){
                        workers[i].clear_histories();
                    }
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
                        Accumulator dummy_accs[2];
                        if(move != 0){
                            if(board.get_side_to_move() == WHITE){
                                board.make_move<WHITE>(move, dummy_accs[0], dummy_accs[1]);
                            } else {
                                board.make_move<BLACK>(move, dummy_accs[0], dummy_accs[1]);
                            }
                        }
                    }
                }
            } else if(token == "go"){
                abort_search = true;
                for(auto& t : search_threads){
                    if(t.joinable()) t.join();
                }
                search_threads.clear();

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
                TM.start_time = std::chrono::steady_clock::now();
                if(movetime > 0){
                    TM.time_control_enabled = true;
                    TM.hard_limit = std::max(1,movetime - 50);
                    TM.soft_limit = TM.hard_limit;
                } else if(wtime > 0 || btime > 0){
                    setup_time_controle(board.get_side_to_move(),wtime,btime,winc,binc,moves_to_go);
                } else {
                    TM.time_control_enabled = false;
                }
                
                abort_search = false;
                nodes_searched = 0;
                if(workers.size()<num_threads){
                    workers.resize(num_threads);
                }

                for(int i = 0; i<num_threads; i++){
                    search_threads.emplace_back([board, depth, i]() mutable{
                        workers[i].board = board;
                        workers[i].thread_id = i;
                        Move best_move = (board.get_side_to_move() == WHITE) ? workers[i].get_best_move<WHITE>(depth) : workers[i].get_best_move<BLACK>(depth);
                        if(i == 0) std::cout << "bestmove " << move_to_string(board, best_move) << std::endl;
                    });
                }
            }
        }
    }
}