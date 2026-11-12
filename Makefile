CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -pedantic -pthread

matrix: matrix.c
	$(CC) $(CFLAGS) -o matrix matrix.c

clean:
	rm -f matrix
