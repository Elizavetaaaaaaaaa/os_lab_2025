#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
  // Проверка аргументов командной строки
  if (argc < 3) {
    printf("Usage: %s <PORT> <BUFSIZE>\n", argv[0]);
    exit(1);
  }

  // Парсинг аргументов
  int port = atoi(argv[1]);
  int bufsize = atoi(argv[2]);

  // Валидация аргументов
  if (port <= 0 || bufsize <= 0) {
    printf("Invalid port or buffer size\n");
    exit(1);
  }

  int lfd, cfd;
  int nread;
  char *buf = malloc(bufsize); // Динамический буфер
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  // Создание сокета
  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    free(buf);
    exit(1);
  }

  // Настройка адреса сервера
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  // Привязка сокета
  if (bind(lfd, (SADDR *)&servaddr, sizeof(servaddr)) < 0) {
    perror("bind");
    free(buf);
    close(lfd);
    exit(1);
  }

  // Начало прослушивания
  if (listen(lfd, 5) < 0) {
    perror("listen");
    free(buf);
    close(lfd);
    exit(1);
  }

  printf("TCP Server listening on port %d\n", port);

  // Основной цикл обработки соединений
  while (1) {
    socklen_t clilen = sizeof(cliaddr);

    // Принятие соединения
    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      continue; // Продолжаем работать при ошибке accept
    }
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &cliaddr.sin_addr, client_ip, INET_ADDRSTRLEN);
    printf("Connection established from %s:%d\n", client_ip, ntohs(cliaddr.sin_port));

    // Обработка данных от клиента
    while ((nread = read(cfd, buf, bufsize)) > 0) {
      printf("Received %d bytes: ", nread);
      write(1, buf, nread); // Вывод на stdout
      printf("\n");
    }

    if (nread == -1) {
      perror("read");
    }

    printf("Connection closed\n");
    close(cfd);
  }

  // Очистка ресурсов (никогда не выполнится из-за бесконечного цикла)
  free(buf);
  close(lfd);
  return 0;
}