add_library(EngineLib STATIC
    board.cpp
)

add_executable(Chess-Engine main.cpp)

target_link_libraries(Chess-Engine PRIVATE EngineLib)