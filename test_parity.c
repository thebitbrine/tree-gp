#include "gp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// 3-bit even parity:
// Output 1 if even number of 1s in input (including 0)
// Output 0 if odd number of 1s

float evaluate_parity(Program* prog, void* data) {
    (void)data;

    int correct = 0;
    int total = 8;  // 2^3 = 8 test cases

    for (int test = 0; test < total; test++) {
        int b0 = (test >> 0) & 1;
        int b1 = (test >> 1) & 1;
        int b2 = (test >> 2) & 1;

        // Even parity: XOR of all bits
        int expected = (b0 ^ b1 ^ b2) == 0 ? 1 : 0;

        Context ctx = {0};
        ctx.inputs[0] = b0;
        ctx.inputs[1] = b1;
        ctx.inputs[2] = b2;
        ctx.num_inputs = 3;

        execute_program(prog, &ctx, NULL);

        int result = (ctx.num_outputs > 0) ? (ctx.outputs[0] & 1) : 0;

        if (result == expected) {
            correct++;
        }
    }

    float fitness = (float)correct;
    fitness -= prog->size * 0.01f;  // Parsimony

    return fitness;
}

int main() {
    srand(time(NULL));

    printf("3-bit Even Parity Problem\n");
    printf("=========================\n\n");
    printf("Inputs: b0, b1, b2 (bits)\n");
    printf("Output: 1 if even number of 1s, 0 if odd\n");
    printf("Test cases: 8 (all possible inputs)\n");
    printf("Population: %d\n\n", POP_SIZE);

    Population* pop = pop_create();

    int max_gen = 500;
    float best_ever = -INFINITY;
    int no_improvement = 0;

    for (int gen = 0; gen < max_gen; gen++) {
        evolve_generation(pop, evaluate_parity, NULL, 3);

        if (pop->best_fitness > best_ever) {
            best_ever = pop->best_fitness;
            no_improvement = 0;
        } else {
            no_improvement++;
        }

        if (gen % 10 == 0 || pop->best_fitness >= 7.9f) {
            printf("Gen %4d: Best=%.1f Avg=%.1f Size=%d Lib=%d\n",
                   gen,
                   pop->best_fitness,
                   pop->avg_fitness,
                   pop->best ? pop->best->size : 0,
                   pop->library_size);

            if (gen % 50 == 0 && pop->library_size > 0) {
                printf("  Library top 3:\n");
                for (int i = 0; i < pop->library_size && i < 3; i++) {
                    printf("    %s (params=%d, uses=%d)\n",
                           pop->library[i].name,
                           pop->library[i].num_params,
                           pop->library[i].uses);
                }
            }
        }

        if (pop->best_fitness >= 7.9f) {
            printf("\n*** SOLVED! ***\n");
            printf("Final fitness: %.1f / 8.0\n", pop->best_fitness);
            printf("Solution size: %d nodes\n", pop->best->size);
            printf("\nSolution tree:\n");
            print_tree(pop->best->root, 0);

            printf("\nLibrary (%d entries):\n", pop->library_size);
            for (int i = 0; i < pop->library_size && i < 5; i++) {
                printf("\n[%d] %s (params=%d, uses=%d):\n",
                       i, pop->library[i].name,
                       pop->library[i].num_params,
                       pop->library[i].uses);
                print_tree(pop->library[i].tree, 1);
            }

            // Verify all 8 cases
            printf("\nVerifying all 8 cases:\n");
            for (int test = 0; test < 8; test++) {
                int b0 = (test >> 0) & 1;
                int b1 = (test >> 1) & 1;
                int b2 = (test >> 2) & 1;
                int expected = (b0 ^ b1 ^ b2) == 0 ? 1 : 0;

                Context ctx = {0};
                ctx.inputs[0] = b0;
                ctx.inputs[1] = b1;
                ctx.inputs[2] = b2;
                ctx.num_inputs = 3;

                execute_program(pop->best, &ctx, NULL);
                int result = (ctx.num_outputs > 0) ? (ctx.outputs[0] & 1) : 0;

                printf("  %d %d %d -> %d (got %d) %s\n",
                       b0, b1, b2, expected, result,
                       result == expected ? "OK" : "FAIL");
            }
            break;
        }

        if (no_improvement > 100) {
            printf("\nNo improvement for 100 generations.\n");
            break;
        }
    }

    printf("\nFinal best (fitness: %.1f):\n", pop->best_fitness);
    if (pop->best && pop->best->root) {
        print_tree(pop->best->root, 0);
    }

    pop_destroy(pop);
    return 0;
}
