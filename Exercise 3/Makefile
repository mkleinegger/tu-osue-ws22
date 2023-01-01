# Author: Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
# Created: 2022-10-06
# Programs: mydiff

CC      = gcc
DEFS    = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS  = -std=c99 -pedantic -Wall -g $(DEFS)

OBJECTS = client.o

.PHONY: all clean
all: client

client: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o client