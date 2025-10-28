#include "gp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Operation metadata
OpInfo op_info[] = {
    {OP_ADD, "ADD", 2, TYPE_INT, {TYPE_INT, TYPE_INT}},
    {OP_SUB, "SUB", 2, TYPE_INT, {TYPE_INT, TYPE_INT}},
    {OP_MUL, "MUL", 2, TYPE_INT, {TYPE_INT, TYPE_INT}},
    {OP_DIV, "DIV", 2, TYPE_INT, {TYPE_INT, TYPE_INT}},
    {OP_MOD, "MOD", 2, TYPE_INT, {TYPE_INT, TYPE_INT}},
    {OP_CONST, "CONST", 0, TYPE_INT, {}},
    {OP_INPUT, "INPUT", 0, TYPE_INT, {}},
    {OP_OUTPUT, "OUTPUT", 1, TYPE_VOID, {TYPE_INT}},
    {OP_IF_GT, "IF_GT", 4, TYPE_INT, {TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT}},
    {OP_SEQ, "SEQ", 2, TYPE_VOID, {TYPE_VOID, TYPE_VOID}},
    {OP_LIBRARY, "LIB", 0, TYPE_INT, {}},
    {OP_MEM_READ, "MEM_READ", 0, TYPE_INT, {}},
    {OP_MEM_WRITE, "MEM_WRITE", 1, TYPE_VOID, {TYPE_INT}},
};

// Helper: Get operation info
static OpInfo* get_op_info(OpType op) {
    for (int i = 0; i < OP_COUNT; i++) {
        if (op_info[i].op == op) return &op_info[i];
    }
    return NULL;
}

// Print tree for visualization
void print_tree(Node* node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) printf("  ");

    OpInfo* info = get_op_info(node->op);
    if (info) {
        printf("%s", info->name);
        if (node->op == OP_CONST) {
            printf("(%d)", node->value);
        } else if (node->op == OP_INPUT) {
            printf("[%d]", node->value);
        } else if (node->op == OP_MEM_READ) {
            printf("[mem%d]", node->value);
        } else if (node->op == OP_MEM_WRITE) {
            printf("[mem%d]", node->value);
        }
        printf("\n");

        for (int i = 0; i < node->num_children; i++) {
            print_tree(node->children[i], indent + 1);
        }
    }
}

// Update program metadata
void prog_update_metadata(Program* prog) {
    if (prog && prog->root) {
        prog->depth = node_depth(prog->root);
        prog->size = node_size(prog->root);
    }
}

// Node operations
Node* node_create(OpType op, int value) {
    Node* n = calloc(1, sizeof(Node));
    n->op = op;
    n->value = value;
    OpInfo* info = get_op_info(op);
    if (info) {
        n->type = info->return_type;
        n->num_children = info->arity;
    }
    return n;
}

Node* node_copy(Node* node) {
    if (!node) return NULL;
    Node* copy = node_create(node->op, node->value);
    copy->num_children = node->num_children;
    for (int i = 0; i < node->num_children; i++) {
        copy->children[i] = node_copy(node->children[i]);
    }
    return copy;
}

void node_destroy(Node* node) {
    if (!node) return;
    for (int i = 0; i < node->num_children; i++) {
        node_destroy(node->children[i]);
    }
    free(node);
}

int node_depth(Node* node) {
    if (!node) return 0;
    int max_child_depth = 0;
    for (int i = 0; i < node->num_children; i++) {
        int d = node_depth(node->children[i]);
        if (d > max_child_depth) max_child_depth = d;
    }
    return 1 + max_child_depth;
}

int node_size(Node* node) {
    if (!node) return 0;
    int size = 1;
    for (int i = 0; i < node->num_children; i++) {
        size += node_size(node->children[i]);
    }
    return size;
}

// Random tree generation
static Node* create_random_tree(int depth, ValueType required_type, int num_inputs) {
    if (depth >= MAX_DEPTH || (depth > 0 && rand() % 3 == 0)) {
        // Create terminal
        if (required_type == TYPE_INT) {
            int choice = rand() % 3;
            if (choice == 0 && num_inputs > 0) {
                return node_create(OP_INPUT, rand() % num_inputs);
            } else if (choice == 1) {
                return node_create(OP_MEM_READ, rand() % MAX_MEMORY);
            } else {
                return node_create(OP_CONST, (rand() % 20) - 10);
            }
        } else {
            // TYPE_VOID - create output or mem_write statement
            if (rand() % 3 == 0) {
                Node* mem_write = node_create(OP_MEM_WRITE, rand() % MAX_MEMORY);
                mem_write->children[0] = create_random_tree(depth + 1, TYPE_INT, num_inputs);
                mem_write->num_children = 1;
                return mem_write;
            } else {
                Node* out = node_create(OP_OUTPUT, 0);
                out->children[0] = create_random_tree(depth + 1, TYPE_INT, num_inputs);
                out->num_children = 1;
                return out;
            }
        }
    }

    // Create non-terminal
    OpType ops[10];
    int n_ops = 0;

    // Collect operations that match required type
    for (int i = 0; i < OP_COUNT - 1; i++) {  // Exclude OP_LIBRARY for now
        if (op_info[i].return_type == required_type) {
            ops[n_ops++] = op_info[i].op;
        }
    }

    if (n_ops == 0) {
        // Fallback to terminal
        return create_random_tree(MAX_DEPTH, required_type, num_inputs);
    }

    OpType op = ops[rand() % n_ops];
    Node* node = node_create(op, 0);
    OpInfo* info = get_op_info(op);

    for (int i = 0; i < info->arity; i++) {
        node->children[i] = create_random_tree(depth + 1, info->arg_types[i], num_inputs);
    }

    return node;
}

// Program operations
Program* prog_create_random(int max_depth, int num_inputs) {
    Program* prog = calloc(1, sizeof(Program));

    // Create a program that outputs something
    // SEQ(OUTPUT(...), VOID) pattern
    Node* root = node_create(OP_SEQ, 0);
    root->children[0] = node_create(OP_OUTPUT, 0);
    root->children[0]->children[0] = create_random_tree(0, TYPE_INT, num_inputs);
    root->children[1] = node_create(OP_OUTPUT, 0);
    root->children[1]->children[0] = node_create(OP_CONST, 0);  // Dummy second output

    prog->root = root;
    prog->depth = node_depth(root);
    prog->size = node_size(root);
    prog->fitness = -INFINITY;

    return prog;
}

Program* prog_copy(Program* prog) {
    if (!prog) return NULL;
    Program* copy = calloc(1, sizeof(Program));
    copy->root = node_copy(prog->root);
    copy->fitness = prog->fitness;
    copy->depth = prog->depth;
    copy->size = prog->size;
    return copy;
}

void prog_destroy(Program* prog) {
    if (!prog) return;
    node_destroy(prog->root);
    free(prog);
}

// Execution
int execute_node(Node* node, Context* ctx, Population* pop) {
    if (!node) return 0;

    switch (node->op) {
        case OP_ADD: {
            int a = execute_node(node->children[0], ctx, pop);
            int b = execute_node(node->children[1], ctx, pop);
            return a + b;
        }
        case OP_SUB: {
            int a = execute_node(node->children[0], ctx, pop);
            int b = execute_node(node->children[1], ctx, pop);
            return a - b;
        }
        case OP_MUL: {
            int a = execute_node(node->children[0], ctx, pop);
            int b = execute_node(node->children[1], ctx, pop);
            return a * b;
        }
        case OP_DIV: {
            int a = execute_node(node->children[0], ctx, pop);
            int b = execute_node(node->children[1], ctx, pop);
            return (b != 0) ? (a / b) : 0;
        }
        case OP_MOD: {
            int a = execute_node(node->children[0], ctx, pop);
            int b = execute_node(node->children[1], ctx, pop);
            return (b != 0) ? (a % b) : 0;
        }
        case OP_CONST:
            return node->value;
        case OP_INPUT: {
            int idx = node->value;
            if (idx >= 0 && idx < ctx->num_inputs) {
                return ctx->inputs[idx];
            }
            return 0;
        }
        case OP_OUTPUT: {
            int val = execute_node(node->children[0], ctx, pop);
            if (ctx->num_outputs < MAX_OUTPUTS) {
                ctx->outputs[ctx->num_outputs++] = val;
            }
            return 0;
        }
        case OP_IF_GT: {
            int a = execute_node(node->children[0], ctx, pop);
            int b = execute_node(node->children[1], ctx, pop);
            if (a > b) {
                return execute_node(node->children[2], ctx, pop);
            } else {
                return execute_node(node->children[3], ctx, pop);
            }
        }
        case OP_SEQ: {
            execute_node(node->children[0], ctx, pop);
            execute_node(node->children[1], ctx, pop);
            return 0;
        }
        case OP_LIBRARY: {
            int idx = node->value;
            if (pop && idx >= 0 && idx < pop->library_size) {
                return execute_node(pop->library[idx].tree, ctx, pop);
            }
            return 0;
        }
        case OP_MEM_READ: {
            int idx = node->value;
            if (idx >= 0 && idx < MAX_MEMORY) {
                return ctx->memory[idx];
            }
            return 0;
        }
        case OP_MEM_WRITE: {
            int idx = node->value;
            int val = execute_node(node->children[0], ctx, pop);
            if (idx >= 0 && idx < MAX_MEMORY) {
                ctx->memory[idx] = val;
            }
            return 0;
        }
        default:
            return 0;
    }
}

void execute_program(Program* prog, Context* ctx, Population* pop) {
    ctx->num_outputs = 0;
    if (prog && prog->root) {
        execute_node(prog->root, ctx, pop);
    }
}

// Population
Population* pop_create() {
    Population* pop = calloc(1, sizeof(Population));
    pthread_mutex_init(&pop->lock, NULL);
    pop->best_fitness = -INFINITY;
    return pop;
}

void pop_destroy(Population* pop) {
    if (!pop) return;
    for (int i = 0; i < POP_SIZE; i++) {
        prog_destroy(pop->programs[i]);
    }
    for (int i = 0; i < pop->library_size; i++) {
        node_destroy(pop->library[i].tree);
    }
    prog_destroy(pop->best);
    pthread_mutex_destroy(&pop->lock);
    free(pop);
}

// Mutation: replace a random subtree
static Node* mutate_tree(Node* node, int depth, int num_inputs) {
    if (!node) return NULL;

    // 20% chance to replace this subtree
    if (rand() % 5 == 0) {
        node_destroy(node);
        return create_random_tree(depth, rand() % 2 == 0 ? TYPE_INT : TYPE_VOID, num_inputs);
    }

    // Recursively mutate children
    Node* copy = node_create(node->op, node->value);
    copy->num_children = node->num_children;
    for (int i = 0; i < node->num_children; i++) {
        copy->children[i] = mutate_tree(node_copy(node->children[i]), depth + 1, num_inputs);
    }

    return copy;
}

// Inject library calls into tree
static void inject_library_calls(Node* node, Population* pop, int depth) {
    if (!node || !pop || pop->library_size == 0) return;
    if (depth > MAX_DEPTH) return;

    // Get node's return type
    OpInfo* info = get_op_info(node->op);
    if (!info) return;

    // 5% chance to replace this node with a library call
    // Only replace INT-returning nodes (library entries return INT)
    if (rand() % 20 == 0 && info->return_type == TYPE_INT) {
        int lib_idx = rand() % pop->library_size;
        node->op = OP_LIBRARY;
        node->value = lib_idx;
        // Destroy children since library is a terminal
        for (int i = 0; i < node->num_children; i++) {
            node_destroy(node->children[i]);
        }
        node->num_children = 0;
        pop->library[lib_idx].uses++;
        return;
    }

    // Recursively process children
    for (int i = 0; i < node->num_children; i++) {
        inject_library_calls(node->children[i], pop, depth + 1);
    }
}

Program* evolve_mutate(Program* parent, Population* pop) {
    Program* child = calloc(1, sizeof(Program));
    int num_inputs = pop ? pop->num_inputs : MAX_INPUTS;
    child->root = mutate_tree(node_copy(parent->root), 0, num_inputs);

    // Possibly inject library calls
    if (pop && pop->library_size > 0 && rand() % 3 == 0) {
        inject_library_calls(child->root, pop, 0);
    }

    child->depth = node_depth(child->root);
    child->size = node_size(child->root);
    child->fitness = -INFINITY;
    return child;
}

// Crossover: swap random subtrees
static Node* get_random_node(Node* node, int* count) {
    if (!node) return NULL;
    if (rand() % (++(*count)) == 0) {
        return node;
    }
    for (int i = 0; i < node->num_children; i++) {
        Node* result = get_random_node(node->children[i], count);
        if (result) return result;
    }
    return node;
}

static void replace_node(Node* target, Node* source) {
    // Destroy old children
    for (int i = 0; i < target->num_children; i++) {
        node_destroy(target->children[i]);
    }

    // Copy new data
    target->op = source->op;
    target->type = source->type;
    target->value = source->value;
    target->num_children = source->num_children;
    for (int i = 0; i < source->num_children; i++) {
        target->children[i] = node_copy(source->children[i]);
    }
}

static Node* crossover_trees(Node* p1, Node* p2) {
    if (!p1 || !p2) return node_copy(p1);

    Node* child = node_copy(p1);

    // Find random crossover point in child
    int count = 0;
    Node* cross_point = get_random_node(child, &count);

    if (cross_point) {
        // Replace with subtree from p2
        count = 0;
        Node* donor = get_random_node(p2, &count);
        if (donor) {
            replace_node(cross_point, donor);
        }
    }

    return child;
}

Program* evolve_crossover(Program* p1, Program* p2) {
    Program* child = calloc(1, sizeof(Program));
    child->root = crossover_trees(p1->root, p2->root);
    child->depth = node_depth(child->root);
    child->size = node_size(child->root);
    child->fitness = -INFINITY;
    return child;
}

// Simplification: remove redundant nodes
void evolve_simplify(Program* prog) {
    // TODO: Implement simplification passes
    // For now, just update metadata
    if (prog && prog->root) {
        prog->depth = node_depth(prog->root);
        prog->size = node_size(prog->root);
    }
}

// Thread data for parallel fitness evaluation
typedef struct {
    Population* pop;
    float (*fitness_fn)(Program*, void*);
    void* data;
    int start_idx;
    int end_idx;
    float partial_fitness;
} ThreadData;

// Worker thread for fitness evaluation
static void* evaluate_fitness_worker(void* arg) {
    ThreadData* td = (ThreadData*)arg;
    td->partial_fitness = 0.0f;

    for (int i = td->start_idx; i < td->end_idx; i++) {
        if (td->pop->programs[i]) {
            td->pop->programs[i]->fitness = td->fitness_fn(td->pop->programs[i], td->data);
            td->partial_fitness += td->pop->programs[i]->fitness;

            // Check for best (with lock)
            pthread_mutex_lock(&td->pop->lock);
            if (td->pop->programs[i]->fitness > td->pop->best_fitness) {
                prog_destroy(td->pop->best);
                td->pop->best = prog_copy(td->pop->programs[i]);
                td->pop->best_fitness = td->pop->programs[i]->fitness;
            }
            pthread_mutex_unlock(&td->pop->lock);
        }
    }

    return NULL;
}

// Tournament selection
static Program* tournament_select(Population* pop) {
    Program* best = NULL;
    float best_fitness = -INFINITY;

    for (int i = 0; i < TOURNAMENT_SIZE; i++) {
        int idx = rand() % POP_SIZE;
        if (pop->programs[idx] && pop->programs[idx]->fitness > best_fitness) {
            best = pop->programs[idx];
            best_fitness = pop->programs[idx]->fitness;
        }
    }

    return best;
}

// Evolution
void evolve_generation(Population* pop, float (*fitness_fn)(Program*, void*), void* data, int num_inputs) {
    // Store num_inputs in population
    pop->num_inputs = num_inputs;

    // Initialize population if empty
    if (!pop->programs[0]) {
        for (int i = 0; i < POP_SIZE; i++) {
            pop->programs[i] = prog_create_random(5, num_inputs);
        }
    }

    // Evaluate fitness in parallel
    int num_threads = 12;  // Number of CPU cores
    pthread_t threads[12];
    ThreadData thread_data[12];

    int chunk_size = POP_SIZE / num_threads;
    int remainder = POP_SIZE % num_threads;

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].pop = pop;
        thread_data[i].fitness_fn = fitness_fn;
        thread_data[i].data = data;
        thread_data[i].start_idx = i * chunk_size;
        thread_data[i].end_idx = (i + 1) * chunk_size;

        // Last thread handles remainder
        if (i == num_threads - 1) {
            thread_data[i].end_idx += remainder;
        }

        pthread_create(&threads[i], NULL, evaluate_fitness_worker, &thread_data[i]);
    }

    // Wait for all threads and accumulate fitness
    float total_fitness = 0;
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        total_fitness += thread_data[i].partial_fitness;
    }
    pop->avg_fitness = total_fitness / POP_SIZE;

    // Create new generation
    Program* new_pop[POP_SIZE];

    // Elitism: keep best programs
    for (int i = 0; i < ELITE_SIZE; i++) {
        // Find best in population
        Program* best = pop->programs[0];
        int best_idx = 0;
        for (int j = 1; j < POP_SIZE; j++) {
            if (pop->programs[j] && pop->programs[j]->fitness > best->fitness) {
                best = pop->programs[j];
                best_idx = j;
            }
        }
        new_pop[i] = prog_copy(best);
        pop->programs[best_idx]->fitness = -INFINITY;  // Mark as used
    }

    // Restore fitness
    for (int i = 0; i < POP_SIZE; i++) {
        if (pop->programs[i] && pop->programs[i]->fitness == -INFINITY) {
            pop->programs[i]->fitness = fitness_fn(pop->programs[i], data);
        }
    }

    // Generate offspring
    for (int i = ELITE_SIZE; i < POP_SIZE; i++) {
        if (rand() % 10 < 7) {  // 70% crossover
            Program* p1 = tournament_select(pop);
            Program* p2 = tournament_select(pop);
            new_pop[i] = evolve_crossover(p1, p2);
        } else {  // 30% mutation
            Program* parent = tournament_select(pop);
            new_pop[i] = evolve_mutate(parent, pop);
        }
    }

    // Replace population
    for (int i = 0; i < POP_SIZE; i++) {
        prog_destroy(pop->programs[i]);
        pop->programs[i] = new_pop[i];
    }

    // Update library every 10 generations
    if (pop->generation % 10 == 0) {
        library_update(pop);
    }

    pop->generation++;
}

// Library learning helpers

// Check if two trees are structurally equivalent
static int trees_equal(Node* a, Node* b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;
    if (a->op != b->op) return 0;
    if (a->op == OP_CONST && a->value != b->value) return 0;
    if (a->op == OP_INPUT && a->value != b->value) return 0;
    if (a->num_children != b->num_children) return 0;

    for (int i = 0; i < a->num_children; i++) {
        if (!trees_equal(a->children[i], b->children[i])) return 0;
    }
    return 1;
}

// Check if pattern already in library
static int library_contains(Population* pop, Node* pattern) {
    for (int i = 0; i < pop->library_size; i++) {
        if (trees_equal(pop->library[i].tree, pattern)) {
            return 1;
        }
    }
    return 0;
}

// Extract all subtrees of a certain size from a tree
static void extract_subtrees(Node* node, Node*** subtrees, int* count, int min_size, int max_size) {
    if (!node || *count >= 100) return;  // Limit extracted subtrees

    int size = node_size(node);
    if (size >= min_size && size <= max_size) {
        // Add this subtree
        (*subtrees)[*count] = node;
        (*count)++;
    }

    // Recursively extract from children
    for (int i = 0; i < node->num_children; i++) {
        extract_subtrees(node->children[i], subtrees, count, min_size, max_size);
    }
}

// Add pattern to library
void library_add(Population* pop, Node* pattern, const char* name, float fitness) {
    if (pop->library_size >= MAX_LIBRARY) {
        // Library full - replace least used entry
        int min_uses = pop->library[0].uses;
        int min_idx = 0;
        for (int i = 1; i < pop->library_size; i++) {
            if (pop->library[i].uses < min_uses) {
                min_uses = pop->library[i].uses;
                min_idx = i;
            }
        }
        // Replace it
        node_destroy(pop->library[min_idx].tree);
        strncpy(pop->library[min_idx].name, name, 31);
        pop->library[min_idx].tree = node_copy(pattern);
        pop->library[min_idx].uses = 1;
        pop->library[min_idx].avg_fitness = fitness;
    } else {
        // Add new entry
        LibraryEntry* entry = &pop->library[pop->library_size++];
        strncpy(entry->name, name, 31);
        entry->tree = node_copy(pattern);
        entry->uses = 1;
        entry->avg_fitness = fitness;
    }
}

// Update library from elite programs
void library_update(Population* pop) {
    // Extract subtrees from top 3 programs
    int num_elite = (ELITE_SIZE < 3) ? ELITE_SIZE : 3;

    // Sort programs by fitness
    Program* sorted[POP_SIZE];
    for (int i = 0; i < POP_SIZE; i++) {
        sorted[i] = pop->programs[i];
    }
    for (int i = 0; i < POP_SIZE - 1; i++) {
        for (int j = i + 1; j < POP_SIZE; j++) {
            if (sorted[j]->fitness > sorted[i]->fitness) {
                Program* tmp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = tmp;
            }
        }
    }

    // Extract subtrees from elite programs
    Node** candidates = malloc(sizeof(Node*) * 100);
    int num_candidates = 0;

    for (int i = 0; i < num_elite; i++) {
        if (sorted[i] && sorted[i]->root) {
            extract_subtrees(sorted[i]->root, &candidates, &num_candidates, 3, 10);
        }
    }

    // Add promising subtrees to library
    int added = 0;
    for (int i = 0; i < num_candidates && added < 2; i++) {
        Node* candidate = candidates[i];

        // Don't add trivial patterns
        if (node_size(candidate) < 3) continue;

        // Don't add if already in library
        if (library_contains(pop, candidate)) continue;

        // Don't add pure terminals or single operations
        if (candidate->num_children == 0) continue;

        // Add to library
        char name[32];
        snprintf(name, 32, "lib%d", pop->library_size);
        library_add(pop, candidate, name, sorted[0]->fitness);
        added++;
    }

    free(candidates);

    // Decay unused library entries
    for (int i = 0; i < pop->library_size; i++) {
        pop->library[i].uses = (int)(pop->library[i].uses * 0.95);  // 5% decay
    }
}
