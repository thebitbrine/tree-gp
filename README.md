# Tree-Based Genetic Programming System

A self-evolving system built in C that uses tree-based genetic programming to learn programs from scratch.

## Features

- Tree-based program representation with typed operations
- Multi-threaded fitness evaluation (9x speedup on 12 cores)
- Memory operations for stateful programs
- Automatic ADF (Automatically Defined Functions) with parameterization
- Library learning with diversity enforcement and quality scoring
- Crossover and mutation operators with elitism
- Tournament selection with parsimony pressure

## Solved Tasks

- **Addition** (100% gen 0): Learn `a + b`
- **CartPole** (90% in 10-50 gens): Balance pole on cart, discovers PD controller
- **Maze Navigation** (solved in 20-50 gens): Navigate 5x5 maze with walls
- **3-bit Parity** (100% in ~77 gens): Even parity using XOR operations
- **6-bit Multiplexer** (partial, 75%): Address decoding with boolean logic

## Partially Solved

- **Taxi-v3** (~35 steps improvement): Pick up and drop off passenger (limited by temporal credit assignment)

## Building

```bash
make
```

## Running

```bash
./test_add          # Simple addition task
./test_cartpole     # CartPole balancing
./test_maze         # Maze navigation
./test_parity       # 3-bit even parity (boolean logic)
./test_mux          # 6-bit multiplexer (hard)
./test_taxi         # Taxi-v3 (very hard, temporal credit assignment)
./test_adf          # ADF demonstration
./benchmark         # Performance benchmark
```

## Architecture

### Core Components

- `gp.h/gp.c` - Core GP system with tree operations, evolution, library learning
- `test_*.c` - Task-specific fitness functions and environments

### Operations (35 total)

- **Arithmetic**: ADD, SUB, MUL, DIV, MOD
- **Boolean**: AND, OR, XOR, NOT
- **Comparison**: EQ, LT, LTE, GT
- **Math**: ABS, NEG, MAX, MIN
- **Activation**: SIN, TANH, STEP, IDENT
- **Control**: IF_GT, IF, SEQ
- **I/O**: INPUT, OUTPUT, CONST
- **Memory**: MEM_READ, MEM_WRITE
- **Library**: LIB (non-parameterized patterns)
- **ADF**: FUNC_CALL (parameterized functions), PARAM (function parameters)

### Evolution

- Population: 800 individuals
- Tournament selection (size 7)
- Elitism (top 10 preserved)
- 70% crossover, 30% mutation
- Library update every 5 generations
- Automatic parameterization of extracted patterns
- Diversity enforcement (70% similarity threshold)
- Quality scoring and competitive pruning

## Performance

With multi-threading on 12 cores:
- Population 1200, 100 generations: 14.5 seconds
- 9.1x speedup vs single-threaded

## Future Work

- Better reward shaping for Taxi-v3
- Hierarchical evolution (sub-programs)
- More complex tasks
- NEAT-style speciation
