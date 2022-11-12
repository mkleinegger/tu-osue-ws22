# Author: Maximilian Kleinegger <e12041500@student.tuwien.ac.at>
# Created: 2022-11-01
# Programs: supervisor generator

CC      = gcc
DEFS    = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS  = -std=c99 -pedantic -Wall -g $(DEFS)
LDFLAGS = -pthread -lrt 

GENERATOR_OBJECTS = generator.o sharedMemory.o circleBuffer.o
SUPERVISOR_OBJECTS = supervisor.o sharedMemory.o circleBuffer.o

.PHONY: all clean
all: generator supervisor

generator: $(GENERATOR_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

supervisor: $(SUPERVISOR_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o generator supervisor