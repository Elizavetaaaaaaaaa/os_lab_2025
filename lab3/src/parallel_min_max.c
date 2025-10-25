#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

#include "find_min_max.h"
#include "utils.h"

// Дополнить программу parallel_min_max.c, так чтобы после заданного таймаута 
// родительский процесс посылал дочерним сигнал SIGKILL. Таймаут должен быть задан, как именной необязательный 
// параметр командной строки (--timeout 10). Если таймаут не задан, то выполнение программы не должно меняться.

// PID дочернего процесса
pid_t child_pid = 0;

// Обработчик сигнала SIGALRM - вызывается при срабатывании будильника
// sig - номер сигнала (в данном случае SIGALRM)
void timeout_handler(int sig) {
    if (child_pid > 0) {
        printf("Timeout reached! Killing child process %d\n", child_pid);
        // Отправляем сигнал SIGKILL дочернему процессу
        // SIGKILL - немедленное завершение процесса
        kill(child_pid, SIGKILL);
    }
}

int main(int argc, char **argv) {
    int timeout = 0; // Таймаут по умолчанию - отключен (0 секунд)
    int seed, array_size;
    int use_files = 0;
    
    if (argc < 3) {
        printf("Usage: %s seed arraysize [--by_files] [--timeout N]\n", argv[0]);
        return 1;
    }

    // Обработка аргументов командной строки
    // Используем while для гибкого разбора аргументов
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "--by_files") == 0) {
            // Флаг использования файлов для межпроцессного взаимодействия
            use_files = 1;
            i++;
        } else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            // Параметр таймаута: --timeout N
            // argv[i] содержит "--timeout", argv[i+1] содержит число секунд
            timeout = atoi(argv[i + 1]);
            i += 2; // Пропускаем два аргумента (флаг и значение)
        } else {
            // Это seed или array_size (позиционные аргументы)
            if (seed == 0) {
                seed = atoi(argv[i]);
            } else {
                array_size = atoi(argv[i]);
            }
            i++;
        }
    }

    if (seed <= 0) {
        printf("seed is a positive number\n");
        return 1;
    }

    if (array_size <= 0) {
        printf("array_size is a positive number\n");
        return 1;
    }

    // Настройка обработки таймаута, если он задан
    if (timeout > 0) {
        // Регистрируем обработчик сигнала SIGALRM
        // SIGALRM будет отправлен процессу после истечения timeout секунд
        signal(SIGALRM, timeout_handler);
        // Устанавливаем будильник на timeout секунд
        // Когда время истечет, ядро отправит процессу сигнал SIGALRM
        alarm(timeout);
    }


    int *array = malloc(array_size * sizeof(int));
    GenerateArray(array, array_size, seed);

    // точка разделения массива между процессами
    unsigned int mid = array_size / 2;
    
    // Создаем дочерний процесс с помощью fork()
    pid_t pid = fork();
    child_pid = pid; // Сохраняем PID дочернего процесса в глобальной переменной
    
    if (pid == 0) {
        // === ДОЧЕРНИЙ ПРОЦЕСС ===

        struct MinMax min_max = GetMinMax(array, 0, mid);
        
        if (use_files) {
            FILE *file = fopen("child_result.txt", "w");
            fprintf(file, "%d %d", min_max.min, min_max.max);
            fclose(file);
        } else {
            // Режим pipe: передаем результаты через канал
            int fd[2];
            pipe(fd); // Создаем pipe
            write(fd[1], &min_max.min, sizeof(int));
            write(fd[1], &min_max.max, sizeof(int));
            close(fd[1]); 
        }
        
        // Завершаем дочерний процесс
        exit(0);
    } else if (pid > 0) {
        // === РОДИТЕЛЬСКИЙ ПРОЦЕСС ===
        
        struct MinMax parent_min_max = GetMinMax(array, mid, array_size);
        struct MinMax child_min_max;
        
        int status; // Переменная для статуса завершения дочернего процесса
        int child_finished = 0; // Флаг завершения дочернего процесса
        
        if (use_files) {
            // Режим файлов: используем неблокирующее ожидание
            
            // Неблокирующее ожидание с WNOHANG
            // Цикл проверяет состояние дочернего процесса без блокировки
            while (!child_finished) {
                // WNOHANG - не блокировать, если дочерний процесс еще работает
                pid_t result = waitpid(pid, &status, WNOHANG);
                if (result == pid) {
                    // Дочерний процесс завершился
                    child_finished = 1;
                } else if (result == 0) {
                    // Дочерний процесс еще работает
                    // Делаем небольшую паузу перед следующей проверкой
                    usleep(100000); // Ждем 100ms (100000 микросекунд)
                } else {
                    // Ошибка при вызове waitpid
                    perror("waitpid failed");
                    break;
                }
            }
            
            // Обработка результатов дочернего процесса
            if (child_finished && WIFEXITED(status)) {
                // Дочерний процесс завершился нормально
                // Читаем результаты из файла
                FILE *file = fopen("child_result.txt", "r");
                fscanf(file, "%d %d", &child_min_max.min, &child_min_max.max);
                fclose(file);
                remove("child_result.txt"); // Удаляем временный файл
            } else {
                // Дочерний процесс был убит по таймауту или завершился аномально
                printf("Child process did not finish in time\n");
                // Используем результаты только родительского процесса
                child_min_max.min = parent_min_max.min;
                child_min_max.max = parent_min_max.max;
            }
        } else {
            // Режим pipe: используем блокирующее ожидание
            int fd[2];
            pipe(fd); 
            
            // Блокирующее ожидание завершения дочернего процесса
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                // Дочерний процесс завершился нормально
                read(fd[0], &child_min_max.min, sizeof(int));
                read(fd[0], &child_min_max.max, sizeof(int));
                close(fd[0]); 
            } else {
                // Дочерний процесс завершился аномально
                printf("Child process terminated abnormally\n");
                // Используем результаты только родительского процесса
                child_min_max.min = parent_min_max.min;
                child_min_max.max = parent_min_max.max;
            }
        }
        
        struct MinMax final_min_max;
        final_min_max.min = (parent_min_max.min < child_min_max.min) ? parent_min_max.min : child_min_max.min;
        final_min_max.max = (parent_min_max.max > child_min_max.max) ? parent_min_max.max : child_min_max.max;
        
        printf("min: %d\n", final_min_max.min);
        printf("max: %d\n", final_min_max.max);
        

        free(array);
        return 0;
    } else {

        perror("fork failed");
        free(array);
        return 1;
    }
}