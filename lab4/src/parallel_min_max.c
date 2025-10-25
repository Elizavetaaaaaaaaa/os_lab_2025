#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальная переменная для хранения PID дочернего процесса
pid_t child_pid = 0;

// Обработчик сигнала SIGALRM
void timeout_handler(int sig) {
    if (child_pid > 0) {
        printf("Timeout reached! Killing child process %d\n", child_pid);
        kill(child_pid, SIGKILL);
    }
}

int main(int argc, char **argv) {
    int timeout = 0; // Таймаут по умолчанию - отключен
    int seed = 0;
    int array_size = 0;
    int use_files = 0;
    
    // Анализ аргументов командной строки
    if (argc < 3) {
        printf("Usage: %s seed arraysize [--by_files] [--timeout N]\n", argv[0]);
        return 1;
    }

    // Первые два аргумента - seed и arraysize (позиционные)
    seed = atoi(argv[1]);
    array_size = atoi(argv[2]);

    // Обработка остальных аргументов (опциональные флаги)
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--by_files") == 0) {
            use_files = 1;
        } else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            timeout = atoi(argv[i + 1]);
            i++; // Пропускаем следующий аргумент (значение таймаута)
        } else {
            printf("Unknown parameter: %s\n", argv[i]);
            printf("Usage: %s seed arraysize [--by_files] [--timeout N]\n", argv[0]);
            return 1;
        }
    }

    // Проверка валидности seed
    if (seed <= 0) {
        printf("seed is a positive number\n");
        return 1;
    }

    // Проверка валидности размера массива
    if (array_size <= 0) {
        printf("array_size is a positive number\n");
        return 1;
    }

    // Устанавливаем обработчик сигнала SIGALRM, если таймаут задан
    if (timeout > 0) {
        signal(SIGALRM, timeout_handler);
        alarm(timeout); // Устанавливаем будильник
        printf("Timeout set to %d seconds\n", timeout);
    }

    // Выделение памяти и генерация массива
    int *array = malloc(array_size * sizeof(int));
    if (array == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }
    GenerateArray(array, array_size, seed);

    // Разделяем массив на две части
    unsigned int mid = array_size / 2;
    
    // Создаем дочерний процесс
    pid_t pid = fork();
    child_pid = pid; // Сохраняем PID дочернего процесса
    
    if (pid == 0) {
        // ДОЧЕРНИЙ ПРОЦЕСС - обрабатывает первую половину массива
        struct MinMax min_max = GetMinMax(array, 0, mid);
        
        if (use_files) {
            // Режим файлов: записываем результаты в файл
            FILE *file = fopen("child_result.txt", "w");
            if (file != NULL) {
                fprintf(file, "%d %d", min_max.min, min_max.max);
                fclose(file);
            }
        } else {
            // Режим pipe: передаем результаты через канал
            int fd[2];
            if (pipe(fd) == 0) {
                write(fd[1], &min_max.min, sizeof(int));
                write(fd[1], &min_max.max, sizeof(int));
                close(fd[1]);
            }
        }
        
        free(array);
        exit(0);
    } else if (pid > 0) {
        // РОДИТЕЛЬСКИЙ ПРОЦЕСС - обрабатывает вторую половину массива
        struct MinMax parent_min_max = GetMinMax(array, mid, array_size);
        struct MinMax child_min_max;
        int status;
        
        if (use_files) {
            // Режим файлов: ждем завершения дочернего процесса
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                // Дочерний процесс завершился нормально
                FILE *file = fopen("child_result.txt", "r");
                if (file != NULL) {
                    fscanf(file, "%d %d", &child_min_max.min, &child_min_max.max);
                    fclose(file);
                    remove("child_result.txt"); // Удаляем временный файл
                }
            } else {
                // Дочерний процесс был завершен по таймауту
                printf("Child process terminated by timeout\n");
                child_min_max = parent_min_max; // Используем только результаты родителя
            }
        } else {
            // Режим pipe: создаем pipe и читаем результаты
            int fd[2];
            if (pipe(fd) == 0) {
                waitpid(pid, &status, 0);
                
                if (WIFEXITED(status)) {
                    read(fd[0], &child_min_max.min, sizeof(int));
                    read(fd[0], &child_min_max.max, sizeof(int));
                } else {
                    printf("Child process terminated abnormally\n");
                    child_min_max = parent_min_max;
                }
                close(fd[0]);
            }
        }
        
        // Объединяем результаты
        struct MinMax final_min_max;
        final_min_max.min = (parent_min_max.min < child_min_max.min) ? 
                           parent_min_max.min : child_min_max.min;
        final_min_max.max = (parent_min_max.max > child_min_max.max) ? 
                           parent_min_max.max : child_min_max.max;
        
        // Выводим результаты
        printf("min: %d\n", final_min_max.min);
        printf("max: %d\n", final_min_max.max);
        
        free(array);
        return 0;
    } else {
        // ОШИБКА при создании процесса
        perror("fork failed");
        free(array);
        return 1;
    }
}