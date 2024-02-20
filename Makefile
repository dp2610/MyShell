CC=gcc
CFLAGS=-Wall -Werror

.PHONY: all clean

all: mysh

mysh: my_shell.o
	$(CC) $(CFLAGS) -o mysh my_shell.o

my_shell.o: my_shell.c
	$(CC) $(CFLAGS) -c my_shell.c

clean:
	rm -f *.o mysh