#ifndef SUM_UTILS_H
#define SUM_UTILS_H

#include <stdint.h>

// Структура для передачи аргументов в поток
struct SumArgs {
  int *array;
  int begin;
  int end;
};

// Функция для вычисления суммы части массива
int Sum(const struct SumArgs *args);

// Функция для параллельного суммирования
long long ParallelSum(int *array, int array_size, int threads_num);

#endif