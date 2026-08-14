#pragma once
#include <cstdint>

using Bitboard = uint64_t;
using Move = uint16_t;
inline int get_from_sq(Move m) { return m & 0x3F; }
inline int get_to_sq(Move m) { return (m>>6) & 0x3F; }
inline int get_flags(Move m) { return m >> 12; }
enum Color : uint8_t{WHITE,BLACK,NO_COLOR};

enum Piece : uint8_t {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE_TYPE};

enum Square : uint8_t {
    A1,B1,C1,D1,E1,F1,G1,H1,
    A2,B2,C2,D2,E2,F2,G2,H2,
    A3,B3,C3,D3,E3,F3,G3,H3,
    A4,B4,C4,D4,E4,F4,G4,H4,
    A5,B5,C5,D5,E5,F5,G5,H5,
    A6,B6,C6,D6,E6,F6,G6,H6,
    A7,B7,C7,D7,E7,F7,G7,H7,
    A8,B8,C8,D8,E8,F8,G8,H8,
    NO_SQUARE // = 64
};

enum CastlingRights : uint8_t {
    WK = 1, WQ = 2, BK = 4, BQ = 8
};

inline constexpr uint8_t CASTLING_MASK[64] = {
    13, 15, 15, 15, 12, 15, 15, 12,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11,
};

struct StateInfo {                                                              //  optimal for padding    
    Bitboard Zobrist_Key = 0ULL; //Incremental Zobrist Hash Key                 //  8 bytes
    uint8_t castling_rights = 15; //Castling Rights last 4 bits 1 1 1 1         //  1 byte
    Square En_Passant_Target_Square = NO_SQUARE; // En Passant Target Square    //  1 byte
    uint8_t Half_move_Clock = 0; // Half-move Clock                             //  1 byte
    Piece Captured_Piece_Type = NO_PIECE_TYPE;                                  //  1 byte
                                                                                //  +4 bytes padding = 16bytes!
};

enum File : uint8_t {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H
};
//enum Rank : uint8_t {
//    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8
//};
