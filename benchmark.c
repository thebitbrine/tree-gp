#include "gp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// CartPole environment
#define GRAVITY 9.8
#define CART_MASS 1.0
#define POLE_MASS 0.1
#define TOTAL_MASS (CART_MASS + POLE_MASS)
#define POLE_LENGTH 0.5
#define POLE_MASS_LENGTH (POLE_MASS * POLE_LENGTH)
#define FORCE_MAG 10.0
#define TAU 0.02

#define X_THRESHOLD 2.4
#define THETA_THRESHOLD_RADIANS (12.0 * M_PI / 180.0)

typedef struct {
    float x, x_dot, theta, theta_dot;
} CartPoleState;

void cartpole_reset(CartPoleState* state) {
    state->x = ((rand() % 200) - 100) / 1000.0;
    state->x_dot = ((rand() % 200) - 100) / 1000.0;
    state->theta = ((rand() % 200) - 100) / 1000.0;
    state->theta_dot = ((rand() % 200) - 100) / 1000.0;
}

int cartpole_is_done(CartPoleState* state) {
    return (fabs(state->x) > X_THRESHOLD ||
            fabs(state->theta) > THETA_THRESHOLD_RADIANS);
}

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

float evaluate_cartpole(Program* prog, void* data) {
    (void)data;

    int num_episodes = 10;
    int max_steps = 500;
    int total_steps = 0;

    for (int ep = 0; ep < num_episodes; ep++) {
        CartPoleState state;
        cartpole_reset(&state);

        for (int step = 0; step < max_steps; step++) {
            Context ctx = {0};
            ctx.inputs[0] = (int)(state.x * 100);
            ctx.inputs[1] = (int)(state.x_dot * 100);
            ctx.inputs[2] = (int)(state.theta * 100);
            ctx.inputs[3] = (int)(state.theta_dot * 100);
            ctx.num_inputs = 4;

            execute_program(prog, &ctx, NULL);

            int action = (ctx.num_outputs > 0 && ctx.outputs[0] > 0) ? 1 : 0;
            cartpole_step(&state, action);

            if (cartpole_is_done(&state)) {
                break;
            }
            total_steps++;
        }
    }

    float avg_steps = (float)total_steps / num_episodes;
    float fitness = avg_steps;

    float complexity_penalty = prog->size * 0.1f;
    fitness -= complexity_penalty;

    return fitness;
}

int main() {
    srand(time(NULL));

    printf("Multi-threaded GP Benchmark - CartPole\n");
    printf("======================================\n\n");
    printf("Population: %d, Fixed generations: 100\n\n", POP_SIZE);

    Population* pop = pop_create();

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int gen = 0; gen < 100; gen++) {
        evolve_generation(pop, evaluate_cartpole, NULL, 4);

        if (gen % 10 == 0) {
            printf("Gen %3d: Best=%.1f Avg=%.1f Size=%d Depth=%d\n",
                   gen,
                   pop->best_fitness,
                   pop->avg_fitness,
                   pop->best ? pop->best->size : 0,
                   pop->best ? pop->best->depth : 0);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (end.tv_sec - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\n100 generations completed in %.2f seconds\n", elapsed);
    printf("Average: %.3f seconds per generation\n", elapsed / 100.0);
    printf("Final best fitness: %.1f\n", pop->best_fitness);

    pop_destroy(pop);
    return 0;
}
