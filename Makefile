CC=gcc
CFLAGS=-I.

all: myserver.c
	$(CC) $(CFLAGS) myserver.c -o myserver
clean:
	rm -f *o myserver

