#include "nnue.h"
#include <fstream>
#include <iostream>
#include <vector>

NNUE_Network nnue;

bool load_nnue(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::ate | std::ios::binary); // Open at end to get size
    if (!file.is_open()) {
        std::cout << "info string ERROR: Failed to load NNUE file: " << file_path << std::endl;
        exit(1);
    }

    
    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg); // Rewind to start

    // Headers
    file.read(reinterpret_cast<char*>(&nnue.version), 4);
    file.read(reinterpret_cast<char*>(&nnue.hash_value), 4);
    file.read(reinterpret_cast<char*>(&nnue.size_description), 4);

    std::vector<char> desc(nnue.size_description + 1, '\0');
    file.read(desc.data(), nnue.size_description);
    
    uint32_t transformer_hash;
    file.read(reinterpret_cast<char*>(&transformer_hash), 4);
    
    // Read Feature Transformer
    file.read(reinterpret_cast<char*>(&nnue.feature_weights), sizeof(FeatureTransformer));
    
    // There is exactly ONE hash (4 bytes) before the rest of the Network
    uint32_t network_hash;
    file.read(reinterpret_cast<char*>(&network_hash), 4);
    
    // Read Layers 1, 2, 3
    file.read(reinterpret_cast<char*>(&nnue.layer1), sizeof(NetworkLayer1));
    file.read(reinterpret_cast<char*>(&nnue.layer2), sizeof(NetworkLayer2));
    file.read(reinterpret_cast<char*>(&nnue.layer3), sizeof(NetworkLayer3));

    return true;
}