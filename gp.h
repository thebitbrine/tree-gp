#ifndef GP_H
#define GP_H

#include <stdint.h>
#include <pthread.h>

// Type system for operations
typedef enum {
    TYPE_INT,        // Single integer
    TYPE_VOID,       // No return value
} ValueType;

// Operation types
typedef enum {
    // Arithmetic (Int, Int -> Int)
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,

    // Boolean/Bitwise (Int, Int -> Int)
    OP_AND,          // Bitwise AND
    OP_OR,           // Bitwise OR
    OP_XOR,          // Bitwise XOR
    OP_NOT,          // Bitwise NOT (unary)

    // Comparison (Int, Int -> Int) - returns 1 or 0
    OP_EQ,           // Equal
    OP_LT,           // Less than
    OP_LTE,          // Less than or equal

    // Math (Int -> Int, or Int,Int -> Int)
    OP_ABS,          // Absolute value (unary)
    OP_NEG,          // Negate (unary)
    OP_MAX,          // Maximum of two values
    OP_MIN,          // Minimum of two values
    OP_GT,           // Greater than

    // Activation/Transfer functions (Int -> Int)
    OP_SIN,          // Sine (scaled for integers)
    OP_TANH,         // Hyperbolic tangent (scaled)
    OP_STEP,         // Step function: 1 if x > 0, else 0
    OP_IDENT,        // Identity/pass-through

    // Terminals (-> Int)
    OP_CONST,        // Constant value
    OP_INPUT,        // Read input[N]

    // Side effects (Int -> Void)
    OP_OUTPUT,       // Write to output

    // Control flow
    OP_IF_GT,        // If a > b then c else d (Int, Int, Int, Int -> Int)
    OP_IF,           // If cond != 0 then a else b (Int, Int, Int -> Int)

    // Sequence (Void, Void -> Void)
    OP_SEQ,          // Execute two statements in sequence

    // Library functions (learned patterns)
    OP_LIBRARY,      // Reference to library function

    // Memory operations (-> Int, Int -> Void)
    OP_MEM_READ,     // Read memory[N]
    OP_MEM_WRITE,    // Write value to memory[N]

    // ADF (Automatically Defined Functions)
    OP_FUNC_CALL,    // Call function with arguments
    OP_PARAM,        // Reference parameter within function

    OP_COUNT
} OpType;

// Maximum tree depth and children
#define MAX_DEPTH 10
#define MAX_CHILDREN 4
#define MAX_LIBRARY 32
#define MAX_INPUTS 16  // Increased for 11-bit mux and larger problems
#define MAX_OUTPUTS 8
#define MAX_MEMORY 8

// Tree node
typedef struct Node {
    OpType op;
    ValueType type;
    int value;              // For OP_CONST, OP_INPUT index, or OP_LIBRARY index
    int num_children;
    struct Node* children[MAX_CHILDREN];
} Node;

// Library entry (learned patterns / ADF functions)
typedef struct {
    char name[32];
    Node* tree;
    int uses;               // How many times it's been used successfully
    float avg_fitness;      // Average fitness of programs using it
    int num_params;         // Number of parameters for ADF
    ValueType param_types[MAX_CHILDREN];  // Parameter types
} LibraryEntry;

// Individual program
typedef struct {
    Node* root;
    float fitness;
    int depth;
    int size;              // Number of nodes
} Program;

// Population
#define POP_SIZE 2000  // Increased for harder problems
#define TOURNAMENT_SIZE 7
#define ELITE_SIZE 20  // More elites with larger population

typedef struct {
    Program* programs[POP_SIZE];
    LibraryEntry library[MAX_LIBRARY];
    int library_size;

    Program* best;
    float best_fitness;

    // Evolution stats
    int generation;
    float avg_fitness;
    int num_inputs;  // Number of inputs for this problem

    pthread_mutex_t lock;
} Population;

// Operation metadata (for printing/debugging)
typedef struct {
    OpType op;
    const char* name;
    int arity;
    ValueType return_type;
    ValueType arg_types[MAX_CHILDREN];
} OpInfo;

extern OpInfo op_info[];

// Function prototypes
Population* pop_create();
void pop_destroy(Population* pop);

Node* node_create(OpType op, int value);
Node* node_copy(Node* node);
void node_destroy(Node* node);
int node_depth(Node* node);
int node_size(Node* node);

Program* prog_create_random(int max_depth, int num_inputs);
Program* prog_copy(Program* prog);
void prog_destroy(Program* prog);
void prog_update_metadata(Program* prog);

// Visualization
void print_tree(Node* node, int indent);

// Execution
typedef struct {
    int inputs[MAX_INPUTS];
    int num_inputs;
    int outputs[MAX_OUTPUTS];
    int num_outputs;
    int memory[MAX_MEMORY];     // Persistent memory between executions

    // ADF argument stack (for nested function calls)
    int args[MAX_CHILDREN * 4];  // Stack for up to 4 levels of nesting with 4 args each
    int arg_stack_ptr;           // Current position in argument stack
    int arg_frame_base;          // Base of current function's arguments
} Context;

int execute_node(Node* node, Context* ctx, Population* pop);
void execute_program(Program* prog, Context* ctx, Population* pop);

// Evolution operators
Program* evolve_mutate(Program* parent, Population* pop);
Program* evolve_crossover(Program* p1, Program* p2);
void evolve_simplify(Program* prog);

// Evolution
void evolve_generation(Population* pop, float (*fitness_fn)(Program*, void*), void* data, int num_inputs);

// Library learning
void library_add(Population* pop, Node* pattern, const char* name, float fitness);
void library_update(Population* pop);

#endif
