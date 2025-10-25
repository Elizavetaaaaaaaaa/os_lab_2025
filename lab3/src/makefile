# Определение компилятора
CC=gcc
# Флаги компиляции
CFLAGS=-I.

# PHONY targets - указывают, что эти targets не являются файлами
.PHONY: all clean

# Target all - собирает ВСЕ программы проекта
all: sequential_min_max parallel_min_max exec_sequential

# Target для сборки sequential_min_max
sequential_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o sequential_min_max find_min_max.o utils.o sequential_min_max.c $(CFLAGS)

# Target для сборки parallel_min_max  
parallel_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

# Target для сборки exec_sequential 
exec_sequential: exec_sequential.o
	$(CC) -o exec_sequential exec_sequential.o $(CFLAGS)

# Target для компиляции exec_sequential.o
exec_sequential.o: exec_sequential.c
	$(CC) -o exec_sequential.o -c exec_sequential.c $(CFLAGS)

# Target для компиляции utils.o
utils.o: utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

# Target для компиляции find_min_max.o
find_min_max.o: utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

# Target для очистки - удаляет все объектные и исполняемые файлы
clean:
	rm -f utils.o find_min_max.o sequential_min_max parallel_min_max exec_sequential exec_sequential.o child_result.txt


