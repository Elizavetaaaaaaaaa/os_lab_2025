#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <pthread.h>

#include "utils.h"
#include "sum_utils.h"

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;
  
  // Анализ аргументов командной строки
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--threads_num") == 0 && i + 1 < argc) {
      threads_num = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
      seed = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--array_size") == 0 && i + 1 < argc) {
      array_size = atoi(argv[++i]);
    }
  }
  
  // Проверка корректности аргументов
  if (threads_num <= 0 || array_size <= 0 || seed <= 0) {
    printf("Usage: %s --threads_num <num> --seed <num> --array_size <num>\n", argv[0]);
    printf("All parameters must be positive numbers\n");
    return 1;
  }
  
  // Выделение памяти и генерация массива (не входит в замер времени)
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  // Замер времени выполнения только суммирования
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  
  // Параллельное суммирование
  long long total_sum = ParallelSum(array, array_size, threads_num);
  
  clock_gettime(CLOCK_MONOTONIC, &end);
  
  // Вычисление времени выполнения
  double time_taken = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1000000000.0;
  
  // Вывод результатов
  printf("=== Parallel Sum Results ===\n");
  printf("Threads number: %u\n", threads_num);
  printf("Array size: %u\n", array_size);
  printf("Seed: %u\n", seed);
  printf("Total sum: %lld\n", total_sum);
  printf("Time taken: %.6f seconds\n", time_taken);
  
  free(array);
  return 0;
}