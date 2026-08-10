add_library(EngineLib STATIC
    bitboard.cpp
    board.cpp
    zobrist.cpp
    attacks.cpp
    movegen.cpp
)

target_include_directories(EngineLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(Chess-Engine main.cpp)

target_link_libraries(Chess-Engine PRIVATE EngineLib)