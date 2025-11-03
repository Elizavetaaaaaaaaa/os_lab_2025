# Компилятор и флаги
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
LDFLAGS = 

# Исходные файлы
TCP_CLIENT_SRC = tcpclient.c
TCP_SERVER_SRC = tcpserver.c
UDP_CLIENT_SRC = udpclient.c
UDP_SERVER_SRC = udpserver.c

# Исполняемые файлы
TCP_CLIENT = tcpclient
TCP_SERVER = tcpserver
UDP_CLIENT = udpclient
UDP_SERVER = udpserver

# Цели по умолчанию
all: $(TCP_CLIENT) $(TCP_SERVER) $(UDP_CLIENT) $(UDP_SERVER)

# TCP клиент
$(TCP_CLIENT): $(TCP_CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(TCP_CLIENT) $(TCP_CLIENT_SRC) $(LDFLAGS)

# TCP сервер
$(TCP_SERVER): $(TCP_SERVER_SRC)
	$(CC) $(CFLAGS) -o $(TCP_SERVER) $(TCP_SERVER_SRC) $(LDFLAGS)

# UDP клиент
$(UDP_CLIENT): $(UDP_CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(UDP_CLIENT) $(UDP_CLIENT_SRC) $(LDFLAGS)

# UDP сервер
$(UDP_SERVER): $(UDP_SERVER_SRC)
	$(CC) $(CFLAGS) -o $(UDP_SERVER) $(UDP_SERVER_SRC) $(LDFLAGS)

# Очистка
clean:
	rm -f $(TCP_CLIENT) $(TCP_SERVER) $(UDP_CLIENT) $(UDP_SERVER)

# Тестирование TCP
test-tcp: $(TCP_CLIENT) $(TCP_SERVER)
	@echo "=== Testing TCP ==="
	@./$(TCP_SERVER) 8080 100 &
	@sleep 2
	@echo "Hello TCP" | ./$(TCP_CLIENT) 127.0.0.1 8080 100
	@-pkill $(TCP_SERVER) 2>/dev/null || true

# Тестирование UDP
test-udp: $(UDP_CLIENT) $(UDP_SERVER)
	@echo "=== Testing UDP ==="
	@./$(UDP_SERVER) 8081 100 &
	@sleep 2
	@echo "Hello UDP" | ./$(UDP_CLIENT) 127.0.0.1 8081 100
	@-pkill $(UDP_SERVER) 2>/dev/null || true

# Тестирование всего
test: test-tcp test-udp

# Справка по использованию
help:
	@echo "Available targets:"
	@echo "  all        - Build all programs"
	@echo "  clean      - Remove compiled programs"
	@echo "  test       - Run all tests"
	@echo "  test-tcp   - Test TCP client/server"
	@echo "  test-udp   - Test UDP client/server"
	@echo ""
	@echo "Usage examples:"
	@echo "  TCP Server: ./$(TCP_SERVER) <port> <bufsize>"
	@echo "  TCP Client: ./$(TCP_CLIENT) <ip> <port> <bufsize>"
	@echo "  UDP Server: ./$(UDP_SERVER) <port> <bufsize>"
	@echo "  UDP Client: ./$(UDP_CLIENT) <ip> <port> <bufsize>"

.PHONY: all clean test test-tcp test-udp help