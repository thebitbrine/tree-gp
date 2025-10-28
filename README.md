# Tree-Based Genetic Programming System

A self-evolving system built in C that uses tree-based genetic programming to learn programs from scratch.

## Features

- Tree-based program representation with typed operations
- Multi-threaded fitness evaluation (9x speedup on 12 cores)
- Memory operations for stateful programs
- Library learning system that extracts and reuses successful patterns
- Crossover and mutation operators with elitism
- Tournament selection with parsimony pressure

## Solved Tasks

- **Addition** (100% gen 0): Learn `a + b`
- **CartPole** (90% in 10-50 gens): Balance pole on cart, discovers PD controller
- **Maze Navigation** (solved in 20-50 gens): Navigate 5x5 maze with walls

## Partially Solved

- **Taxi-v3** (-192 fitness): Pick up and drop off passenger (work in progress)

## Building

```bash
make
```

## Running

```bash
./test_add          # Simple addition task
./test_cartpole     # CartPole balancing
./test_maze         # Maze navigation
./test_taxi         # Taxi-v3 (hard)
./benchmark         # Performance benchmark
```

## Architecture

### Core Components

- `gp.h/gp.c` - Core GP system with tree operations, evolution, library learning
- `test_*.c` - Task-specific fitness functions and environments

### Operations

- Arithmetic: ADD, SUB, MUL, DIV, MOD
- I/O: INPUT, OUTPUT
- Control: IF_GT, SEQ
- Memory: MEM_READ, MEM_WRITE
- Library: LIB (calls learned patterns)

### Evolution

- Population: 800 individuals
- Tournament selection (size 7)
- Elitism (top 10 preserved)
- 70% crossover, 30% mutation
- Library update every 10 generations

## Performance

With multi-threading on 12 cores:
- Population 1200, 100 generations: 14.5 seconds
- 9.1x speedup vs single-threaded

## Future Work

- Better reward shaping for Taxi-v3
- Hierarchical evolution (sub-programs)
- More complex tasks
- NEAT-style speciation
