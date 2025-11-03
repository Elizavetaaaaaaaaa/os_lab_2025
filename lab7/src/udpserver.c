#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

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

  int sockfd, n;
  char *mesg = malloc(bufsize);
  char ipadr[INET_ADDRSTRLEN];
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  // Создание UDP сокета
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    free(mesg);
    exit(1);
  }

  // Настройка адреса сервера
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  // Привязка сокета
  if (bind(sockfd, (SADDR *)&servaddr, sizeof(servaddr)) < 0) {
    perror("bind problem");
    free(mesg);
    close(sockfd);
    exit(1);
  }

  printf("UDP SERVER starts on port %d...\n", port);

  // Основной цикл обработки запросов
  while (1) {
    socklen_t len = sizeof(cliaddr);

    // Получение сообщения от клиента
    if ((n = recvfrom(sockfd, mesg, bufsize, 0, (SADDR *)&cliaddr, &len)) < 0) {
      perror("recvfrom");
      continue; // Продолжаем работать при ошибке
    }
    
    mesg[n] = 0; // Добавление нуль-терминатора

    // Вывод информации о запросе
    printf("REQUEST: '%s' FROM %s:%d\n", 
           mesg,
           inet_ntop(AF_INET, &cliaddr.sin_addr, ipadr, sizeof(ipadr)),
           ntohs(cliaddr.sin_port));

    // Отправка эхо-ответа
    if (sendto(sockfd, mesg, n, 0, (SADDR *)&cliaddr, len) < 0) {
      perror("sendto");
      continue; // Продолжаем работать при ошибке
    }
  }

  // Очистка ресурсов (никогда не выполнится из-за бесконечного цикла)
  free(mesg);
  close(sockfd);
  return 0;
}