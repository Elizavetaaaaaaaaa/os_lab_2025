#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#include "find_min_max.h"
#include "utils.h"

// Завершить программу parallel_min_max.c, так, чтобы задача нахождения минимума и максимума в массиве 
// решалась параллельно. Если выставлен аргумент by_files для синхронизации процессов использовать файлы (задание 2), 
// в противном случае использовать pipe (задание 3).

int main(int argc, char **argv) {
  // Проверяем количество аргументов командной строки
  // argv[0] - имя программы
  // argv[1] - seed для генератора случайных чисел
  // argv[2] - размер массива
  // argv[3] - опциональный флаг --by_files для выбора способа синхронизации
  if (argc != 3 && argc != 4) {
    printf("Usage: %s seed arraysize [--by_files]\n", argv[0]);
    return 1; 
  }

  int seed = atoi(argv[1]); 
  if (seed <= 0) {
    printf("seed is a positive number\n");
    return 1; 
  }

  int array_size = atoi(argv[2]);   
  if (array_size <= 0) {
    printf("array_size is a positive number\n");
    return 1; 
  }

  // Флаг для выбора способа синхронизации между процессами: 0 - использовать pipe, 1 - использовать файлы 
  int use_files = 0;

  if (argc == 4 && strcmp(argv[3], "--by_files") == 0) {
    use_files = 1; 
  }

  int *array = malloc(array_size * sizeof(int));

  GenerateArray(array, array_size, seed);

  unsigned int mid = array_size / 2;  // точка разделения между процессами
  
  pid_t pid = fork();  // fork() - создание нового процесса
  
  if (pid == 0) {
    // === ДОЧЕРНИЙ ПРОЦЕСС ===
    
    struct MinMax min_max = GetMinMax(array, 0, mid);
    
    if (use_files) {
      FILE *file = fopen("child_result.txt", "w");  
      fprintf(file, "%d %d", min_max.min, min_max.max);
      fclose(file);
    } else {

      // Создаем pipe (канал для межпроцессного взаимодействия)
      // fd[0] - дескриптор для чтения, fd[1] - дескриптор для записи
      int fd[2];
      pipe(fd);
      
      write(fd[1], &min_max.min, sizeof(int));  
      write(fd[1], &min_max.max, sizeof(int));  
  
      close(fd[1]);
    }
    
    
    exit(0);    // Завершаем дочерний процесс
  } else {
    // === РОДИТЕЛЬСКИЙ ПРОЦЕСС ===
    
    struct MinMax parent_min_max = GetMinMax(array, mid, array_size);

    struct MinMax child_min_max;
    
    if (use_files) {
      // Ждем завершения дочернего процесса с помощью wait()
      // NULL означает, что нам не нужна дополнительная информация о статусе
      wait(NULL);
      
      FILE *file = fopen("child_result.txt", "r");
      fscanf(file, "%d %d", &child_min_max.min, &child_min_max.max);
      fclose(file);
      remove("child_result.txt");
    } else {
      // Создаем pipe для получения данных от дочернего процесса
      int fd[2];
      pipe(fd);
      
      // Ждем завершения дочернего процесса
      wait(NULL);
      

      read(fd[0], &child_min_max.min, sizeof(int));  
      read(fd[0], &child_min_max.max, sizeof(int)); 

      close(fd[0]);
    }
    
    struct MinMax final_min_max;
    final_min_max.min = (parent_min_max.min < child_min_max.min) ? parent_min_max.min : child_min_max.min;
    final_min_max.max = (parent_min_max.max > child_min_max.max) ? parent_min_max.max : child_min_max.max;
    

    printf("min: %d\n", final_min_max.min);
    printf("max: %d\n", final_min_max.max);

    free(array);

    return 0;
  }
}