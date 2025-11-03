 /*
 Написать программу для паралелльного вычисления факториала по модулю mod (k!), которая будет 
 принимать на вход следующие параметры (пример: -k 10 --pnum=4 --mod=10):
    k - число, факториал которого необходимо вычислить.
    pnum - количество потоков.
    mod - модуль факториала
Для синхронизации результатов необходимо использовать мьютексы.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

// Структура для передачи данных в поток
typedef struct {
    int start;          // Начальное число для вычисления
    int end;            // Конечное число для вычисления
    int mod;            // Модуль для вычисления
    long long *result;  // Указатель на общий результат
    pthread_mutex_t *mutex;  // Указатель на мьютекс для синхронизации
    int thread_id;      // ID потока (для отладки)
} thread_data_t;

/**
 * Функция, выполняемая каждым потоком
 * Вычисляет частичный факториал от start до end
 */
void* calculate_partial_factorial(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    long long partial_result = 1;
    
    printf("Поток %d: вычисляю факториал от %d до %d по модулю %d\n", 
           data->thread_id, data->start, data->end, data->mod);
    
    // Вычисление частичного факториала для диапазона [start, end]
    for (int i = data->start; i <= data->end; i++) {
        partial_result = (partial_result * i) % data->mod;
    }
    
    printf("Поток %d: частичный результат = %lld\n", 
           data->thread_id, partial_result);
    
    // Синхронизированное обновление общего результата
    pthread_mutex_lock(data->mutex);
    printf("Поток %d: блокирую мьютекс, текущий общий результат = %lld\n", 
           data->thread_id, *(data->result));
    
    *(data->result) = (*(data->result) * partial_result) % data->mod;
    
    printf("Поток %d: новый общий результат = %lld, разблокирую мьютекс\n", 
           data->thread_id, *(data->result));
    pthread_mutex_unlock(data->mutex);
    
    return NULL;
}

/**
 * Функция для вывода справки по использованию
 */
void print_usage(const char* program_name) {
    printf("Использование: %s -k <число> --pnum=<потоки> --mod=<модуль>\n", program_name);
    printf("Параметры:\n");
    printf("  -k <число>        Число, факториал которого вычисляется (k!)\n");
    printf("  --pnum=<потоки>   Количество потоков для параллельного вычисления\n");
    printf("  --mod=<модуль>    Модуль для вычисления факториала (k! mod mod)\n");
    printf("\nПример: %s -k 10 --pnum=4 --mod=1000000\n", program_name);
}

int main(int argc, char* argv[]) {
    int k = 0;          // Число для вычисления факториала
    int pnum = 1;       // Количество потоков (по умолчанию 1)
    int mod = 0;        // Модуль
    
    // Разбор аргументов командной строки
    static struct option long_options[] = {
        {"k", required_argument, 0, 'k'},
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    printf("=== Параллельное вычисление факториала ===\n");
    
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "k:p:m:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 'k':
                k = atoi(optarg);
                if (k < 0) {
                    fprintf(stderr, "Ошибка: k должно быть неотрицательным числом\n");
                    exit(1);
                }
                break;
            case 'p':
                pnum = atoi(optarg);
                if (pnum <= 0) {
                    fprintf(stderr, "Ошибка: pnum должно быть положительным числом\n");
                    exit(1);
                }
                break;
            case 'm':
                mod = atoi(optarg);
                if (mod <= 0) {
                    fprintf(stderr, "Ошибка: mod должно быть положительным числом\n");
                    exit(1);
                }
                break;
            case 'h':
                print_usage(argv[0]);
                exit(0);
            default:
                fprintf(stderr, "Неизвестная опция\n");
                print_usage(argv[0]);
                exit(1);
        }
    }
    
    // Проверка обязательных параметров
    if (k == 0 || pnum == 0 || mod == 0) {
        fprintf(stderr, "Ошибка: все параметры (k, pnum, mod) должны быть указаны\n");
        print_usage(argv[0]);
        exit(1);
    }
    
    printf("Параметры: k = %d, потоков = %d, mod = %d\n", k, pnum, mod);
    
    // Особые случаи для факториала
    if (k == 0 || k == 1) {
        printf("Результат: %d! mod %d = 1\n", k, mod);
        return 0;
    }
    
    // Общий результат (инициализируется 1, так как 1 - нейтральный элемент для умножения)
    long long result = 1;
    
    // Инициализация мьютекса для синхронизации
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    
    // Создание массивов для потоков и их данных
    pthread_t threads[pnum];
    thread_data_t thread_data[pnum];
    
    // Распределение работы между потоками
    int numbers_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;  // Факториал начинается с 1
    
    printf("Распределение работы:\n");
    
    for (int i = 0; i < pnum; i++) {
        // Определение диапазона чисел для текущего потока
        int numbers_for_this_thread = numbers_per_thread;
        if (i < remainder) {
            numbers_for_this_thread++;
        }
        
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + numbers_for_this_thread - 1;
        thread_data[i].mod = mod;
        thread_data[i].result = &result;
        thread_data[i].mutex = &mutex;
        thread_data[i].thread_id = i + 1;
        
        printf("  Поток %d: числа от %d до %d (%d чисел)\n", 
               i + 1, current_start, thread_data[i].end, numbers_for_this_thread);
        
        current_start += numbers_for_this_thread;
        
        // Создание потока
        if (pthread_create(&threads[i], NULL, calculate_partial_factorial, &thread_data[i]) != 0) {
            perror("Ошибка при создании потока");
            exit(1);
        }
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Ошибка при ожидании потока");
            exit(1);
        }
    }
    
    // Уничтожение мьютекса
    pthread_mutex_destroy(&mutex);
    
    // Вывод финального результата
    printf("\n=== Результат ===\n");
    printf("%d! mod %d = %lld\n", k, mod, result);
    
    return 0;
}