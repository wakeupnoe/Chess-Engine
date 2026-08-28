# Chess-Engine
A competitive C++ Chess engine.

## Current Status
- CMake setup succeeded (+ Testing environment with Catch2)
- basic datatypes, helperfunctions in types.h, bitboard.h
- Movegeneration in movegen.h
- board class and make_move / unmake_move in board.h
- attack generation for all pieces in attacks.h
- Perft test setup (currently main runs perft tests)
- Implement alpha beta search in search.h and basic heurisic evaluation
- move ordering beginning

## Performance
- Perft runs at ~75M NPS (without Bulk Counting)

## Next steps
[+] Quiescence search
[+] history heuristic & Null pointer pruning
[-] PVS & LMR
[-] Aspiartion Windows & SEE
[-] MovePicker state machine
[-] NNUE integration
[-] Automated tuning?

## Meta-Plan
[ ] Set up automatic testing environment via GitHub-Actions
[ ] Build the project modular and testable, so that bugs get 
[ ] No AI-written code, only feedback and conceptual use of it
Use of AI in:
- all of tests/, in .github/workflows/ci.yml, and AIPerft.cpp/AIPerft.h, otherwise src is AI code free

## Plan for the Engine
- Highly optimized low level operations
- Mini-Max Search up until a certain depth (with Alpha-Beta pruning, quincent search, move ordering later)
- evaluation via hard coded heuristic (later via NNUE)