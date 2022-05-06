CC = mpic++
CFLAGS = -I. -O3

wordleSolver.out: wordleSolver.o
	$(CC) $(CFLAGS) -o wordleSolver.out wordleSolver.o

wordleSolver.o: wordleSolver.cpp
	$(CC) $(CFLAGS) -c wordleSolver.cpp

clean:
	rm *.o *.out
