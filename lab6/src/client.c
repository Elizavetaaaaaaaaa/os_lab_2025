/**
 * client.c - Клиент для распределенного вычисления факториала по модулю
 * Использование: ./client --k 1000 --mod 5 --servers servers.txt
 * 
 * Клиент распределяет вычисление факториала k! mod mod между несколькими серверами,
 * используя многопоточность для параллельного взаимодействия с серверами.
 * 
 * Архитектура:
 * 1. Парсинг аргументов командной строки
 * 2. Чтение конфигурации серверов из файла
 * 3. Распределение диапазонов чисел между серверами
 * 4. Параллельное взаимодействие с серверами через потоки
 * 5. Объединение результатов и вывод итога
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>  // Для правильного форматирования uint64_t (PRIu64)

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>

#include "common.h"  // Общие структуры и функции

/**
 * Структура для передачи аргументов в поток обработки сервера
 * Содержит всю необходимую информацию для взаимодействия с одним сервером
 */
struct ThreadArgs {
  struct Server server;    // Информация о сервере (IP и порт)
  uint64_t begin;          // Начало диапазона чисел для вычислений
  uint64_t end;            // Конец диапазона чисел для вычислений
  uint64_t mod;            // Модуль для вычислений
  uint64_t* result;        // Указатель для сохранения результата от сервера
};

/**
 * Функция, выполняемая в отдельном потоке для взаимодействия с одним сервером
 * Устанавливает TCP-соединение, отправляет задачу, получает и сохраняет результат
 * 
 * @param args - указатель на структуру ThreadArgs с параметрами для сервера
 * @return NULL (результат сохраняется через указатель в структуре)
 */
void* ProcessServer(void* args) {
  struct ThreadArgs* thread_args = (struct ThreadArgs*)args;
  
  // Получение информации о хосте по IP-адресу или доменному имени
  struct hostent *hostname = gethostbyname(thread_args->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", thread_args->server.ip);
    *(thread_args->result) = 0;  // В случае ошибки возвращаем 0
    return NULL;
  }

  // Настройка адреса сервера для подключения
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;  // Используем IPv4
  server_addr.sin_port = htons(thread_args->server.port);  // Порт в сетевом порядке байт
  
  // Копирование IP-адреса из структуры hostent
  if (hostname->h_addr_list[0] != NULL) {
    memcpy(&server_addr.sin_addr.s_addr, 
           hostname->h_addr_list[0], 
           hostname->h_length);
  } else {
    fprintf(stderr, "No address found for %s\n", thread_args->server.ip);
    *(thread_args->result) = 0;
    return NULL;
  }

  // СОЗДАНИЕ И НАСТРОЙКА КЛИЕНТСКОГО СОКЕТА
  
  // Создание TCP сокета
  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    *(thread_args->result) = 0;
    return NULL;
  }

  // Установка соединения с сервером
  if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "Connection to %s:%d failed\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sck);
    *(thread_args->result) = 0;
    return NULL;
  }

  // ПОДГОТОВКА И ОТПРАВКА ЗАДАЧИ СЕРВЕРУ
  
  // Формирование бинарного сообщения: 3 числа uint64_t (begin, end, mod)
  char task[sizeof(uint64_t) * 3];
  memcpy(task, &thread_args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &thread_args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &thread_args->mod, sizeof(uint64_t));

  // Отправка задачи серверу
  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed to %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sck);
    *(thread_args->result) = 0;
    return NULL;
  }

  // ПОЛУЧЕНИЕ ОТВЕТА ОТ СЕРВЕРА
  
  char response[sizeof(uint64_t)];  // Буфер для ответа (одно число uint64_t)
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed from %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sck);
    *(thread_args->result) = 0;
    return NULL;
  }

  // Извлечение результата из бинарного ответа
  uint64_t answer = 0;
  memcpy(&answer, response, sizeof(uint64_t));
  *(thread_args->result) = answer;  // Сохранение результата

  // Вывод информации о полученном результате
  // PRIu64 - правильный спецификатор формата для uint64_t
  printf("Server %s:%d returned: %" PRIu64 " for range [%" PRIu64 ", %" PRIu64 "]\n", 
         thread_args->server.ip, thread_args->server.port, answer,
         thread_args->begin, thread_args->end);

  close(sck);  // Закрытие соединения
  return NULL;
}

/**
 * Основная функция клиента
 * Организует весь процесс вычисления: от парсинга аргументов до вывода результата
 */
int main(int argc, char **argv) {
  uint64_t k = 0;          // Число для вычисления факториала (k!)
  uint64_t mod = 0;        // Модуль для вычислений
  char servers_file[255] = {'\0'};  // Путь к файлу со списком серверов

  // ПАРСИНГ АРГУМЕНТОВ КОМАНДНОЙ СТРОКИ
  
  // Используем getopt_long для обработки длинных опций (--key value)
  while (true) {
    // Определение доступных опций командной строки
    static struct option options[] = {
      {"k", required_argument, 0, 0},        // Число для факториала
      {"mod", required_argument, 0, 0},      // Модуль
      {"servers", required_argument, 0, 0},  // Файл с серверами
      {0, 0, 0, 0}                           // Конец списка
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)  // Все опции обработаны
      break;

    switch (c) {
    case 0: {  // Обработка длинных опций
      switch (option_index) {
      case 0:  // --k
        if (!ConvertStringToUI64(optarg, &k)) {
          fprintf(stderr, "Invalid k value: %s\n", optarg);
          return 1;
        }
        break;
      case 1:  // --mod
        if (!ConvertStringToUI64(optarg, &mod)) {
          fprintf(stderr, "Invalid mod value: %s\n", optarg);
          return 1;
        }
        break;
      case 2:  // --servers
        // Безопасное копирование пути к файлу
        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
        servers_file[sizeof(servers_file) - 1] = '\0';  // Гарантия нуль-терминации
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':  // Неизвестная опция
      printf("Arguments error\n");
      break;
    default:   // Неожиданная ошибка
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  // ПРОВЕРКА ОБЯЗАТЕЛЬНЫХ ПАРАМЕТРОВ
  // Используем 0 вместо -1, так как uint64_t всегда неотрицательный
  if (k == 0 || mod == 0 || !strlen(servers_file)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // ЧТЕНИЕ КОНФИГУРАЦИИ СЕРВЕРОВ ИЗ ФАЙЛА
  
  FILE* file = fopen(servers_file, "r");
  if (file == NULL) {
    fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
    return 1;
  }

  struct Server* servers = NULL;     // Динамический массив серверов
  unsigned int servers_num = 0;      // Количество найденных серверов
  char line[255];                    // Буфер для чтения строк

  // Чтение файла построчно
  while (fgets(line, sizeof(line), file)) {
    line[strcspn(line, "\n")] = 0;  // Удаление символа новой строки
    
    if (strlen(line) == 0)  // Пропуск пустых строк
      continue;

    // Разбор строки формата "IP:port"
    char* colon = strchr(line, ':');
    if (colon == NULL) {
      fprintf(stderr, "Invalid server format in line: %s\n", line);
      continue;
    }

    *colon = '\0';  // Разделение строки на IP и порт
    int port = atoi(colon + 1);  // Преобразование порта в число

    // Проверка корректности номера порта
    if (port <= 0) {
      fprintf(stderr, "Invalid port in line: %s\n", line);
      continue;
    }

    // Добавление сервера в динамический массив
    servers = realloc(servers, (servers_num + 1) * sizeof(struct Server));
    // Безопасное копирование IP-адреса
    strncpy(servers[servers_num].ip, line, sizeof(servers[servers_num].ip) - 1);
    servers[servers_num].ip[sizeof(servers[servers_num].ip) - 1] = '\0';
    servers[servers_num].port = port;
    servers_num++;  // Увеличение счетчика серверов
  }
  fclose(file);

  // Проверка наличия хотя бы одного валидного сервера
  if (servers_num == 0) {
    fprintf(stderr, "No valid servers found in file: %s\n", servers_file);
    free(servers);
    return 1;
  }

  printf("Found %u servers\n", servers_num);

  // РАСПРЕДЕЛЕНИЕ ВЫЧИСЛЕНИЙ МЕЖДУ СЕРВЕРАМИ
  
  pthread_t threads[servers_num];           // Массив идентификаторов потоков
  struct ThreadArgs thread_args[servers_num]; // Аргументы для каждого потока
  uint64_t results[servers_num];            // Результаты от каждого сервера

  // Вычисление распределения чисел между серверами
  uint64_t numbers_per_server = k / servers_num;  // Базовое количество на сервер
  uint64_t remainder = k % servers_num;           // Остаток для равномерного распределения
  uint64_t current_begin = 1;                     // Начало первого диапазона

  // Создание потоков для взаимодействия с каждым сервером
  for (unsigned int i = 0; i < servers_num; i++) {
    // Определение диапазона чисел для текущего сервера
    uint64_t numbers_for_this_server = numbers_per_server;
    if (i < remainder) {
      numbers_for_this_server++;  // Распределение остатка по первым серверам
    }

    // Настройка параметров для текущего потока
    thread_args[i].server = servers[i];
    thread_args[i].begin = current_begin;
    thread_args[i].end = current_begin + numbers_for_this_server - 1;
    thread_args[i].mod = mod;
    thread_args[i].result = &results[i];  // Указатель на ячейку для результата

    // Вывод информации о распределении
    printf("Server %s:%d will process range [%" PRIu64 ", %" PRIu64 "]\n", 
           servers[i].ip, servers[i].port, thread_args[i].begin, thread_args[i].end);

    // Создание потока для взаимодействия с сервером
    if (pthread_create(&threads[i], NULL, ProcessServer, &thread_args[i])) {
      fprintf(stderr, "Error creating thread for server %s:%d\n", 
              servers[i].ip, servers[i].port);
      results[i] = 1;  // При ошибке используем нейтральный элемент умножения
    }

    current_begin += numbers_for_this_server;  // Сдвиг начала для следующего сервера
  }

  // СБОР И ОБЪЕДИНЕНИЕ РЕЗУЛЬТАТОВ
  
  uint64_t total = 1;  // Нейтральный элемент для умножения
  
  // Ожидание завершения всех потоков
  for (unsigned int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    // Последовательное умножение результатов по модулю
    total = MultModulo(total, results[i], mod);
  }

  // ВЫВОД ФИНАЛЬНОГО РЕЗУЛЬТАТА
  
  printf("\nFinal result: %" PRIu64 "! mod %" PRIu64 " = %" PRIu64 "\n", k, mod, total);

  // ОСВОБОЖДЕНИЕ РЕСУРСОВ
  free(servers);  // Освобождение массива серверов
  
  return 0;
}