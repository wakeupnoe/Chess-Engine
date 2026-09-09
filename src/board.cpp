#include "board.h"
#include <sstream>
#include <cstring>
#include <cassert>


namespace {
    // CONTRACT [FEN Parsing Helpers]:
    // - ASSUMES: FEN strings passed are semi-valid.
    // - GUARANTEES: Will fail loudly via assert if FEN contains invalid piece chars or out-of-bounds EP squares.

    void parse_piece_char(char c, Color& col, Piece& p){
        col = islower(c) ? BLACK : WHITE;
        switch (tolower(c)) {
            case 'p' : p = PAWN; break;
            case 'n' : p = KNIGHT; break;
            case 'r' : p = ROOK; break;
            case 'b' : p = BISHOP; break;
            case 'k' : p = KING; break;
            case 'q' : p = QUEEN; break;
            default: 
                assert(false && "parse_piece_char: Invalid piece character in FEN!");
                p = NO_PIECE_TYPE; // Fallback
                break;
        }
    }


    void parseEnPassantSquare(std::string target, StateInfo& current){
        if(size(target)==2){
            int file = target[0] - 'a';
            int rank = target[1] - '1'; // fixed form '0' to '1'

            assert(file >= 0 && file <= 7 && "parseEnPassantSquare: File out of bounds!");
            assert(rank >= 0 && rank <= 7 && "parseEnPassantSquare: Rank out of bounds!");

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

// CONTRACT [FEN Parsing]:
// - ASSUMES: FEN string is syntactically valid.
// - GUARANTEES: File and Rank pointers will never exceed physical board bounds (0-7), 
//   preventing pieceOnSquare buffer overflows on malformed inputs.
void Board::load_fen(const std::string& fen){
    clear();

    std::istringstream ss(fen);
    std::string pieces_str, color_str, castling_str, en_passant;
    int half_move = 0, full_move = 1; // default values if fen is truncated
    ss >> pieces_str >> color_str >> castling_str >> en_passant >> half_move >> full_move;
    
    int rank = 7, file = 0;
    for (char c : pieces_str){
        if(c=='/'){
            rank--;
            file = 0;
            assert(rank >= 0 && "load_fen: Rank underflow (too many slashes in FEN)!");
        } else if(isdigit(c)){
            file += c-'0';
            assert(file <= 8 && "load_fen: File overflow (number too large in FEN)!");
        } else {
            assert(rank >= 0 && rank <= 7 && "load_fen: Rank out of bounds!");
            assert(file >= 0 && file < 8 && "load_fen: File overflow (too many pieces on rank)!");
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

    assert(gamePly < MAX_GAME_PLIES && "load_fen: Starting full_move too large for HistoryStack!");

    currentState.Zobrist_Key = generate_zobrist_key();
}


// CONTRACT [Zobrist Generation]:
// - ASSUMES: pieceOnSquare array contains strictly valid pieces (0-5) or NO_PIECE_TYPE (6).
// - ASSUMES: En_Passant_Target_Square is strictly 0-63 or NO_SQUARE (64).
// - ASSUMES: castling_rights is strictly 0-15.
// - GUARANTEES: A ground-truth calculation of the Zobrist key directly from the physical board state.
uint64_t Board::generate_zobrist_key() const {
    uint64_t key = 0ULL;

    for(int sq = 0; sq < 64; sq++){
        Piece p = pieceOnSquare[sq];
        if(p!=NO_PIECE_TYPE){
            assert(p >= PAWN && p <= KING && "generate_zobrist_key: Garbage piece ID found on square!");

            Color col = (pieces[WHITE][p] & SquareBB(sq)) ? WHITE : BLACK;
            key ^= zobrist_pieces[col][p][sq];
        }
    }
    assert((sideToMove == WHITE || sideToMove == BLACK) && "generate_zobrist_key: Invalid sideToMove!");
    if(sideToMove == BLACK) key ^= zobrist_side;
    assert(currentState.En_Passant_Target_Square <= NO_SQUARE && "generate_zobrist_key: EP square out of bounds!");
    key ^= zobrist_ep[currentState.En_Passant_Target_Square];
    assert(currentState.castling_rights < 16 && "generate_zobrist_key: Castling rights out of bounds!");
    key ^= zobrist_castling[currentState.castling_rights];
    return key;
}