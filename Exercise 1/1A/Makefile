# Author: Maximilian Kleinegger (12041500)
# Created: 2022-10-06
# Programs: mydiff

CC      = gcc
DEFS    = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS  = -std=c99 -pedantic -Wall -g $(DEFS)

OBJECTS = mydiff.o

.PHONY: all clean
all: mydiff

mydiff: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o mydiff