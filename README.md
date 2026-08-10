# Chess-Engine
A competitive C++ Chess engine.

# Current Status
- Hello world test via CMake succeeded
- Set up testing environment with Catch2
- Set up basics in types.h and bitboard.h
- attack pattern generation for king, knight and rock done!

next up: Bishop (+Queen)

Currently implementing board-, and move-logic as well as move generation


# Meta-Plan
[y] Set up automatic testing environment via GitHub-Actions
[ ] Build the project modular and testable, so that bugs get 
[ ] No AI-written code, only feedback and conceptual use of it
Use of AI in: all of tests/ nad in .github/workflows/ci.yml, src is AI code free

# Plan for the Engine
- Use bitboards and operate low-level (e.g. move generation via bitboard shifts)
- Mini-Max Search up until a certain depth (with Alpha-Beta pruning later)
- evaluation via hard coded heuristic (later via NN)