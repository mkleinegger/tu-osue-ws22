# Author: Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
# Created: 2022-12-05
# Programs: forksort

CC      = gcc
DEFS    = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS  = -std=c99 -pedantic -Wall -g $(DEFS)

.PHONY: all clean
all: forksort

forksort: forksort.o
	$(CC) -o $@ $^

forksort.o: forksort.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o forksort