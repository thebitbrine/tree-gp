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

void analyze_controller(Program* prog, int num_trials) {
    printf("\nController Analysis (%d trials)\n", num_trials);
    printf("================================\n\n");

    printf("Solution tree:\n");
    print_tree(prog->root, 0);
    printf("\n");

    int total_successes = 0;
    int total_steps = 0;

    for (int trial = 0; trial < num_trials; trial++) {
        CartPoleState state;
        cartpole_reset(&state);

        int steps = 0;
        for (; steps < 500; steps++) {
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
        }

        total_steps += steps;
        if (steps == 500) total_successes++;
    }

    printf("Success rate: %d/%d = %.1f%%\n",
           total_successes, num_trials,
           100.0 * total_successes / num_trials);
    printf("Average steps: %.1f\n\n", (float)total_steps / num_trials);

    // Show some example state transitions
    printf("Example state transitions:\n");
    printf("%-8s %-8s %-8s %-8s | %-8s | %-6s\n",
           "x", "x_dot", "theta", "th_dot", "output", "action");
    printf("---------------------------------------------------------------\n");

    CartPoleState state;
    cartpole_reset(&state);

    for (int i = 0; i < 10; i++) {
        Context ctx = {0};
        ctx.inputs[0] = (int)(state.x * 100);
        ctx.inputs[1] = (int)(state.x_dot * 100);
        ctx.inputs[2] = (int)(state.theta * 100);
        ctx.inputs[3] = (int)(state.theta_dot * 100);
        ctx.num_inputs = 4;

        execute_program(prog, &ctx, NULL);

        int output = (ctx.num_outputs > 0) ? ctx.outputs[0] : 0;
        int action = output > 0 ? 1 : 0;

        printf("%8.3f %8.3f %8.3f %8.3f | %8d | %s\n",
               state.x, state.x_dot, state.theta, state.theta_dot,
               output, action ? "RIGHT" : "LEFT");

        cartpole_step(&state, action);
        if (cartpole_is_done(&state)) break;
    }
}

int main() {
    printf("CartPole Solution Analysis\n");
    printf("==========================\n\n");

    // Create a simple PD controller manually
    printf("Testing classic PD controller: theta + theta_dot\n");

    // Build: OUTPUT(ADD(INPUT[theta], INPUT[theta_dot]))
    Node* theta = node_create(OP_INPUT, 2);  // theta is input[2]
    Node* theta_dot = node_create(OP_INPUT, 3);  // theta_dot is input[3]
    Node* add = node_create(OP_ADD, 0);
    add->children[0] = theta;
    add->children[1] = theta_dot;
    add->num_children = 2;

    Node* output = node_create(OP_OUTPUT, 0);
    output->children[0] = add;
    output->num_children = 1;

    Program* pd_controller = malloc(sizeof(Program));
    pd_controller->root = output;
    pd_controller->fitness = 0;
    prog_update_metadata(pd_controller);

    analyze_controller(pd_controller, 100);

    // Test variations
    printf("\n\nTesting with position feedback: (theta + theta_dot) + x\n");

    Node* x = node_create(OP_INPUT, 0);  // x is input[0]
    Node* theta2 = node_create(OP_INPUT, 2);
    Node* theta_dot2 = node_create(OP_INPUT, 3);
    Node* add1 = node_create(OP_ADD, 0);
    add1->children[0] = theta2;
    add1->children[1] = theta_dot2;
    add1->num_children = 2;

    Node* add2 = node_create(OP_ADD, 0);
    add2->children[0] = add1;
    add2->children[1] = x;
    add2->num_children = 2;

    Node* output2 = node_create(OP_OUTPUT, 0);
    output2->children[0] = add2;
    output2->num_children = 1;

    Program* pdx_controller = malloc(sizeof(Program));
    pdx_controller->root = output2;
    pdx_controller->fitness = 0;
    prog_update_metadata(pdx_controller);

    analyze_controller(pdx_controller, 100);

    prog_destroy(pd_controller);
    prog_destroy(pdx_controller);

    return 0;
}
