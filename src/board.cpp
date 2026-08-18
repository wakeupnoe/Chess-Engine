#include "board.h"
#include <sstream>
#include <string>
#include <cstring>


namespace {
    void parse_piece_char(char c, Color& col, Piece& p){
        col = islower(c) ? BLACK : WHITE;
        switch (tolower(c)) {
            case 'p' : p = PAWN; break;
            case 'n' : p = KNIGHT; break;
            case 'r' : p = ROOK; break;
            case 'b' : p = BISHOP; break;
            case 'k' : p = KING; break;
            case 'q' : p = QUEEN; break;
        }
    }

    void parseEnPassantSquare(std::string target, StateInfo& current){
        if(size(target)==2){
            int file = target[0] - 'a';
            int rank = target[1] - '0';
            current.En_Passant_Target_Square = static_cast<Square>(rank*8+file);
        } else current.En_Passant_Target_Square = NO_SQUARE;
    }

    void parseCastlingRights(const std::string& rights,  StateInfo& current){
        current.castling_rights = 0;
        for(char c : rights){
            switch(c){
                case 'K' : current.castling_rights|=WK; break;
                case 'Q' : current.castling_rights|=WQ; break;
                case 'k' : current.castling_rights|=BK; break;
                case 'q' : current.castling_rights|=BQ; break;
                default : break;
            }
        }
    }
}

void Board::clear() {
    std::memset(pieces,0, sizeof(pieces));
    std::memset(colorPieces,0, sizeof(colorPieces));
    allPieces = 0ULL;
    currentState = StateInfo(); // StateInfo() is the default constructor for StateInfo and should generate the StateInfo of the starrting position 
    // HistroyStack.fill(StateInfo()); not needed since gamePly=0 means that nothing of value is in the historystack.
    std::fill(std::begin(pieceOnSquare), std::end(pieceOnSquare), NO_PIECE_TYPE);
    gamePly = 0;
}


Board::Board() {
    clear();
}

void Board::load_fen(const char* fen){
    clear();

    std::istringstream ss(fen);
    std::string pieces_str, color_str, castling_str, en_passant;
    int half_move, full_move;
    ss >> pieces_str >> color_str >> castling_str >> en_passant >> half_move >> full_move;
    
    int rank = 7, file = 0;
    for (char c : pieces_str){
        if(c=='/'){
            rank--;
            file = 0;
        } else if(isdigit(c)){
            file += c-'0';
        } else {
            Color col;
            Piece p;
            parse_piece_char(c,col,p);
            Square sq = static_cast<Square>(rank * 8 + file);
            pieceOnSquare[sq] = p;
            
            SetBit(pieces[col][p], sq);
            SetBit(colorPieces[col], sq);
            SetBit(allPieces, sq);

            file++;
        }
    }
    sideToMove = (color_str == "w") ? WHITE : BLACK;
    parseCastlingRights(castling_str, currentState);
    parseEnPassantSquare(en_passant,currentState);
    currentState.Half_move_Clock = half_move;
    gamePly = 2*(full_move-1) + (sideToMove == BLACK ? 1 : 0);
}
