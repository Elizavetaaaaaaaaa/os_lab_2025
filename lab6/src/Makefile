# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
LDFLAGS = -lpthread

# Цели
all: client server

# Общая библиотека
common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

# Клиент
client: client.o common.o
	$(CC) $(CFLAGS) -o client client.o common.o $(LDFLAGS)

client.o: client.c common.h
	$(CC) $(CFLAGS) -c client.c

# Сервер
server: server.o common.o
	$(CC) $(CFLAGS) -o server server.o common.o $(LDFLAGS)

server.o: server.c common.h
	$(CC) $(CFLAGS) -c server.c

# Очистка
clean:
	rm -f *.o client server servers.txt

# Создание тестового файла servers.txt
servers.txt:
	echo "127.0.0.1:20001" > servers.txt

# Тестирование
test: all servers.txt
	@echo "=== Тестирование ==="
	@./server --port 20001 --tnum 2 &
	@sleep 2
	@echo "Запуск клиента..."
	@./client --k 10 --mod 1000 --servers servers.txt || true
	@-pkill server 2>/dev/null || true

# Создание архива для передачи
dist: clean
	mkdir -p factorial_project
	cp client.c server.c common.c common.h Makefile README.md factorial_project/
	tar -czf factorial_project.tar.gz factorial_project/
	rm -rf factorial_project/

.PHONY: all clean test dist