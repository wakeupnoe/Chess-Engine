#include <catch2/catch_test_macros.hpp>
#include "../src/types.h"
#include "../src/bitboard.h"

TEST_CASE("Bitboard Primitives", "[bitboard]") {
    Bitboard b = 0ULL;

    SECTION("SetBit and GetBit") {
        SetBit(b, E4);
        REQUIRE(GetBit(b, E4) == true);
        REQUIRE(GetBit(b, A1) == false);
        REQUIRE(b == 0x10000000ULL);
    }

    SECTION("PopBit") {
        SetBit(b, E4);
        PopBit(b, E4);
        REQUIRE(b == 0ULL);
        REQUIRE(GetBit(b, E4) == false);
    }

    SECTION("popcount") {
        SetBit(b, A1);
        SetBit(b, H8);
        SetBit(b, D4);
        REQUIRE(popcount(b) == 3);
    }

    SECTION("lsb and pop_lsb") {
        SetBit(b, C3); // Index 18
        SetBit(b, F7); // Index 53
        
        REQUIRE(lsb(b) == C3);
        
        Square popped = static_cast<Square>(pop_lsb(b));
        REQUIRE(popped == C3);
        REQUIRE(popcount(b) == 1);
        REQUIRE(lsb(b) == F7);
    }
}

TEST_CASE("Bitboard Masks", "[bitboard]") {
    SECTION("File and Rank BBs") {
        // File A should have exactly 8 bits set
        REQUIRE(popcount(FileBB[0]) == 8);
        // Rank 1 should equal the bottom 8 bits
        REQUIRE(RankBB[0] == 0xFFULL);
        // Intersection of File A and Rank 1 should be exactly square A1
        REQUIRE((FileBB[0] & RankBB[0]) == (1ULL << A1));
    }
}