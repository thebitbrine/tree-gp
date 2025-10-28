CC = gcc
CFLAGS = -Wall -O2 -g -pthread
LDFLAGS = -lm -pthread

all: test_add test_cartpole benchmark analyze_solution test_sequence test_maze test_taxi test_adf test_mux test_parity

test_add: test_add.c gp.c gp.h
	$(CC) $(CFLAGS) -o test_add test_add.c gp.c $(LDFLAGS)

test_cartpole: test_cartpole.c gp.c gp.h
	$(CC) $(CFLAGS) -o test_cartpole test_cartpole.c gp.c $(LDFLAGS)

benchmark: benchmark.c gp.c gp.h
	$(CC) $(CFLAGS) -o benchmark benchmark.c gp.c $(LDFLAGS)

analyze_solution: analyze_solution.c gp.c gp.h
	$(CC) $(CFLAGS) -o analyze_solution analyze_solution.c gp.c $(LDFLAGS)

test_sequence: test_sequence.c gp.c gp.h
	$(CC) $(CFLAGS) -o test_sequence test_sequence.c gp.c $(LDFLAGS)

test_maze: test_maze.c gp.c gp.h
	$(CC) $(CFLAGS) -o test_maze test_maze.c gp.c $(LDFLAGS)

test_taxi: test_taxi.c gp.c gp.h
	$(CC) $(CFLAGS) -o test_taxi test_taxi.c gp.c $(LDFLAGS)

test_adf: test_adf.c gp.c gp.h
	$(CC) $(CFLAGS) -o test_adf test_adf.c gp.c $(LDFLAGS)

test_mux: test_mux.c gp.c gp.h
	$(CC) $(CFLAGS) -o test_mux test_mux.c gp.c $(LDFLAGS)

test_parity: test_parity.c gp.c gp.h
	$(CC) $(CFLAGS) -o test_parity test_parity.c gp.c $(LDFLAGS)

clean:
	rm -f test_add test_cartpole benchmark analyze_solution test_sequence test_maze test_taxi test_adf test_mux test_parity *.o

.PHONY: all clean
