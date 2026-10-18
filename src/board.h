#pragma once
#include "bitboard.h"
#include "types.h"
#include "zobrist.h"
#include <array>
#include <string>
#include <cassert>


// Forward declare the NNUE update function so the template knows it exists
struct Accumulator; // Forward declaration
void update_accumulator_piece(Accumulator& acc, Color c, Piece p, Square sq, int weight, Square w_king, Square b_king);

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

    void load_fen(const std::string& fen);
    uint64_t generate_zobrist_key() const;
    void clear();
    template <Color Us>
    bool make_move(Move move, const Accumulator& prev_acc, Accumulator& next_acc);
    template <Color Us>
    void unmake_move(Move move);
    template <Color Us>
    void make_null_move(const Accumulator& prev_acc, Accumulator& next_acc);
    template <Color Us>
    void unmake_null_move();

    inline Bitboard get_pieces(Color c, Piece p) const { assert(c == WHITE || c == BLACK); assert(p >= PAWN && p <= KING); return pieces[c][p]; }
    inline Bitboard get_color_pieces(Color c) const { assert(c == WHITE || c == BLACK); return colorPieces[c]; }
    inline Bitboard get_all_pieces() const { return allPieces; }
    inline void move_piece(Color c, Piece p, int from_sq, int to_sq) {
        assert(c == WHITE || c == BLACK); assert(p >= PAWN && p <= KING);
        assert(from_sq >= A1 && from_sq <= H8); assert(to_sq >= A1 && to_sq <= H8);
        Bitboard move_mask = SquareBB(from_sq) | SquareBB(to_sq);
        pieces[c][p] ^= move_mask;
        colorPieces[c] ^= move_mask;
        allPieces ^= move_mask;
        pieceOnSquare[to_sq] = p; 
        pieceOnSquare[from_sq] = NO_PIECE_TYPE;
        currentState.Zobrist_Key ^= zobrist_pieces[c][p][from_sq] ^ zobrist_pieces[c][p][to_sq];
    }
    inline void quiet_move_piece(Color c, Piece p, int from_sq, int to_sq) {
        assert(c == WHITE || c == BLACK); assert(p >= PAWN && p <= KING);
        assert(from_sq >= A1 && from_sq <= H8); assert(to_sq >= A1 && to_sq <= H8);

        Bitboard move_mask = SquareBB(from_sq) | SquareBB(to_sq);
        pieces[c][p] ^= move_mask;
        colorPieces[c] ^= move_mask;
        allPieces ^= move_mask;
        pieceOnSquare[to_sq] = p; 
        pieceOnSquare[from_sq] = NO_PIECE_TYPE;
    }
    inline void toggle_piece(Color c, Piece p, int sq) {
        assert(c == WHITE || c == BLACK); assert(p >= PAWN && p <= KING); assert(sq >= A1 && sq <= H8);

        Bitboard bb = SquareBB(sq);
        pieces[c][p] ^= bb;
        colorPieces[c] ^= bb;
        allPieces ^= bb;
        currentState.Zobrist_Key ^= zobrist_pieces[c][p][sq];
    }
    inline void quiet_toggle_piece(Color c, Piece p, int sq) {
        assert(c == WHITE || c == BLACK); assert(p >= PAWN && p <= KING); assert(sq >= A1 && sq <= H8);

        Bitboard bb = SquareBB(sq);
        pieces[c][p] ^= bb;
        colorPieces[c] ^= bb;
        allPieces ^= bb;
    }
    inline void add_piece(Color c, Piece p, int sq){ toggle_piece(c,p,sq); pieceOnSquare[sq] = p; }
    inline void quiet_add_piece(Color c, Piece p, int sq){ quiet_toggle_piece(c,p,sq); pieceOnSquare[sq] = p; }
    inline void remove_piece(Color c, Piece p, int sq){ toggle_piece(c,p,sq); pieceOnSquare[sq] = NO_PIECE_TYPE; }
    inline void quiet_remove_piece(Color c, Piece p, int sq){ quiet_toggle_piece(c,p,sq); pieceOnSquare[sq] = NO_PIECE_TYPE; }
    inline Piece get_piece_on_sq(int sq) const { assert(sq >= A1 && sq <= H8); return pieceOnSquare[sq]; }
    inline Color get_side_to_move() const { return sideToMove; }
    inline Bitboard get_en_passant_Bitboard() const { return currentState.En_Passant_Target_Square == NO_SQUARE ? 0ULL : SquareBB(currentState.En_Passant_Target_Square); }
    inline int get_castling_rights() const { return currentState.castling_rights; }
    inline int get_game_ply() const {return gamePly; }
    inline uint64_t get_zobrist_key() const { return currentState.Zobrist_Key; }
    inline bool is_draw(){
        if(currentState.Half_move_Clock >= 100) return true;
        int limit = gamePly - currentState.Half_move_Clock;
        assert(limit >= 0 && "Half-move clock exceeds total game plies!");
        for(int i = gamePly - 2; i>=limit; i-=2){
            if(HistoryStack[i].Zobrist_Key == currentState.Zobrist_Key) return true;
        }
        return false;
    }
};

template <Color Us>
void Board::make_null_move(const Accumulator& prev_acc, Accumulator& next_acc){
    next_acc = prev_acc;
    // CONTRACT ENFORCEMENT
    assert(gamePly < MAX_GAME_PLIES && "make_null_move: HistoryStack overflow!");
    assert(currentState.En_Passant_Target_Square <= NO_SQUARE && "make_null_move: Corrupted EP square index!");

    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    HistoryStack[gamePly++] = currentState;
    currentState.Half_move_Clock++;
    currentState.Zobrist_Key ^= zobrist_ep[currentState.En_Passant_Target_Square];
    currentState.En_Passant_Target_Square = NO_SQUARE;
    currentState.Zobrist_Key ^= zobrist_side;
    sideToMove = Them;
}

template <Color Us>
void Board::unmake_null_move(){
    // CONTRACT ENFORCEMENT
    assert(gamePly > 0 && "unmake_null_move: HistoryStack underflow!");

    currentState = HistoryStack[--gamePly];
    sideToMove = Us;
}


template <Color Us> // Color of the side that makes the move
bool Board::make_move(Move move, const Accumulator& prev_acc, Accumulator& next_acc){
    next_acc = prev_acc;

    // Phase 0: Basic Variables
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    int from_sq = get_from_sq(move);
    int to_sq = get_to_sq(move);
    int flag = get_flags(move);
    Piece pieceType = pieceOnSquare[from_sq];
    Piece capturedType = pieceOnSquare[to_sq];

    Square w_king = static_cast<Square>(lsb(get_pieces(WHITE, KING)));
    Square b_king = static_cast<Square>(lsb(get_pieces(BLACK, KING)));
    
    // STRICT CONTRACT ENFORCEMENT
    for (int sq = 0; sq < 64; sq++) {
        Piece p = pieceOnSquare[sq];
        if (p != NO_PIECE_TYPE) {
            Color c = (colorPieces[WHITE] & SquareBB(sq)) ? WHITE : BLACK;
            assert((pieces[c][p] & SquareBB(sq)) && "CRITICAL: pieceOnSquare completely desynced from bitboards!");
        }
    }
    assert((get_color_pieces(Us) & SquareBB(from_sq)) && "make_move: Bitboard missing Us piece!");
    assert((pieces[Us][pieceType] & SquareBB(from_sq)) && "make_move: pieceOnSquare completely desynced from Bitboards!");
    assert(gamePly < MAX_GAME_PLIES && "HistoryStack overflow!");
    assert(pieceType != NO_PIECE_TYPE && "make_move: Attempted to move an empty square!");
    assert((colorPieces[Us] & SquareBB(from_sq)) && "make_move: Attempted to move enemy piece!");
    assert(!(capturedType != NO_PIECE_TYPE && (colorPieces[Us] & SquareBB(to_sq))) && "make_move: Friendly fire!");
    assert(capturedType != KING && "make_move: Attempted to capture the King!");
    
    if(flag == 2 || flag == 3) assert(pieceType == KING && "make_move: Non-King attempting to castle!");
    if(flag == 5) {
        assert(pieceType == PAWN && "make_move: Non-Pawn attempting En Passant!");
        assert(to_sq == currentState.En_Passant_Target_Square && "make_move: EP target mismatch!");
    }
    if(flag >= 8) assert(pieceType == PAWN && "make_move: Non-Pawn attempting to promote!");

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
        update_accumulator_piece(next_acc, Them, capturedType, static_cast<Square>(to_sq), -1, w_king, b_king);
    }

    move_piece(Us,pieceType,from_sq,to_sq);
    update_accumulator_piece(next_acc, Us, pieceType, static_cast<Square>(from_sq), -1, w_king, b_king);
    update_accumulator_piece(next_acc, Us, pieceType, static_cast<Square>(to_sq), 1, w_king, b_king);



    // Phase 4: Special Move Flags
    if(flag){
        if(flag & 8){ // promotion
            remove_piece(Us,PAWN,to_sq);
            Piece promoted_to = static_cast<Piece>(KNIGHT + (flag & 3)); 
            add_piece(Us,promoted_to,to_sq); // last 3 bits? ROOK, BISHOP, KNIGHT, QUEEN (in some order)  (if we ordered Pieces correctly and matched with moveencoding)
            update_accumulator_piece(next_acc, Us, PAWN, static_cast<Square>(to_sq), -1, w_king, b_king);
            update_accumulator_piece(next_acc, Us, promoted_to, static_cast<Square>(to_sq), 1, w_king, b_king);
        } else if(flag == 5){ // enpassant
            int enemy_pawn_sq = to_sq + (Us == WHITE ? -8 : 8);
            remove_piece(Them,PAWN,enemy_pawn_sq);
            currentState.Captured_Piece_Type = PAWN;

            update_accumulator_piece(next_acc, Them, PAWN, static_cast<Square>(enemy_pawn_sq), -1, w_king, b_king);
        } else if(flag == 1){ // double push
            currentState.En_Passant_Target_Square = static_cast<Square>(to_sq + (Us == WHITE ? -8 : 8));
            currentState.Zobrist_Key ^= zobrist_ep[currentState.En_Passant_Target_Square]; // square indexable
        } else if(flag == 2){ // KING SIDE
            Square from_rook = Us == WHITE ? H1 : H8;
            Square to_rook = static_cast<Square>(from_rook - 2);
            move_piece(Us,ROOK,from_rook,to_rook);

            update_accumulator_piece(next_acc, Us, ROOK, from_rook, -1, w_king, b_king);
            update_accumulator_piece(next_acc, Us, ROOK, to_rook, 1, w_king, b_king);
        } else if(flag == 3){ // QUEEN SIDE
            Square from_rook = Us == WHITE ? A1 : A8;
            Square to_rook = static_cast<Square>(from_rook + 3);
            move_piece(Us,ROOK,from_rook,to_rook);
            update_accumulator_piece(next_acc, Us, ROOK, from_rook, -1, w_king, b_king);
            update_accumulator_piece(next_acc, Us, ROOK, to_rook, 1, w_king, b_king);
        }
    }
    // NNUE update if king moved:
    if(pieceType == KING){
        refresh_single_accumulator<Us>(*this,next_acc);
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
        // STRICT CONTRACT ENFORCEMENT
        for (int sq = 0; sq < 64; sq++) {
            Piece p = pieceOnSquare[sq];
            if (p != NO_PIECE_TYPE) {
                Color c = (colorPieces[WHITE] & SquareBB(sq)) ? WHITE : BLACK;
                assert((pieces[c][p] & SquareBB(sq)) && "CRITICAL: pieceOnSquare completely desynced from bitboards!");
            }
        }
        return false;
    }

    // STRICT CONTRACT ENFORCEMENT
    for (int sq = 0; sq < 64; sq++) {
        Piece p = pieceOnSquare[sq];
        if (p != NO_PIECE_TYPE) {
            Color c = (colorPieces[WHITE] & SquareBB(sq)) ? WHITE : BLACK;
            assert((pieces[c][p] & SquareBB(sq)) && "CRITICAL: pieceOnSquare completely desynced from bitboards!");
        }
    }
    return true;
}

template <Color Us>
void Board::unmake_move(Move move){
    // Phase 0: Basic Variables
    constexpr Color Them = Us == WHITE ? BLACK : WHITE;
    int from_sq = get_from_sq(move);
    int to_sq = get_to_sq(move);
    int flag = get_flags(move);
    Piece pieceType = pieceOnSquare[to_sq];
    Piece capturedType = currentState.Captured_Piece_Type;


    // SEMANTIC CONTRACT ENFORCEMENT
    assert(gamePly > 0 && "unmake_move: HistoryStack underflow (gamePly is 0)!");
    assert(pieceType != NO_PIECE_TYPE && "unmake_move: Attempting to unmake a move from an empty square!");
    assert((colorPieces[Us] & SquareBB(to_sq)) && "unmake_move: Piece to unmake does not belong to Us!");
    if(flag >= 8) {
        Piece promoted_to = static_cast<Piece>(KNIGHT + (flag & 3)); 
        assert(pieceType == promoted_to && "unmake_move: Promoted piece type mismatch!");
    }
    
    // Phase 1: Base Piece Reversal
    quiet_move_piece(Us,pieceType,to_sq,from_sq);

    // Phase 2: Special Flag Reversal
    if(flag){
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
    if(capturedType != NO_PIECE_TYPE && flag != 5){
        quiet_add_piece(Them, capturedType, to_sq);
    }

    // Phase 4: State Time Travel
    currentState = HistoryStack[--gamePly];
    sideToMove = Us;
}