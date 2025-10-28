#include "gp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// 6-bit multiplexer:
// Inputs: a0, a1 (address), d0, d1, d2, d3 (data)
// Output: data bit at address (a1*2 + a0)
// Example: a0=0, a1=1 -> select d2 (address 2)

float evaluate_mux(Program* prog, void* data) {
    (void)data;

    int correct = 0;
    int total = 64;  // All 2^6 possible inputs

    for (int test = 0; test < total; test++) {
        // Extract bits from test case
        int a0 = (test >> 0) & 1;
        int a1 = (test >> 1) & 1;
        int d0 = (test >> 2) & 1;
        int d1 = (test >> 3) & 1;
        int d2 = (test >> 4) & 1;
        int d3 = (test >> 5) & 1;

        // Calculate expected output
        int address = a1 * 2 + a0;
        int expected;
        if (address == 0) expected = d0;
        else if (address == 1) expected = d1;
        else if (address == 2) expected = d2;
        else expected = d3;

        // Run program
        Context ctx = {0};
        ctx.inputs[0] = a0;
        ctx.inputs[1] = a1;
        ctx.inputs[2] = d0;
        ctx.inputs[3] = d1;
        ctx.inputs[4] = d2;
        ctx.inputs[5] = d3;
        ctx.num_inputs = 6;

        execute_program(prog, &ctx, NULL);

        int result = (ctx.num_outputs > 0) ? (ctx.outputs[0] & 1) : 0;

        if (result == expected) {
            correct++;
        }
    }

    float fitness = (float)correct;

    // Parsimony pressure
    fitness -= prog->size * 0.01f;

    return fitness;
}

int main() {
    srand(time(NULL));

    printf("6-bit Multiplexer Problem\n");
    printf("=========================\n\n");
    printf("Inputs: a0, a1 (address), d0, d1, d2, d3 (data)\n");
    printf("Output: data[a1*2 + a0]\n");
    printf("Test cases: 64 (all possible inputs)\n");
    printf("Population: %d\n\n", POP_SIZE);

    Population* pop = pop_create();

    int max_gen = 1000;
    float best_ever = -INFINITY;
    int no_improvement = 0;

    for (int gen = 0; gen < max_gen; gen++) {
        evolve_generation(pop, evaluate_mux, NULL, 6);

        if (pop->best_fitness > best_ever) {
            best_ever = pop->best_fitness;
            no_improvement = 0;
        } else {
            no_improvement++;
        }

        if (gen % 10 == 0 || pop->best_fitness >= 63.0f) {
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

        if (pop->best_fitness >= 63.9f) {
            printf("\n*** SOLVED! ***\n");
            printf("Final fitness: %.1f / 64.0\n", pop->best_fitness);
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

            // Test on all cases
            printf("\nVerifying all 64 cases:\n");
            int errors = 0;
            for (int test = 0; test < 64; test++) {
                int a0 = (test >> 0) & 1;
                int a1 = (test >> 1) & 1;
                int d0 = (test >> 2) & 1;
                int d1 = (test >> 3) & 1;
                int d2 = (test >> 4) & 1;
                int d3 = (test >> 5) & 1;

                int address = a1 * 2 + a0;
                int expected = (address == 0) ? d0 : (address == 1) ? d1 : (address == 2) ? d2 : d3;

                Context ctx = {0};
                ctx.inputs[0] = a0;
                ctx.inputs[1] = a1;
                ctx.inputs[2] = d0;
                ctx.inputs[3] = d1;
                ctx.inputs[4] = d2;
                ctx.inputs[5] = d3;
                ctx.num_inputs = 6;

                execute_program(pop->best, &ctx, NULL);
                int result = (ctx.num_outputs > 0) ? (ctx.outputs[0] & 1) : 0;

                if (result != expected) {
                    printf("  FAIL: a1=%d a0=%d d=[%d,%d,%d,%d] expected=%d got=%d\n",
                           a1, a0, d0, d1, d2, d3, expected, result);
                    errors++;
                }
            }
            printf("Accuracy: %d/64 = %.1f%%\n", 64 - errors, (64 - errors) * 100.0f / 64);
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
