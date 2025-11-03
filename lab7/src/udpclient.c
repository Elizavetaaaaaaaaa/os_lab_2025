#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define SADDR struct sockaddr

int main(int argc, char **argv) {
  // Проверка аргументов командной строки
  if (argc != 4) {
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

  int sockfd, n;
  char *sendline = malloc(bufsize);
  char *recvline = malloc(bufsize + 1);
  struct sockaddr_in servaddr;

  // Настройка адреса сервера
  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) < 0) {
    perror("inet_pton problem");
    free(sendline);
    free(recvline);
    exit(1);
  }

  // Создание UDP сокета
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    free(sendline);
    free(recvline);
    exit(1);
  }

  printf("UDP Client ready. Server: %s:%d\n", ip, port);
  printf("Enter strings to send (Ctrl+D to exit):\n");

  // Основной цикл отправки/получения сообщений
  while ((n = read(0, sendline, bufsize)) > 0) {
    // Отправка сообщения серверу
    if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, sizeof(servaddr)) == -1) {
      perror("sendto problem");
      break;
    }

    // Получение ответа от сервера
    socklen_t len = sizeof(servaddr);
    int recv_len = recvfrom(sockfd, recvline, bufsize, 0, (SADDR *)&servaddr, &len);
    if (recv_len == -1) {
      perror("recvfrom problem");
      break;
    }

    recvline[recv_len] = '\0'; // Добавление нуль-терминатора
    printf("REPLY FROM SERVER: %s\n", recvline);
  }

  // Очистка ресурсов
  free(sendline);
  free(recvline);
  close(sockfd);
  printf("Client stopped\n");
  return 0;
}