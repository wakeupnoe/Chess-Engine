#pragma once
#include "types.h"
#include "board.h"
#include <cstdint>
#include <string>
#include <iostream>

inline int flip_sq(Square sq){
    return sq ^ 63; // was 56, but LLM suggested to change it since the specific net of stockfish 12 used uses 63
}

inline int get_piece_index(Piece piece, Color color, Color perspective){
    if(piece == KING) return -1;
    bool is_friendly = (color == perspective);
    return piece * 2 + (is_friendly ? 0 : 1); // was is_friendly ? piece : piece + 5; but LLM suggested to change it (the indexing changed within stockfish)
}

inline int get_halfkp_index(Color perspective, Square king_sq, Square piece_sq, Piece piece_type, Color piece_color ){
    int p_index = get_piece_index(piece_type, piece_color, perspective);
    if(p_index == -1) return -1;
    int k_sq = (perspective == WHITE) ? king_sq : flip_sq(king_sq);
    int p_sq = (perspective == WHITE) ? piece_sq : flip_sq(piece_sq);
    return k_sq * 641 + p_index * 64 + p_sq + 1; //  +1 wild AI call, since stockfish did that?? Let's see
}

constexpr int INPUT_SIZE = 41024;
constexpr int LAYER_0_SIZE = 256;
constexpr int LAYER_1_SIZE = 32;
constexpr int LAYER_2_SIZE = 32;
constexpr int OUTPUT_SIZE = 1;

#pragma pack(push, 1)

struct FeatureTransformer {
    int16_t biases[LAYER_0_SIZE];
    int16_t weights[INPUT_SIZE][LAYER_0_SIZE];
};

struct NetworkLayer1 {
    int32_t biases[LAYER_1_SIZE];
    int8_t weights[LAYER_1_SIZE][LAYER_0_SIZE * 2]; // white and black
};

struct NetworkLayer2 {
    int32_t biases[LAYER_2_SIZE];
    int8_t weights[LAYER_2_SIZE][LAYER_1_SIZE];
};

struct NetworkLayer3 {
    int32_t biases[OUTPUT_SIZE];
    int8_t weights[OUTPUT_SIZE][LAYER_2_SIZE];
};

struct NNUE_Network {
    uint32_t version;
    uint32_t hash_value; // architecture identifier
    uint32_t size_description; // Metadata

    //actual network
    FeatureTransformer feature_weights;
    NetworkLayer1 layer1;
    NetworkLayer2 layer2;
    NetworkLayer3 layer3;
};

#pragma pack(pop)

extern NNUE_Network nnue; // the global unicat ^^

bool load_nnue(const std::string& file_path);

inline int clipped_relu(int x){
    return std::max(0, std::min(127, x));
}


template <Color Us>
int nnue_evaluate(const Accumulator& acc) {
    const int16_t* us_acc = (Us == WHITE) ? acc.white : acc.black;
    const int16_t* them_acc = (Us == WHITE) ? acc.black : acc.white;
    int8_t input_features[512];
    for(int i = 0; i < LAYER_0_SIZE; i++){
        input_features[i] = clipped_relu(us_acc[i]);
        input_features[LAYER_0_SIZE + i] = clipped_relu(them_acc[i]);
    }

    // Layer 1
    int32_t l1_output[LAYER_1_SIZE];
    for(int i = 0; i<LAYER_1_SIZE; i++){
        int32_t sum = nnue.layer1.biases[i];
        for(int j = 0; j<LAYER_0_SIZE * 2; j++){
            sum += input_features[j] * nnue.layer1.weights[i][j];
        }
        l1_output[i] = clipped_relu(sum >> 6);
    }

    // Layer 2
    int32_t l2_output[LAYER_2_SIZE];
    for(int i = 0; i<LAYER_2_SIZE; i++){
        int32_t sum = nnue.layer2.biases[i];
        for(int j = 0; j<LAYER_1_SIZE; j++){
            sum += l1_output[j] * nnue.layer2.weights[i][j];
        }
        l2_output[i] = clipped_relu(sum >> 6);
    }

    // Layer 3
    int32_t final_output = nnue.layer3.biases[0];
    for(int j = 0; j<LAYER_2_SIZE; j++){
        final_output += l2_output[j] * nnue.layer3.weights[0][j];
    }
    
    return final_output / 16;
}