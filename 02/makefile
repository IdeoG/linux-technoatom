CC=gcc
all:
	$(CC) fork.c -w -o out_fork.o
	$(CC) system.c -w -o out_system.o
	$(CC) semaphore.c -w -o sem.o
fork: 
	./out_fork.o

system:
	./out_system.o

sem:
	./sem.o

clean:
	rm *.o