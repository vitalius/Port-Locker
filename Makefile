# Vialiy Pavlenko
# CS455
# Project 1
# Fall 2010
CC=gcc
CFLAGS=-c -Wall -pedantic -O2
SOURCES=server.c client.o utils.c protocol.c
OBJECTS=$(SOURCES:.c=.o)
HEADERS=protocol.h common.h
EXEC_SERVER=pssvr
EXEC_CLIENT=psc

all: $(SOURCES) $(HEADERS) $(EXEC_SERVER) $(EXEC_CLIENT)

$(EXEC_SERVER): $(OBJECTS)
	$(CC) server.o utils.o protocol.o -o $@

$(EXEC_CLIENT): $(OBJECTS)
	$(CC) client.o utils.o protocol.o -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXEC_SERVER) $(EXEC_CLIENT)
