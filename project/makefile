CC = gcc
FLAGS = -w -o

all:
	$(CC) client.c $(FLAGS) client.o
	$(CC) server.c $(FLAGS) server.o
	$(CC) listener.c $(FLAGS) listener.o

clean: 
	rm -f *.o