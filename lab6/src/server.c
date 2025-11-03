// Для параллелизации между серверами я выбрала многопоточность по следующим причинам:
//    Простота реализации - pthreads уже используются в проекте
//    Эффективность - потоки легковесны по сравнению с процессами
//    Совместимость - код легко интегрируется с существующей структурой
//    Производительность - параллельные TCP-соединения к разным серверам

/**
 * Сервер для параллельного вычисления факториала по модулю
 * 
 * Сервер принимает от клиентов диапазоны чисел [begin, end] и модуль mod,
 * вычисляет произведение чисел в диапазоне по модулю (begin * (begin+1) * ... * end) mod mod
 * с использованием многопоточности для ускорения вычислений.
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h> 

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"
#include "common.h"  // Общие структуры и функции

/**
 * Вычисление частичного факториала для диапазона чисел [begin, end] по модулю
 * Вычисляет произведение: begin * (begin+1) * ... * end mod mod
 * 
 * @param args - структура с параметрами вычисления (диапазон и модуль)
 * @return результат вычисления произведения диапазона по модулю
 */
uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;
  // Последовательное перемножение всех чисел в диапазоне с применением модуля
  for (uint64_t i = args->begin; i <= args->end; i++) {
    ans = MultModulo(ans, i, args->mod);
  }
  return ans;
}

/**
 * Функция-обертка для выполнения вычислений в отдельном потоке
 * Создает копию результата в куче для безопасной передачи между потоками
 * 
 * @param args - указатель на структуру FactorialArgs с параметрами вычислений
 * @return указатель на результат вычисления (выделяется в куче)
 */
void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  // Выделение памяти для результата, который будет передан обратно в главный поток
  uint64_t *result = malloc(sizeof(uint64_t));
  *result = Factorial(fargs);
  return (void *)result;
}

/**
 * Основная функция сервера
 * Организует прием соединений, обработку запросов и параллельные вычисления
 */
int main(int argc, char **argv) {
  int tnum = -1;  // Количество потоков для вычислений (инициализация невалидным значением)
  int port = -1;   // Порт для прослушивания (инициализация невалидным значением)

  // ПАРСИНГ АРГУМЕНТОВ КОМАНДНОЙ СТРОКИ
  
  // Обработка опций командной строки с использованием getopt_long
  while (true) {
    // Определение доступных длинных опций
    static struct option options[] = {
      {"port", required_argument, 0, 0},  // Порт сервера
      {"tnum", required_argument, 0, 0},  // Количество потоков
      {0, 0, 0, 0}                        // Конец списка опций
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)  // Все опции обработаны
      break;

    switch (c) {
    case 0: {  // Обработка длинных опций (--key value)
      switch (option_index) {
      case 0:  // --port
        port = atoi(optarg);
        // Проверка корректности номера порта
        if (port <= 0) {
          fprintf(stderr, "Port must be positive number\n");
          return 1;
        }
        break;
      case 1:  // --tnum
        tnum = atoi(optarg);
        // Проверка корректности количества потоков
        if (tnum <= 0) {
          fprintf(stderr, "Thread count must be positive number\n");
          return 1;
        }
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':  // Неизвестная опция
      printf("Unknown argument\n");
      break;
    default:   // Неожиданная ошибка getopt
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  // ПРОВЕРКА ОБЯЗАТЕЛЬНЫХ ПАРАМЕТРОВ
  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  // СОЗДАНИЕ И НАСТРОЙКА СЕРВЕРНОГО СОКЕТА
  
  // Создание TCP сокета для IPv4
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket!");
    return 1;
  }

  // Настройка адреса сервера
  struct sockaddr_in server;
  server.sin_family = AF_INET;           // Семейство адресов IPv4
  server.sin_port = htons((uint16_t)port); // Порт в сетевом порядке байт
  server.sin_addr.s_addr = htonl(INADDR_ANY); // Принимать соединения на все интерфейсы

  // Разрешение повторного использования адреса (для быстрого перезапуска сервера)
  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  // Привязка сокета к адресу и порту
  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    fprintf(stderr, "Can not bind to socket!");
    return 1;
  }

  // Начало прослушивания входящих соединений
  // 128 - максимальная длина очереди ожидающих соединений
  err = listen(server_fd, 128);
  if (err < 0) {
    fprintf(stderr, "Could not listen on socket\n");
    return 1;
  }

  printf("Server listening at %d\n", port);

  // ОСНОВНОЙ ЦИКЛ ОБРАБОТКИ СОЕДИНЕНИЙ
  
  // Бесконечный цикл для непрерывной работы сервера
  while (true) {
    // Принятие входящего соединения от клиента
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

    if (client_fd < 0) {
      fprintf(stderr, "Could not establish new connection\n");
      continue; // Продолжаем принимать соединения несмотря на ошибку
    }

    // ЦИКЛ ОБРАБОТКИ ЗАПРОСОВ ОТ КОНКРЕТНОГО КЛИЕНТА
    
    // Обработка нескольких запросов в рамках одного соединения
    while (true) {
      // Размер буфера для приема данных: 3 числа uint64_t (begin, end, mod)
      size_t buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      
      // Прием данных от клиента
      ssize_t read = recv(client_fd, from_client, buffer_size, 0);

      if (!read)  // Клиент закрыл соединение
        break;
      if (read < 0) {  // Ошибка при чтении
        fprintf(stderr, "Client read failed\n");
        break;
      }
      // Проверка полноты полученных данных
      if ((size_t)read < buffer_size) {
        fprintf(stderr, "Client send wrong data format\n");
        break;
      }

      // ПОДГОТОВКА К ПАРАЛЛЕЛЬНЫМ ВЫЧИСЛЕНИЯМ
      
      pthread_t threads[tnum];  // Массив идентификаторов потоков

      // Извлечение параметров из полученных бинарных данных
      uint64_t begin = 0;
      uint64_t end = 0;
      uint64_t mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      // Логирование полученных параметров (PRIu64 - правильный формат для uint64_t)
      fprintf(stdout, "Receive: %" PRIu64 " %" PRIu64 " %" PRIu64 "\n", begin, end, mod);

      struct FactorialArgs args[tnum];  // Аргументы для каждого потока
      
      // РАСПРЕДЕЛЕНИЕ РАБОТЫ МЕЖДУ ПОТОКАМИ
      
      // Вычисление общего количества чисел в диапазоне
      uint64_t numbers_count = end - begin + 1;
      // Базовое количество чисел на поток
      uint64_t numbers_per_thread = numbers_count / tnum;
      // Остаток чисел для распределения по первым потокам
      uint64_t remainder = numbers_count % tnum;
      uint64_t current_begin = begin;  // Текущее начало диапазона

      // Создание потоков для параллельных вычислений
      for (int i = 0; i < tnum; i++) {
        // Определение количества чисел для текущего потока
        uint64_t numbers_for_this_thread = numbers_per_thread;
        if ((uint64_t)i < remainder) {  // Распределение остатка
          numbers_for_this_thread++;
        }

        // Настройка параметров для текущего потока
        args[i].begin = current_begin;
        args[i].end = current_begin + numbers_for_this_thread - 1;
        args[i].mod = mod;
        current_begin += numbers_for_this_thread;  // Сдвиг для следующего потока

        // Логирование распределения работы
        printf("Thread %d: numbers from %" PRIu64 " to %" PRIu64 "\n", 
               i, args[i].begin, args[i].end);

        // Создание потока для вычисления части факториала
        if (pthread_create(&threads[i], NULL, ThreadFactorial, (void *)&args[i])) {
          printf("Error: pthread_create failed!\n");
          return 1;
        }
      }

      // СБОР РЕЗУЛЬТАТОВ И ВЫЧИСЛЕНИЕ ОКОНЧАТЕЛЬНОГО РЕЗУЛЬТАТА
      
      uint64_t total = 1;  // Нейтральный элемент для умножения
      
      // Ожидание завершения всех потоков и сбор результатов
      for (int i = 0; i < tnum; i++) {
        uint64_t *result = NULL;
        // Ожидание завершения потока и получение результата
        pthread_join(threads[i], (void **)&result);
        if (result != NULL) {
          // Умножение текущего результата на результат потока по модулю
          total = MultModulo(total, *result, mod);
          free(result);  // Освобождение памяти, выделенной в потоке
        }
      }

      // Логирование окончательного результата
      printf("Total: %" PRIu64 "\n", total);

      // ОТПРАВКА РЕЗУЛЬТАТА КЛИЕНТУ
      
      char buffer[sizeof(total)];
      memcpy(buffer, &total, sizeof(total));  // Упаковка результата в бинарный формат
      err = send(client_fd, buffer, sizeof(total), 0);
      if (err < 0) {
        fprintf(stderr, "Can't send data to client\n");
        break;
      }
    }

    // ЗАВЕРШЕНИЕ РАБОТЫ С КЛИЕНТОМ
    
    shutdown(client_fd, SHUT_RDWR);  // Отключение передачи в обоих направлениях
    close(client_fd);  // Закрытие файлового дескриптора клиента
  }

  // Закрытие серверного сокета (эта строка никогда не выполнится в бесконечном цикле)
  close(server_fd);
  return 0;
}