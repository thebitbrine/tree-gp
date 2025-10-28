#include "gp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Simple fitness: compute a + b using evolved ADF
float evaluate_add_adf(Program* prog, void* data) {
    (void)data;

    int correct = 0;
    int total = 20;

    for (int i = 0; i < total; i++) {
        int a = rand() % 20 - 10;
        int b = rand() % 20 - 10;

        Context ctx = {0};
        ctx.inputs[0] = a;
        ctx.inputs[1] = b;
        ctx.num_inputs = 2;

        execute_program(prog, &ctx, NULL);

        if (ctx.num_outputs > 0 && ctx.outputs[0] == a + b) {
            correct++;
        }
    }

    return (float)correct;
}

int main() {
    srand(time(NULL));

    printf("ADF Test - Learning Addition\n");
    printf("==============================\n\n");

    Population* pop = pop_create();

    // Run evolution
    for (int gen = 0; gen < 50; gen++) {
        evolve_generation(pop, evaluate_add_adf, NULL, 2);

        if (gen % 10 == 0 || pop->best_fitness >= 20.0) {
            printf("Gen %3d: Best=%.1f Avg=%.1f Size=%d LibSize=%d\n",
                   gen, pop->best_fitness, pop->avg_fitness,
                   pop->best ? pop->best->size : 0,
                   pop->library_size);

            // Show library entries with their parameterization
            if (pop->library_size > 0) {
                printf("  Library (%d entries):\n", pop->library_size);
                for (int i = 0; i < pop->library_size && i < 5; i++) {
                    printf("    %s (params=%d, uses=%d):\n",
                           pop->library[i].name,
                           pop->library[i].num_params,
                           pop->library[i].uses);
                    print_tree(pop->library[i].tree, 3);
                }
            }
        }

        if (pop->best_fitness >= 20.0) {
            printf("\nSolved! Best solution:\n");
            print_tree(pop->best->root, 0);
            break;
        }
    }

    pop_destroy(pop);
    return 0;
}
