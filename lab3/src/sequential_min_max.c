#include <stdio.h>
#include <stdlib.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  // Проверяем, что количество аргументов равно 3: имя программы, seed и размер массива
  // argc содержит количество переданных аргументов командной строки
  if (argc != 3) {
    printf("Usage: %s seed arraysize\n", argv[0]);
    return 1;
  }

  int seed = atoi(argv[1]);     // Преобразуем seed из строки в целое число
  if (seed <= 0) {
    printf("seed is a positive number\n");
    return 1; 
  }

  int array_size = atoi(argv[2]); 
  if (array_size <= 0) {
    printf("array_size is a positive number\n");
    return 1; 
  }

  // Выделяем динамическую память под массив целых чисел заданного размера
  int *array = malloc(array_size * sizeof(int));
  
  // GenerateArray (utils.h) заполняет массив псевдослучайными числами
  GenerateArray(array, array_size, seed);

  // GetMinMax проходит по массиву от индекса 0 до array_size и находит min/max
  struct MinMax min_max = GetMinMax(array, 0, array_size);

  free(array);
  
  printf("min: %d\n", min_max.min);
  printf("max: %d\n", min_max.max);

  return 0; 
}