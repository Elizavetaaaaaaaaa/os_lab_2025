CC=gcc
CFLAGS=-I.
PTHREAD_FLAGS=-pthread


all: parallel_min_max process_memory parallel_sum


parallel_min_max: utils.o find_min_max.o parallel_min_max.c
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)


process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

parallel_sum: utils.o sum_utils.o parallel_sum.c
	$(CC) -o parallel_sum utils.o sum_utils.o parallel_sum.c $(CFLAGS) $(PTHREAD_FLAGS)

utils.o: utils.c utils.h
	$(CC) -c utils.c $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h
	$(CC) -c find_min_max.c $(CFLAGS)

sum_utils.o: sum_utils.c sum_utils.h
	$(CC) -c sum_utils.c $(CFLAGS) $(PTHREAD_FLAGS)


clean:
	rm -f *.o parallel_min_max process_memory parallel_sum child_result.txt

.PHONY: all clean