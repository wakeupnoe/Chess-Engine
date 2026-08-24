#pragma once
#include "bitboard.h"
#include "types.h"
#include "zobrist.h"
#include <array>

const int MAX_GAME_PLIES = 1024;

class Board {
private:
    Bitboard pieces[2][6]; // pieces[COLOR][PIECE]
    Bitboard colorPieces[2]; // colorPieces[COLOR]
    Bitboard allPieces;
    StateInfo currentState;
    Piece pieceOnSquare[64];
    Color sideToMove;
    std::array<StateInfo, MAX_GAME_PLIES> HistoryStack;
    int gamePly;

public:
    Board();

    void load_fen(const char* fen);
    void clear();
    template <Color Us>
    bool make_move(Move move);
    template <Color Us>
    void unmake_move(Move move);

    inline Bitboard get_pieces(Color c, Piece p) const { return pieces[c][p]; }
    inline Bitboard get_color_pieces(Color c) const { return colorPieces[c]; }
    inline Bitboard get_all_pieces() const { return allPieces; }
    inline void move_piece(Color c, Piece p, int from_sq, int to_sq) {
        Bitboard move_mask = SquareBB(from_sq) | SquareBB(to_sq);
        pieces[c][p] ^= move_mask;
        colorPieces[c] ^= move_mask;
        allPieces ^= move_mask;
        pieceOnSquare[to_sq] = p; 
        pieceOnSquare[from_sq] = NO_PIECE_TYPE;
        currentState.Zobrist_Key ^= zobrist_pieces[c][p][from_sq] ^ zobrist_pieces[c][p][to_sq];
    }
    inline void quiet_move_piece(Color c, Piece p, int from_sq, int to_sq) {
        Bitboard move_mask = SquareBB(from_sq) | SquareBB(to_sq);
        pieces[c][p] ^= move_mask;
        colorPieces[c] ^= move_mask;
        allPieces ^= move_mask;
        pieceOnSquare[to_sq] = p; 
        pieceOnSquare[from_sq] = NO_PIECE_TYPE;
    }
    inline void toggle_piece(Color c, Piece p, int sq) {
        Bitboard bb = SquareBB(sq);
        pieces[c][p] ^= bb;
        colorPieces[c] ^= bb;
        allPieces ^= bb;
        currentState.Zobrist_Key ^= zobrist_pieces[c][p][sq];
    }
    inline void quiet_toggle_piece(Color c, Piece p, int sq) {
        Bitboard bb = SquareBB(sq);
        pieces[c][p] ^= bb;
        colorPieces[c] ^= bb;
        allPieces ^= bb;
    }
    inline void add_piece(Color c, Piece p, int sq){ toggle_piece(c,p,sq); pieceOnSquare[sq] = p; }
    inline void quiet_add_piece(Color c, Piece p, int sq){ quiet_toggle_piece(c,p,sq); pieceOnSquare[sq] = p; }
    inline void remove_piece(Color c, Piece p, int sq){ toggle_piece(c,p,sq); pieceOnSquare[sq] = NO_PIECE_TYPE; }
    inline void quiet_remove_piece(Color c, Piece p, int sq){ quiet_toggle_piece(c,p,sq); pieceOnSquare[sq] = NO_PIECE_TYPE; }
    inline Piece get_piece_on_sq(int sq) const { return pieceOnSquare[sq]; }
    inline Color get_side_to_move() const { return sideToMove; }
    inline Bitboard get_en_passant_Bitboard() const { return currentState.En_Passant_Target_Square == NO_SQUARE ? 0ULL : SquareBB(currentState.En_Passant_Target_Square); }
    inline int get_castling_rights() const { return currentState.castling_rights; }
    inline int get_game_ply() const {return gamePly; }
    inline bool is_draw(){
        if(currentState.Half_move_Clock >= 100) return true;
        int limit = gamePly - currentState.Half_move_Clock;
        for(int i = gamePly - 2; i>=limit; i-=2){
            if(HistoryStack[i].Zobrist_Key == currentState.Zobrist_Key) return true;
        }
        return false;
    }
};


template <Color Us> // Color of the side that makes the move
bool Board::make_move(Move move){
    // Phase 0: Basic Variables
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    int from_sq = get_from_sq(move);
    int to_sq = get_to_sq(move);
    int flag = get_flags(move);
    Piece pieceType = pieceOnSquare[from_sq];
    Piece capturedType = pieceOnSquare[to_sq];
    
    // Phase 1: History Snapshot
    HistoryStack[gamePly++] = currentState;
    
    // Phase 2: Clock & EP Resets
    currentState.Half_move_Clock++;
    currentState.Half_move_Clock *= (pieceType != PAWN) && (capturedType == NO_PIECE_TYPE);
    currentState.Captured_Piece_Type = capturedType;
    currentState.Zobrist_Key ^= zobrist_ep[currentState.En_Passant_Target_Square];
    currentState.En_Passant_Target_Square = NO_SQUARE;
    
    // Phase 3: Base Piece Movement
    if(capturedType!=NO_PIECE_TYPE){
        remove_piece(Them,capturedType,to_sq);
    }
    move_piece(Us,pieceType,from_sq,to_sq);

    // Phase 4: Special Move Flags
    if(flag){
        if(flag & 8){ // promotion
            remove_piece(Us,PAWN,to_sq);
            Piece promoted_to = static_cast<Piece>(KNIGHT + (flag & 3)); 
            add_piece(Us,promoted_to,to_sq); // last 3 bits? ROOK, BISHOP, KNIGHT, QUEEN (in some order)  (if we ordered Pieces correctly and matched with moveencoding)
        } else if(flag == 5){ // enpassant
            int enemy_pawn_sq = to_sq + (Us == WHITE ? -8 : 8);
            remove_piece(Them,PAWN,enemy_pawn_sq);
            currentState.Captured_Piece_Type = PAWN;
        } else if(flag == 1){ // double push
            currentState.En_Passant_Target_Square = static_cast<Square>(to_sq + (Us == WHITE ? -8 : 8));
            currentState.Zobrist_Key ^= zobrist_ep[currentState.En_Passant_Target_Square]; // square indexable
        } else if(flag == 2){ // KING SIDE
            Square from_rook = Us == WHITE ? H1 : H8;           // HERE
            Square to_rook = static_cast<Square>(from_rook - 2);
            move_piece(Us,ROOK,from_rook,to_rook);
        } else if(flag == 3){ // QUEEN SIDE                     // AND HERE
            Square from_rook = Us == WHITE ? A1 : A8;
            Square to_rook = static_cast<Square>(from_rook + 3);
            move_piece(Us,ROOK,from_rook,to_rook);
        }
    }
    // Phase 5: Irreversible Board State AND Phase 6: Zobrist State Toggles
    currentState.Zobrist_Key ^= zobrist_castling[currentState.castling_rights];
    currentState.castling_rights &= CASTLING_MASK[from_sq] & CASTLING_MASK[to_sq];
    currentState.Zobrist_Key ^= zobrist_castling[currentState.castling_rights];
    currentState.Zobrist_Key ^= zobrist_side;
    Board::sideToMove = Them;

    // Phase 7: Legality Check
    Square king_sq = static_cast<Square>(lsb(get_pieces(Us, KING)));
    if(is_square_attacked<Them>(king_sq, *this)) {
        unmake_move<Us>(move);
        return false;
    }

    return true;
}

template <Color Us> // so we call with the side who did the move, right?
void Board::unmake_move(Move move){
    // Phase 0: Basic Variables
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    int from_sq = get_from_sq(move);
    int to_sq = get_to_sq(move);
    int flag = get_flags(move);
    Piece pieceType = pieceOnSquare[to_sq];
    Piece capturedType = currentState.Captured_Piece_Type;
    
    // Phase 1: Base Piece Reversal
    quiet_move_piece(Us,pieceType,to_sq,from_sq);

    // Phase 2: Special Flag Reversal
    if(flag){  // argh performance hit
        if(flag & 8){ // promotion
            Piece promoted_to = static_cast<Piece>(KNIGHT + (flag & 3)); 
            quiet_remove_piece(Us,promoted_to,from_sq);
            quiet_add_piece(Us,PAWN,from_sq);
        } else if(flag == 5){ // enpassant
            int enemy_pawn_sq = to_sq + (Us == WHITE ? -8 : 8);
            quiet_add_piece(Them,PAWN,enemy_pawn_sq);
        }else if(flag == 2){ // KING SIDE Castling
            Square from_rook = Us == WHITE ? H1 : H8;
            Square to_rook = static_cast<Square>(from_rook - 2);
            quiet_move_piece(Us,ROOK,to_rook,from_rook);
        }else if(flag == 3){ // QUEEN SIDE Castling
            Square from_rook = Us == WHITE ? A1 : A8;
            Square to_rook = static_cast<Square>(from_rook + 3);
            quiet_move_piece(Us,ROOK,to_rook,from_rook);
        }
    }

    // Phase 3: Captured Piece Restoration
    if(capturedType != NO_PIECE_TYPE && flag != 5){ // argh performance hit
        quiet_add_piece(Them, capturedType, to_sq);
    }

    // Phase 4: State Time Travel
    currentState = HistoryStack[--gamePly];
    sideToMove = Us;
}