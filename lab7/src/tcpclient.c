#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  // Проверка аргументов командной строки
  if (argc < 4) {
    printf("Usage: %s <IP> <PORT> <BUFSIZE>\n", argv[0]);
    exit(1);
  }

  // Парсинг аргументов
  char *ip = argv[1];
  int port = atoi(argv[2]);
  int bufsize = atoi(argv[3]);

  // Валидация аргументов
  if (port <= 0 || bufsize <= 0) {
    printf("Invalid port or buffer size\n");
    exit(1);
  }

  int fd;
  int nread;
  char *buf = malloc(bufsize); // Динамический буфер
  struct sockaddr_in servaddr;
  
  // Создание сокета
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    free(buf);
    exit(1);
  }

  // Настройка адреса сервера
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
    perror("bad address");
    free(buf);
    close(fd);
    exit(1);
  }

  servaddr.sin_port = htons(port);

  // Подключение к серверу
  if (connect(fd, (SADDR *)&servaddr, sizeof(servaddr)) < 0) {
    perror("connect");
    free(buf);
    close(fd);
    exit(1);
  }

  printf("Connected to server %s:%d\n", ip, port);
  printf("Input message to send (Ctrl+D to exit):\n");
  
  // Чтение и отправка сообщений
  while ((nread = read(0, buf, bufsize)) > 0) {
    if (write(fd, buf, nread) < 0) {
      perror("write");
      break;
    }
  }

  // Очистка ресурсов
  free(buf);
  close(fd);
  printf("Connection closed\n");
  return 0;
}