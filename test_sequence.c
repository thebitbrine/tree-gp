#include "gp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Test: accumulate sequence
// Task: Given a sequence of numbers, output the running sum
// Example: inputs [3, 5, 2] -> outputs [3, 8, 10]
// Requires memory to track sum so far

float evaluate_sequence(Program* prog, void* data) {
    (void)data;

    float total_error = 0.0f;
    int num_tests = 10;

    for (int test = 0; test < num_tests; test++) {
        Context ctx = {0};  // Zero-initialize including memory

        // Generate random sequence of 5 numbers
        int sequence[5];
        for (int i = 0; i < 5; i++) {
            sequence[i] = rand() % 10;
        }

        // Run program on each number, expect running sum
        int running_sum = 0;
        for (int i = 0; i < 5; i++) {
            ctx.inputs[0] = sequence[i];
            ctx.num_inputs = 1;
            ctx.num_outputs = 0;  // Reset outputs

            execute_program(prog, &ctx, NULL);

            running_sum += sequence[i];
            int expected = running_sum;
            int result = (ctx.num_outputs > 0) ? ctx.outputs[0] : 0;
            int error = abs(result - expected);
            total_error += error;
        }
    }

    float avg_error = total_error / (num_tests * 5);
    float fitness = 100.0f - avg_error;
    fitness -= prog->size * 0.01f;  // Parsimony
    return fitness;
}

int main() {
    srand(time(NULL));

    printf("Tree-based GP - Sequence Accumulation Test\n");
    printf("==========================================\n\n");
    printf("Task: Output running sum of inputs\n");
    printf("Example: inputs [3,5,2] -> outputs [3,8,10]\n");
    printf("Requires memory to track sum\n\n");

    Population* pop = pop_create();

    int max_gen = 2000;
    float best_ever = -INFINITY;
    int no_improvement = 0;

    for (int gen = 0; gen < max_gen; gen++) {
        evolve_generation(pop, evaluate_sequence, NULL, 1);

        if (pop->best_fitness > best_ever) {
            best_ever = pop->best_fitness;
            no_improvement = 0;
        } else {
            no_improvement++;
        }

        if (gen % 50 == 0 || pop->best_fitness >= 98.0f) {
            printf("Gen %4d: Best=%.1f Avg=%.1f Size=%d Depth=%d\n",
                   gen,
                   pop->best_fitness,
                   pop->avg_fitness,
                   pop->best ? pop->best->size : 0,
                   pop->best ? pop->best->depth : 0);
        }

        if (pop->best_fitness >= 98.0f) {
            printf("\n*** TASK SOLVED! ***\n");
            printf("Final fitness: %.1f\n", pop->best_fitness);
            printf("Solution size: %d nodes\n", pop->best->size);
            printf("Solution depth: %d\n", pop->best->depth);
            printf("\nSolution tree:\n");
            print_tree(pop->best->root, 0);

            // Test on specific example
            printf("\nTesting on sequence [3, 5, 2, 7, 1]:\n");
            Context ctx = {0};
            int test_seq[] = {3, 5, 2, 7, 1};
            int running_sum = 0;

            for (int i = 0; i < 5; i++) {
                ctx.inputs[0] = test_seq[i];
                ctx.num_inputs = 1;
                ctx.num_outputs = 0;

                execute_program(pop->best, &ctx, NULL);

                running_sum += test_seq[i];
                int result = (ctx.num_outputs > 0) ? ctx.outputs[0] : 0;

                printf("  Input=%d, Expected=%d, Got=%d %s\n",
                       test_seq[i], running_sum, result,
                       (result == running_sum) ? "OK" : "WRONG");
            }

            break;
        }

        if (no_improvement > 500) {
            printf("\nNo improvement for 500 generations, stopping.\n");
            break;
        }
    }

    if (pop->best_fitness < 98.0f) {
        printf("\nDid not fully solve (best: %.1f)\n", pop->best_fitness);
        printf("Best solution:\n");
        print_tree(pop->best->root, 0);
    }

    pop_destroy(pop);
    return 0;
}
