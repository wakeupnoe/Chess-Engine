add_library(EngineLib STATIC
    attacks.cpp
    bitboard.cpp
    board.cpp
    movegen.cpp
    zobrist.cpp
)

target_include_directories(EngineLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(Chess-Engine main.cpp)

target_link_libraries(Chess-Engine PRIVATE EngineLib)