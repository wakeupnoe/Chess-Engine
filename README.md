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
- move ordering done
- Quiescence search
- history heuristic & Null pointer pruning
- PVS & LMR and Aspiartion Windows
- Static exchange evaluation
- Move picker state machine implemented
- Futility Pruning (standard and reverse), late move pruning
- Singular Extensions, Enhanced History heuristic (counter moves)
- Basic UCI support (metrics etc.)
- Lazy SMP
- NNUE integrted (Stockfish 12 weights)
- ~2500 lines of code
- SearchWorker class refactoring complete

## Performance
- Perft runs at ~75M NPS (without Bulk Counting)
- Mate in 8 and 10 solved, not tested extensivly
- ~5M NPS in Play on 8 threads
- Estimation of strength: ~2950 elo

## Next steps
- implement <Nodetype> template for the search functions
- local nodes_searched and sum on UCI metric output instead of atomic accumulator
- Test, improve, possibly generate own .nnue file


## Meta-Plan
[ ] Set up automatic testing environment via GitHub-Actions
[ ] Build the project modular and testable, so that bugs get 
[ ] No AI-written code, only feedback and conceptual use of it
Use of AI in:
- all of tests/, in .github/workflows/ci.yml, in evaluation.h and AIPerft.cpp/AIPerft.h, otherwise src is AI code free

## Plan for the Engine
- Highly optimized low level operations
- Alpha-Beta pruning (pruned to the death)
- evaluation via NNUE
- 2500 elo+ reached!