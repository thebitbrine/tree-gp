#include "gp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// 11-bit multiplexer:
// Inputs: a0, a1, a2 (3 address bits), d0...d7 (8 data bits)
// Output: data bit at address (a2*4 + a1*2 + a0)
// Much harder than 6-bit version!

float evaluate_mux(Program* prog, void* data) {
    (void)data;

    int correct = 0;
    int total = 2048;  // All 2^11 possible inputs

    for (int test = 0; test < total; test++) {
        // Extract bits from test case
        int a0 = (test >> 0) & 1;
        int a1 = (test >> 1) & 1;
        int a2 = (test >> 2) & 1;
        int d0 = (test >> 3) & 1;
        int d1 = (test >> 4) & 1;
        int d2 = (test >> 5) & 1;
        int d3 = (test >> 6) & 1;
        int d4 = (test >> 7) & 1;
        int d5 = (test >> 8) & 1;
        int d6 = (test >> 9) & 1;
        int d7 = (test >> 10) & 1;

        // Calculate expected output
        int address = a2 * 4 + a1 * 2 + a0;
        int expected;
        switch(address) {
            case 0: expected = d0; break;
            case 1: expected = d1; break;
            case 2: expected = d2; break;
            case 3: expected = d3; break;
            case 4: expected = d4; break;
            case 5: expected = d5; break;
            case 6: expected = d6; break;
            case 7: expected = d7; break;
            default: expected = 0;
        }

        // Run program
        Context ctx = {0};
        ctx.inputs[0] = a0;
        ctx.inputs[1] = a1;
        ctx.inputs[2] = a2;
        ctx.inputs[3] = d0;
        ctx.inputs[4] = d1;
        ctx.inputs[5] = d2;
        ctx.inputs[6] = d3;
        ctx.inputs[7] = d4;
        ctx.inputs[8] = d5;
        ctx.inputs[9] = d6;
        ctx.inputs[10] = d7;
        ctx.num_inputs = 11;

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

    printf("11-bit Multiplexer Problem\n");
    printf("==========================\n\n");
    printf("Inputs: a0, a1, a2 (address), d0...d7 (data)\n");
    printf("Output: data[a2*4 + a1*2 + a0]\n");
    printf("Test cases: 2048 (all possible inputs)\n");
    printf("Population: %d\n\n", POP_SIZE);

    Population* pop = pop_create();

    int max_gen = 5000;
    float best_ever = -INFINITY;
    int no_improvement = 0;

    for (int gen = 0; gen < max_gen; gen++) {
        evolve_generation(pop, evaluate_mux, NULL, 11);

        if (pop->best_fitness > best_ever) {
            best_ever = pop->best_fitness;
            no_improvement = 0;
        } else {
            no_improvement++;
        }

        if (gen % 10 == 0 || pop->best_fitness >= 2040.0f) {
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

        if (pop->best_fitness >= 2040.0f) {
            printf("\n*** SOLVED! ***\n");
            printf("Final fitness: %.1f / 2048.0\n", pop->best_fitness);
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

            // Test on all 2048 cases (show first 20 errors)
            printf("\nVerifying all 2048 cases:\n");
            int errors = 0;
            int shown_errors = 0;
            for (int test = 0; test < 2048; test++) {
                int a0 = (test >> 0) & 1;
                int a1 = (test >> 1) & 1;
                int a2 = (test >> 2) & 1;
                int d0 = (test >> 3) & 1;
                int d1 = (test >> 4) & 1;
                int d2 = (test >> 5) & 1;
                int d3 = (test >> 6) & 1;
                int d4 = (test >> 7) & 1;
                int d5 = (test >> 8) & 1;
                int d6 = (test >> 9) & 1;
                int d7 = (test >> 10) & 1;

                int address = a2 * 4 + a1 * 2 + a0;
                int expected;
                switch(address) {
                    case 0: expected = d0; break;
                    case 1: expected = d1; break;
                    case 2: expected = d2; break;
                    case 3: expected = d3; break;
                    case 4: expected = d4; break;
                    case 5: expected = d5; break;
                    case 6: expected = d6; break;
                    case 7: expected = d7; break;
                    default: expected = 0;
                }

                Context ctx = {0};
                ctx.inputs[0] = a0;
                ctx.inputs[1] = a1;
                ctx.inputs[2] = a2;
                ctx.inputs[3] = d0;
                ctx.inputs[4] = d1;
                ctx.inputs[5] = d2;
                ctx.inputs[6] = d3;
                ctx.inputs[7] = d4;
                ctx.inputs[8] = d5;
                ctx.inputs[9] = d6;
                ctx.inputs[10] = d7;
                ctx.num_inputs = 11;

                execute_program(pop->best, &ctx, NULL);
                int result = (ctx.num_outputs > 0) ? (ctx.outputs[0] & 1) : 0;

                if (result != expected) {
                    if (shown_errors < 20) {
                        printf("  FAIL: addr=%d d=[%d,%d,%d,%d,%d,%d,%d,%d] expected=%d got=%d\n",
                               address, d0, d1, d2, d3, d4, d5, d6, d7, expected, result);
                        shown_errors++;
                    }
                    errors++;
                }
            }
            printf("Accuracy: %d/2048 = %.1f%%\n", 2048 - errors, (2048 - errors) * 100.0f / 2048);
            break;
        }

        if (no_improvement > 500) {
            printf("\nNo improvement for 500 generations.\n");
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
