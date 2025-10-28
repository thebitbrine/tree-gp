#include "gp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// CartPole physics constants
#define GRAVITY 9.8
#define CART_MASS 1.0
#define POLE_MASS 0.1
#define TOTAL_MASS (CART_MASS + POLE_MASS)
#define POLE_LENGTH 0.5
#define POLE_MASS_LENGTH (POLE_MASS * POLE_LENGTH)
#define FORCE_MAG 10.0
#define TAU 0.02  // Time step

// CartPole state
typedef struct {
    float x;          // Cart position
    float x_dot;      // Cart velocity
    float theta;      // Pole angle
    float theta_dot;  // Pole angular velocity
} CartPoleState;

// Physics simulation
void cartpole_step(CartPoleState* state, int action) {
    float force = (action == 1) ? FORCE_MAG : -FORCE_MAG;
    float costheta = cos(state->theta);
    float sintheta = sin(state->theta);

    float temp = (force + POLE_MASS_LENGTH * state->theta_dot * state->theta_dot * sintheta) / TOTAL_MASS;
    float theta_acc = (GRAVITY * sintheta - costheta * temp) /
                      (POLE_LENGTH * (4.0/3.0 - POLE_MASS * costheta * costheta / TOTAL_MASS));
    float x_acc = temp - POLE_MASS_LENGTH * theta_acc * costheta / TOTAL_MASS;

    state->x += TAU * state->x_dot;
    state->x_dot += TAU * x_acc;
    state->theta += TAU * state->theta_dot;
    state->theta_dot += TAU * theta_acc;
}

int cartpole_is_failed(CartPoleState* state) {
    return (fabs(state->x) > 2.4 || fabs(state->theta) > 0.2095);  // ~12 degrees
}

// Fitness function for CartPole
float evaluate_cartpole(Program* prog, void* data) {
    (void)data;

    int num_episodes = 10;  // More episodes for more accurate fitness
    float total_reward = 0.0f;

    for (int ep = 0; ep < num_episodes; ep++) {
        CartPoleState state = {
            .x = ((float)rand() / RAND_MAX - 0.5f) * 0.1f,
            .x_dot = 0.0f,
            .theta = ((float)rand() / RAND_MAX - 0.5f) * 0.1f,
            .theta_dot = 0.0f
        };

        int steps = 0;
        int max_steps = 500;

        while (steps < max_steps && !cartpole_is_failed(&state)) {
            // Execute program to get action
            Context ctx = {0};
            // Scale inputs to reasonable range
            ctx.inputs[0] = (int)(state.x * 100);           // Position
            ctx.inputs[1] = (int)(state.x_dot * 100);       // Velocity
            ctx.inputs[2] = (int)(state.theta * 100);       // Angle
            ctx.inputs[3] = (int)(state.theta_dot * 100);   // Angular velocity
            ctx.num_inputs = 4;

            execute_program(prog, &ctx, NULL);

            // Get action (0 = left, 1 = right)
            int action = 0;
            if (ctx.num_outputs > 0) {
                action = (ctx.outputs[0] > 0) ? 1 : 0;
            }

            cartpole_step(&state, action);
            steps++;
        }

        total_reward += steps;
    }

    float avg_reward = total_reward / num_episodes;

    // Parsimony pressure
    float complexity_penalty = prog->size * 0.1f;
    return avg_reward - complexity_penalty;
}

int main() {
    srand(time(NULL));

    printf("Tree-based GP - CartPole Test\n");
    printf("==============================\n\n");
    printf("Task: Learn to balance pole on cart\n");
    printf("State: [x, x_dot, theta, theta_dot]\n");
    printf("Action: 0=left, 1=right (based on output > 0)\n");
    printf("Success: Balance for 500 steps\n");
    printf("Population: %d, Tournament: %d, Elite: %d\n\n", POP_SIZE, TOURNAMENT_SIZE, ELITE_SIZE);

    Population* pop = pop_create();

    int max_gen = 5000;
    int no_improvement = 0;
    float best_ever = -INFINITY;

    for (int gen = 0; gen < max_gen; gen++) {
        evolve_generation(pop, evaluate_cartpole, NULL, 4);  // 4 inputs: x, x_dot, theta, theta_dot

        if (pop->best_fitness > best_ever) {
            best_ever = pop->best_fitness;
            no_improvement = 0;
        } else {
            no_improvement++;
        }

        if (gen % 50 == 0 || pop->best_fitness >= 490.0f) {
            printf("Gen %4d: Best=%.1f Avg=%.1f Size=%d Depth=%d\n",
                   gen,
                   pop->best_fitness,
                   pop->avg_fitness,
                   pop->best ? pop->best->size : 0,
                   pop->best ? pop->best->depth : 0);
        }

        if (pop->best_fitness >= 490.0f) {
            printf("\n*** TASK SOLVED! ***\n");
            printf("Final fitness: %.1f\n", pop->best_fitness);
            printf("Solution size: %d nodes\n", pop->best->size);
            printf("Solution depth: %d\n", pop->best->depth);
            printf("\nSolution tree:\n");
            print_tree(pop->best->root, 0);

            // Test on multiple runs
            printf("\nTesting on 20 episodes:\n");
            int successes = 0;
            float total_steps = 0;

            for (int i = 0; i < 20; i++) {
                CartPoleState state = {
                    .x = ((float)rand() / RAND_MAX - 0.5f) * 0.1f,
                    .x_dot = 0.0f,
                    .theta = ((float)rand() / RAND_MAX - 0.5f) * 0.1f,
                    .theta_dot = 0.0f
                };

                int steps = 0;
                int max_steps = 500;

                while (steps < max_steps && !cartpole_is_failed(&state)) {
                    Context ctx = {0};
                    ctx.inputs[0] = (int)(state.x * 100);
                    ctx.inputs[1] = (int)(state.x_dot * 100);
                    ctx.inputs[2] = (int)(state.theta * 100);
                    ctx.inputs[3] = (int)(state.theta_dot * 100);
                    ctx.num_inputs = 4;

                    execute_program(pop->best, &ctx, NULL);

                    int action = 0;
                    if (ctx.num_outputs > 0) {
                        action = (ctx.outputs[0] > 0) ? 1 : 0;
                    }

                    cartpole_step(&state, action);
                    steps++;
                }

                total_steps += steps;
                if (steps >= 500) successes++;

                printf("  Episode %2d: %3d steps %s\n", i+1, steps, steps >= 500 ? "SUCCESS" : "");
            }

            printf("\nSuccess rate: %d/20 = %.1f%%\n", successes, (successes * 100.0f / 20));
            printf("Average steps: %.1f\n", total_steps / 20);
            break;
        }

        if (no_improvement > 500) {
            printf("\nNo improvement for 500 generations.\n");
            break;
        }
    }

    printf("\nFinal best solution (fitness: %.1f):\n", pop->best_fitness);
    if (pop->best && pop->best->root) {
        print_tree(pop->best->root, 0);
    }

    pop_destroy(pop);
    return 0;
}
