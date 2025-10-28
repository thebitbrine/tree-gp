#include "gp.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

// Simple 5x5 maze navigation
#define MAZE_SIZE 5
#define MAX_STEPS 50

// Actions
#define ACTION_NORTH 0
#define ACTION_SOUTH 1
#define ACTION_EAST 2
#define ACTION_WEST 3

typedef struct {
    int x, y;
    int goal_x, goal_y;
    char maze[MAZE_SIZE][MAZE_SIZE];  // 0=empty, 1=wall
} MazeState;

void maze_init(MazeState* state) {
    // Clear maze
    memset(state->maze, 0, sizeof(state->maze));

    // Add some walls to make it interesting
    state->maze[1][2] = 1;
    state->maze[2][2] = 1;
    state->maze[3][2] = 1;

    // Start and goal
    state->x = 0;
    state->y = 0;
    state->goal_x = 4;
    state->goal_y = 4;
}

int maze_step(MazeState* state, int action) {
    int new_x = state->x;
    int new_y = state->y;

    switch(action) {
        case ACTION_NORTH: new_y--; break;
        case ACTION_SOUTH: new_y++; break;
        case ACTION_EAST:  new_x++; break;
        case ACTION_WEST:  new_x--; break;
    }

    // Check bounds
    if (new_x < 0 || new_x >= MAZE_SIZE || new_y < 0 || new_y >= MAZE_SIZE) {
        return -1;  // Hit boundary
    }

    // Check wall
    if (state->maze[new_y][new_x]) {
        return -1;  // Hit wall
    }

    // Valid move
    state->x = new_x;
    state->y = new_y;

    // Check if reached goal
    if (state->x == state->goal_x && state->y == state->goal_y) {
        return 10;  // Goal reached!
    }

    return 0;  // Valid move, no goal
}

int maze_distance_to_goal(MazeState* state) {
    return abs(state->x - state->goal_x) + abs(state->y - state->goal_y);
}

void maze_print(MazeState* state) {
    for (int y = 0; y < MAZE_SIZE; y++) {
        for (int x = 0; x < MAZE_SIZE; x++) {
            if (x == state->x && y == state->y) {
                printf("A");  // Agent
            } else if (x == state->goal_x && y == state->goal_y) {
                printf("G");  // Goal
            } else if (state->maze[y][x]) {
                printf("#");  // Wall
            } else {
                printf(".");
            }
        }
        printf("\n");
    }
}

float evaluate_maze(Program* prog, void* data) {
    (void)data;

    int num_episodes = 10;
    float total_reward = 0;
    int total_successes = 0;

    for (int ep = 0; ep < num_episodes; ep++) {
        MazeState state;
        maze_init(&state);

        Context ctx = {0};  // Includes memory

        int initial_dist = maze_distance_to_goal(&state);
        int min_dist = initial_dist;

        for (int step = 0; step < MAX_STEPS; step++) {
            // Set inputs: current position and goal position
            ctx.inputs[0] = state.x;
            ctx.inputs[1] = state.y;
            ctx.inputs[2] = state.goal_x;
            ctx.inputs[3] = state.goal_y;
            ctx.num_inputs = 4;
            ctx.num_outputs = 0;

            execute_program(prog, &ctx, NULL);

            // Get action from output (mod 4 for 4 actions)
            int action = 0;
            if (ctx.num_outputs > 0) {
                action = abs(ctx.outputs[0]) % 4;
            }

            int reward = maze_step(&state, action);

            if (reward == 10) {
                // Reached goal!
                total_reward += 100;
                total_reward += (MAX_STEPS - step);  // Bonus for speed
                total_successes++;
                break;
            } else if (reward == -1) {
                // Hit wall or boundary - small penalty
                total_reward -= 1;
            }

            // Track if getting closer
            int dist = maze_distance_to_goal(&state);
            if (dist < min_dist) {
                min_dist = dist;
                total_reward += 1;  // Reward for progress
            }
        }

        // Reward based on how close we got
        total_reward += (initial_dist - min_dist) * 5;
    }

    float fitness = total_reward / num_episodes;

    // Parsimony pressure
    fitness -= prog->size * 0.05f;

    return fitness;
}

int main() {
    srand(time(NULL));

    printf("Tree-based GP - Maze Navigation\n");
    printf("================================\n\n");

    printf("Maze layout:\n");
    MazeState example;
    maze_init(&example);
    maze_print(&example);

    printf("\nTask: Navigate from A to G\n");
    printf("Inputs: [x, y, goal_x, goal_y]\n");
    printf("Output: action (0=N, 1=S, 2=E, 3=W)\n");
    printf("Population: %d\n\n", POP_SIZE);

    Population* pop = pop_create();

    int max_gen = 3000;
    float best_ever = -INFINITY;
    int no_improvement = 0;

    for (int gen = 0; gen < max_gen; gen++) {
        evolve_generation(pop, evaluate_maze, NULL, 4);

        if (pop->best_fitness > best_ever) {
            best_ever = pop->best_fitness;
            no_improvement = 0;
        } else {
            no_improvement++;
        }

        if (gen % 50 == 0 || pop->best_fitness >= 145.0f) {
            printf("Gen %4d: Best=%.1f Avg=%.1f Size=%d Depth=%d\n",
                   gen,
                   pop->best_fitness,
                   pop->avg_fitness,
                   pop->best ? pop->best->size : 0,
                   pop->best ? pop->best->depth : 0);
        }

        if (pop->best_fitness >= 145.0f) {
            printf("\n*** TASK SOLVED! ***\n");
            printf("Final fitness: %.1f\n", pop->best_fitness);
            printf("Solution size: %d nodes\n", pop->best->size);
            printf("Solution depth: %d\n", pop->best->depth);
            printf("\nSolution tree:\n");
            print_tree(pop->best->root, 0);

            // Visualize a run
            printf("\nExample run:\n");
            MazeState state;
            maze_init(&state);
            Context ctx = {0};

            for (int step = 0; step < MAX_STEPS; step++) {
                printf("\nStep %d:\n", step);
                maze_print(&state);

                ctx.inputs[0] = state.x;
                ctx.inputs[1] = state.y;
                ctx.inputs[2] = state.goal_x;
                ctx.inputs[3] = state.goal_y;
                ctx.num_inputs = 4;
                ctx.num_outputs = 0;

                execute_program(pop->best, &ctx, NULL);

                int action = 0;
                if (ctx.num_outputs > 0) {
                    action = abs(ctx.outputs[0]) % 4;
                }

                const char* action_names[] = {"NORTH", "SOUTH", "EAST", "WEST"};
                printf("Action: %s (output=%d)\n",
                       action_names[action],
                       ctx.num_outputs > 0 ? ctx.outputs[0] : 0);

                int reward = maze_step(&state, action);
                if (reward == 10) {
                    printf("\nGoal reached in %d steps!\n", step + 1);
                    maze_print(&state);
                    break;
                } else if (reward == -1) {
                    printf("Hit wall/boundary!\n");
                }
            }

            break;
        }

        if (no_improvement > 500) {
            printf("\nNo improvement for 500 generations, stopping.\n");
            break;
        }
    }

    if (pop->best_fitness < 145.0f) {
        printf("\nDid not fully solve (best: %.1f)\n", pop->best_fitness);
        printf("Best solution:\n");
        print_tree(pop->best->root, 0);
    }

    pop_destroy(pop);
    return 0;
}
