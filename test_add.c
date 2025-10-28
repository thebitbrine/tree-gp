#include "gp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Fitness function for a+b task
float evaluate_add(Program* prog, void* data) {
    (void)data;  // Unused

    float total_error = 0.0f;
    int test_cases = 10;

    for (int i = 0; i < test_cases; i++) {
        int a = rand() % 20;
        int b = rand() % 20;
        int expected = a + b;

        Context ctx = {0};
        ctx.inputs[0] = a;
        ctx.inputs[1] = b;
        ctx.num_inputs = 2;

        execute_program(prog, &ctx, NULL);

        int result = (ctx.num_outputs > 0) ? ctx.outputs[0] : 0;
        int error = abs(result - expected);
        total_error += error;
    }

    float avg_error = total_error / test_cases;
    float fitness = 100.0f - avg_error;

    // Parsimony pressure: penalize overly complex solutions
    float complexity_penalty = prog->size * 0.01f;
    fitness -= complexity_penalty;

    return fitness;
}

void print_tree(Node* node, int indent) {
    if (!node) return;

    for (int i = 0; i < indent; i++) printf("  ");

    OpInfo* info = NULL;
    for (int i = 0; i < OP_COUNT; i++) {
        if (op_info[i].op == node->op) {
            info = &op_info[i];
            break;
        }
    }

    if (info) {
        printf("%s", info->name);
        if (node->op == OP_CONST) {
            printf("(%d)", node->value);
        } else if (node->op == OP_INPUT) {
            printf("[%d]", node->value);
        }
        printf("\n");

        for (int i = 0; i < node->num_children; i++) {
            print_tree(node->children[i], indent + 1);
        }
    }
}

int main() {
    srand(time(NULL));

    printf("Tree-based GP - Simple Add Test\n");
    printf("================================\n\n");
    printf("Task: Learn to output a + b given inputs [a, b]\n");
    printf("Population: %d, Tournament: %d, Elite: %d\n\n", POP_SIZE, TOURNAMENT_SIZE, ELITE_SIZE);

    Population* pop = pop_create();

    int max_gen = 1000;
    int no_improvement = 0;

    for (int gen = 0; gen < max_gen; gen++) {
        evolve_generation(pop, evaluate_add, NULL, 2);  // 2 inputs: a, b

        if (pop->best && pop->best->fitness > pop->best_fitness - 0.01f) {
            no_improvement++;
        } else {
            no_improvement = 0;
        }

        if (gen % 10 == 0 || pop->best_fitness > 99.0f) {
            printf("Gen %4d: Best=%.2f Avg=%.2f Size=%d Depth=%d\n",
                   gen,
                   pop->best_fitness,
                   pop->avg_fitness,
                   pop->best ? pop->best->size : 0,
                   pop->best ? pop->best->depth : 0);
        }

        if (pop->best_fitness >= 99.0f) {
            printf("\n*** TASK SOLVED! ***\n");
            printf("Final fitness: %.2f\n", pop->best_fitness);
            printf("Solution size: %d nodes\n", pop->best->size);
            printf("Solution depth: %d\n", pop->best->depth);
            printf("\nSolution tree:\n");
            print_tree(pop->best->root, 0);

            // Test on fresh cases
            printf("\nTesting on 20 new cases:\n");
            int correct = 0;
            for (int i = 0; i < 20; i++) {
                int a = rand() % 20;
                int b = rand() % 20;
                int expected = a + b;

                Context ctx = {0};
                ctx.inputs[0] = a;
                ctx.inputs[1] = b;
                ctx.num_inputs = 2;

                execute_program(pop->best, &ctx, NULL);

                int result = (ctx.num_outputs > 0) ? ctx.outputs[0] : 0;
                int match = (result == expected) ? 1 : 0;
                correct += match;

                printf("  %2d + %2d = %2d (got %2d) %s\n",
                       a, b, expected, result, match ? "OK" : "FAIL");
            }
            printf("Accuracy: %d/20 = %.1f%%\n", correct, (correct * 100.0f / 20));
            break;
        }

        if (no_improvement > 100) {
            printf("\nNo improvement for 100 generations.\n");
            break;
        }
    }

    printf("\nFinal best solution (fitness: %.2f):\n", pop->best_fitness);
    if (pop->best && pop->best->root) {
        print_tree(pop->best->root, 0);
    }

    pop_destroy(pop);
    return 0;
}
