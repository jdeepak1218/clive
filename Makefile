CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99

clive: clive.c
	$(CC) $(CFLAGS) -o clive clive.c

clean:
	rm -f clive

run: clive
	./clive test.txt

.PHONY: clean run