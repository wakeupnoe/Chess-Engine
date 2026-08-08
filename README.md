# Chess-Engine
A competitive C++ Chess engine.

# Current Status
- Hello world test via CMake succeeded
- Set up testing environment with Catch2 (but no testcases yet, but also no code so thats fine)


# Meta-Plan
- Set up automatic testing environment via GitHub-Actions
- Build the project modular and testable, so that bugs get 
- No AI-written code, only feedback and conceptual use of it

# Plan for the Engine
- Use bitboards and operate low-level (e.g. move generation via bitboard shifts)
- Mini-Max Search up until a certain depth (with Alpha-Beta pruning later)
- evaluation via hard coded heuristic (later via NN)