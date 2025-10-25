#include "sum_utils.h"
#include <pthread.h>

// Функция для вычисления суммы элементов массива в заданном диапазоне
int Sum(const struct SumArgs *args) {
  int sum = 0;
  for (int i = args->begin; i < args->end; i++) {
    sum += args->array[i];
  }
  return sum;
}

// Функция, которую выполняет каждый поток
void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(long)Sum(sum_args);
}

// Основная функция параллельного суммирования
long long ParallelSum(int *array, int array_size, int threads_num) {
  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];
  
  // Вычисляем размер части массива для каждого потока
  int chunk_size = array_size / threads_num;
  
  // Создаем аргументы для каждого потока
  for (int i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = i * chunk_size;
    args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk_size;
    
    // Создаем поток
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i]) != 0) {
      return -1; // Ошибка создания потока
    }
  }

  // Собираем результаты от всех потоков
  long long total_sum = 0;
  for (int i = 0; i < threads_num; i++) {
    void *result;
    pthread_join(threads[i], &result);
    total_sum += (long)result;
  }
  
  return total_sum;
}