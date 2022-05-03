CC = g++
CFLAGS = -std=c++1y -g -fopenmp -O3 -Wall

wordleSolver.out: wordleSolver.o
	$(CC) $(CFLAGS) -o wordleSolver.out wordleSolver.o

wordleSolver.o: wordleSolver.cpp
	$(CC) $(CFLAGS) -c wordleSolver.cpp

clean:
	rm *.o *.out
