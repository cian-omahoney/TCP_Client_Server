CC = gcc
CFLAGS = -g -Wall

all:	master worker

master:	master.o MW_shared.o
	$(CC) master.o MW_shared.o -o master -lpthread

master.o: master.c MW_header.h
	$(CC) -c $(CFLAGS) master.c

worker: worker.o MW_shared.o
	$(CC) worker.o MW_shared.o -o worker -lpthread

worker.o: worker.c MW_header.h
	$(CC) -c $(CFLAGS) worker.c 

MW_shared.o: MW_shared.c MW_header.h
	$(CC) -c $(CFLAGS) MW_shared.c

clean:
	rm -rf *.o master worker
