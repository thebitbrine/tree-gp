#include "gp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Taxi-v3 environment
#define TAXI_SIZE 5
#define MAX_STEPS 200

// Actions
#define ACTION_SOUTH 0
#define ACTION_NORTH 1
#define ACTION_EAST 2
#define ACTION_WEST 3
#define ACTION_PICKUP 4
#define ACTION_DROPOFF 5

// Passenger locations: R(0,0), G(0,4), Y(4,0), B(4,3)
static int LOCS[4][2] = {{0,0}, {0,4}, {4,0}, {4,3}};

typedef struct {
    int taxi_row;
    int taxi_col;
    int pass_loc;   // 0-3 = at location, 4 = in taxi
    int dest_loc;   // 0-3
} TaxiState;

void taxi_reset(TaxiState* state) {
    state->taxi_row = rand() % TAXI_SIZE;
    state->taxi_col = rand() % TAXI_SIZE;
    state->pass_loc = rand() % 4;  // Passenger at one of 4 locations
    do {
        state->dest_loc = rand() % 4;
    } while (state->dest_loc == state->pass_loc);  // Different from pickup
}

int taxi_encode(TaxiState* state) {
    // Encode state to 0-499
    return state->taxi_row + state->taxi_col * 5 + state->pass_loc * 25 + state->dest_loc * 125;
}

void taxi_decode(int encoded, TaxiState* state) {
    state->dest_loc = encoded / 125;
    encoded %= 125;
    state->pass_loc = encoded / 25;
    encoded %= 25;
    state->taxi_col = encoded / 5;
    state->taxi_row = encoded % 5;
}

int taxi_step(TaxiState* state, int action, int* reward) {
    *reward = -1;  // Per-step penalty

    int new_row = state->taxi_row;
    int new_col = state->taxi_col;

    // Movement
    if (action == ACTION_SOUTH) new_row = (state->taxi_row < 4) ? state->taxi_row + 1 : state->taxi_row;
    else if (action == ACTION_NORTH) new_row = (state->taxi_row > 0) ? state->taxi_row - 1 : state->taxi_row;
    else if (action == ACTION_EAST) new_col = (state->taxi_col < 4) ? state->taxi_col + 1 : state->taxi_col;
    else if (action == ACTION_WEST) new_col = (state->taxi_col > 0) ? state->taxi_col - 1 : state->taxi_col;
    else if (action == ACTION_PICKUP) {
        if (state->pass_loc < 4) {
            // Check if at passenger location
            if (state->taxi_row == LOCS[state->pass_loc][0] &&
                state->taxi_col == LOCS[state->pass_loc][1]) {
                state->pass_loc = 4;  // In taxi
                *reward = -1;
                return 0;
            } else {
                *reward = -10;  // Illegal pickup
                return 0;
            }
        } else {
            *reward = -10;  // Already in taxi
            return 0;
        }
    } else if (action == ACTION_DROPOFF) {
        if (state->pass_loc == 4) {
            // Check if at destination
            if (state->taxi_row == LOCS[state->dest_loc][0] &&
                state->taxi_col == LOCS[state->dest_loc][1]) {
                *reward = 20;
                return 1;  // Success!
            } else {
                *reward = -10;  // Illegal dropoff
                return 0;
            }
        } else {
            *reward = -10;  // No passenger
            return 0;
        }
    }

    state->taxi_row = new_row;
    state->taxi_col = new_col;
    return 0;
}

float evaluate_taxi(Program* prog, void* data) {
    (void)data;

    int num_episodes = 10;
    float total_reward = 0;
    int total_successes = 0;

    for (int ep = 0; ep < num_episodes; ep++) {
        TaxiState state;
        taxi_reset(&state);

        Context ctx = {0};
        int episode_reward = 0;

        int initial_pass_dist = abs(state.taxi_row - LOCS[state.pass_loc][0]) +
                                abs(state.taxi_col - LOCS[state.pass_loc][1]);
        int min_pass_dist = initial_pass_dist;
        int min_dest_dist = 999;
        int picked_up = 0;

        for (int step = 0; step < MAX_STEPS; step++) {
            // Decode state into meaningful inputs
            ctx.inputs[0] = state.taxi_row;
            ctx.inputs[1] = state.taxi_col;
            ctx.inputs[2] = state.pass_loc;
            ctx.inputs[3] = state.dest_loc;
            if (state.pass_loc < 4) {
                ctx.inputs[4] = LOCS[state.pass_loc][0];  // Passenger row
                ctx.inputs[5] = LOCS[state.pass_loc][1];  // Passenger col
            } else {
                ctx.inputs[4] = -1;
                ctx.inputs[5] = -1;
            }
            ctx.inputs[6] = LOCS[state.dest_loc][0];  // Destination row
            ctx.inputs[7] = LOCS[state.dest_loc][1];  // Destination col
            ctx.num_inputs = 8;
            ctx.num_outputs = 0;

            execute_program(prog, &ctx, NULL);

            int action = 0;
            if (ctx.num_outputs > 0) {
                action = abs(ctx.outputs[0]) % 6;
            }

            int reward;
            int done = taxi_step(&state, action, &reward);
            episode_reward += reward;

            // Reward shaping
            if (state.pass_loc < 4) {
                // Not picked up yet - reward getting closer to passenger
                int pass_dist = abs(state.taxi_row - LOCS[state.pass_loc][0]) +
                                abs(state.taxi_col - LOCS[state.pass_loc][1]);
                if (pass_dist < min_pass_dist) {
                    episode_reward += 2;
                    min_pass_dist = pass_dist;
                }
            } else {
                // Picked up - reward getting closer to destination
                if (!picked_up) {
                    episode_reward += 30;  // Big bonus for successful pickup
                    picked_up = 1;
                    min_dest_dist = abs(state.taxi_row - LOCS[state.dest_loc][0]) +
                                    abs(state.taxi_col - LOCS[state.dest_loc][1]);
                }
                int dest_dist = abs(state.taxi_row - LOCS[state.dest_loc][0]) +
                                abs(state.taxi_col - LOCS[state.dest_loc][1]);
                if (dest_dist < min_dest_dist) {
                    episode_reward += 2;
                    min_dest_dist = dest_dist;
                }
            }

            if (done) {
                total_successes++;
                break;
            }
        }

        total_reward += episode_reward;
    }

    float fitness = total_reward / num_episodes;
    fitness -= prog->size * 0.05f;
    return fitness;
}

int main() {
    srand(time(NULL));

    printf("Tree-based GP - Taxi-v3\n");
    printf("=======================\n\n");
    printf("Task: Pick up passenger and drop off at destination\n");
    printf("State: taxi position, passenger location, destination\n");
    printf("Actions: 0=S, 1=N, 2=E, 3=W, 4=pickup, 5=dropoff\n");
    printf("Rewards: +20 success, -10 illegal, -1 per step\n");
    printf("Population: %d\n\n", POP_SIZE);

    Population* pop = pop_create();

    int max_gen = 500;
    float best_ever = -INFINITY;
    int no_improvement = 0;

    for (int gen = 0; gen < max_gen; gen++) {
        evolve_generation(pop, evaluate_taxi, NULL, 8);

        if (pop->best_fitness > best_ever) {
            best_ever = pop->best_fitness;
            no_improvement = 0;
        } else {
            no_improvement++;
        }

        if (gen % 100 == 0 || pop->best_fitness >= 7.0f) {
            printf("Gen %4d: Best=%.1f Avg=%.1f Size=%d Lib=%d\n",
                   gen,
                   pop->best_fitness,
                   pop->avg_fitness,
                   pop->best ? pop->best->size : 0,
                   pop->library_size);
        }

        if (pop->best_fitness >= 7.0f) {
            printf("\n*** GOOD SOLUTION FOUND! ***\n");
            printf("Final fitness: %.1f\n", pop->best_fitness);
            printf("Solution size: %d nodes\n", pop->best->size);
            printf("\nSolution tree:\n");
            print_tree(pop->best->root, 0);

            // Test on 20 episodes
            printf("\nTesting on 20 episodes:\n");
            int successes = 0;
            for (int ep = 0; ep < 20; ep++) {
                TaxiState state;
                taxi_reset(&state);
                Context ctx = {0};

                for (int step = 0; step < MAX_STEPS; step++) {
                    ctx.inputs[0] = state.taxi_row;
                    ctx.inputs[1] = state.taxi_col;
                    ctx.inputs[2] = state.pass_loc;
                    ctx.inputs[3] = state.dest_loc;
                    if (state.pass_loc < 4) {
                        ctx.inputs[4] = LOCS[state.pass_loc][0];
                        ctx.inputs[5] = LOCS[state.pass_loc][1];
                    } else {
                        ctx.inputs[4] = -1;
                        ctx.inputs[5] = -1;
                    }
                    ctx.inputs[6] = LOCS[state.dest_loc][0];
                    ctx.inputs[7] = LOCS[state.dest_loc][1];
                    ctx.num_inputs = 8;
                    ctx.num_outputs = 0;

                    execute_program(pop->best, &ctx, NULL);

                    int action = 0;
                    if (ctx.num_outputs > 0) {
                        action = abs(ctx.outputs[0]) % 6;
                    }

                    int reward;
                    int done = taxi_step(&state, action, &reward);

                    if (done) {
                        successes++;
                        printf("  Episode %2d: SUCCESS in %d steps\n", ep + 1, step + 1);
                        break;
                    }

                    if (step == MAX_STEPS - 1) {
                        printf("  Episode %2d: FAILED\n", ep + 1);
                    }
                }
            }
            printf("\nSuccess rate: %d/20 = %.0f%%\n", successes, successes * 5.0f);
            break;
        }

        if (no_improvement > 2000) {
            printf("\nNo improvement for 2000 generations, stopping.\n");
            break;
        }
    }

    if (pop->best_fitness < 7.0f) {
        printf("\nDid not solve (best: %.1f)\n", pop->best_fitness);
        printf("\nBest solution:\n");
        print_tree(pop->best->root, 0);

        printf("\nLibrary (%d entries):\n", pop->library_size);
        for (int i = 0; i < pop->library_size; i++) {
            printf("\n[%d] %s (uses=%d, size=%d):\n",
                   i, pop->library[i].name, pop->library[i].uses,
                   node_size(pop->library[i].tree));
            print_tree(pop->library[i].tree, 1);
        }
    }

    pop_destroy(pop);
    return 0;
}
