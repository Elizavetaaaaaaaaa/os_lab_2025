/**
 * Deadlock (взаимная блокировка) — это ситуация в многопоточном программировании, когда два или более потоков
 *  находятся в состоянии бесконечного ожидания ресурсов, захваченных друг другом. Ни один из потоков не может
 *  продолжить выполнение.
 **/

 /*
 * Демонстрация состояния взаимной блокировки (deadlock)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>  


// Глобальные мьютексы для демонстрации deadlock
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

/**
 * Функция для красивого вывода сообщений с временными метками
 */
void print_message(const char* thread_name, const char* message) {
    printf("[%s] %s\n", thread_name, message);
    fflush(stdout); // Немедленный вывод
}

/**
 * Функция для попытки захвата мьютекса с таймаутом
 * Возвращает 0 если успешно, -1 если таймаут
 */
int try_lock_with_timeout(pthread_mutex_t* mutex, const char* mutex_name, const char* thread_name, int timeout_sec) {
    print_message(thread_name, "Пытаюсь захватить мьютекс...");
    
    struct timeval now;
    gettimeofday(&now, NULL);
    
    struct timespec ts;
    ts.tv_sec = now.tv_sec + timeout_sec;
    ts.tv_nsec = now.tv_usec * 1000;
    
    int result = pthread_mutex_timedlock(mutex, &ts);
    
    if (result == 0) {
        print_message(thread_name, "Успешно захватил мьютекс!");
        return 0;
    } else if (result == ETIMEDOUT) {
        print_message(thread_name, "Таймаут при захвате мьютекса!");
        return -1;
    } else {
        print_message(thread_name, "Ошибка при захвате мьютекса!");
        return -1;
    }
}

/**
 * Альтернативная функция с использованием pthread_mutex_trylock (более совместимая)
 */
int try_lock_with_retry(pthread_mutex_t* mutex, const char* mutex_name, const char* thread_name, int max_attempts) {
    int attempts = 0;
    
    while (attempts < max_attempts) {
        printf("[%s] Попытка %d захватить %s...\n", thread_name, attempts + 1, mutex_name);
        
        if (pthread_mutex_trylock(mutex) == 0) {
            printf("[%s] Успешно захватил %s!\n", thread_name, mutex_name);
            return 0;
        }
        
        printf("[%s] %s занят, жду 1 секунду...\n", thread_name, mutex_name);
        sleep(1);
        attempts++;
    }
    
    printf("[%s] Таймаут! Не удалось захватить %s после %d попыток\n", 
           thread_name, mutex_name, max_attempts);
    return -1;
}

/**
 * Поток 1: захватывает mutex1, затем пытается захватить mutex2
 */
void* thread1_function(void* arg) {
    print_message("ПОТОК-1", "Запущен");
    
    print_message("ПОТОК-1", "Захватываю mutex1...");
    pthread_mutex_lock(&mutex1);
    print_message("ПОТОК-1", "Успешно захватил mutex1");
    
    // Имитация работы с захваченным ресурсом
    sleep(2);
    
    print_message("ПОТОК-1", "Пытаюсь захватить mutex2 (будет ждать, так как mutex2 у ПОТОКА-2)...");
    
    // Версия с deadlock (обычный lock)
    pthread_mutex_lock(&mutex2);
    print_message("ПОТОК-1", "Успешно захватил mutex2 - ЭТОГО НЕ ПРОИЗОЙДЕТ ПРИ DEADLOCK!");
    
    // Критическая секция (никогда не выполнится при deadlock)
    print_message("ПОТОК-1", "Выполняю критическую секцию");
    sleep(1);
    print_message("ПОТОК-1", "Завершил критическую секцию");
    
    // Освобождение ресурсов
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    
    print_message("ПОТОК-1", "Завершил работу");
    return NULL;
}

/**
 * Поток 2: захватывает mutex2, затем пытается захватить mutex1  
 * (обратный порядок - приводит к deadlock)
 */
void* thread2_function(void* arg) {
    print_message("ПОТОК-2", "Запущен");
    
    print_message("ПОТОК-2", "Захватываю mutex2...");
    pthread_mutex_lock(&mutex2);
    print_message("ПОТОК-2", "Успешно захватил mutex2");
    
    // Имитация работы с захваченным ресурсом
    sleep(2);
    
    print_message("ПОТОК-2", "Пытаюсь захватить mutex1 (будет ждать, так как mutex1 у ПОТОКА-1)...");
    
    // Версия с deadlock (обычный lock)
    pthread_mutex_lock(&mutex1);
    print_message("ПОТОК-2", "Успешно захватил mutex1 - ЭТОГО НЕ ПРОИЗОЙДЕТ ПРИ DEADLOCK!");
    
    // Критическая секция (никогда не выполнится при deadlock)
    print_message("ПОТОК-2", "Выполняю критическую секцию");
    sleep(1);
    print_message("ПОТОК-2", "Завершил критическую секцию");
    
    // Освобождение ресурсов
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    
    print_message("ПОТОК-2", "Завершил работу");
    return NULL;
}

/**
 * Безопасная версия потоков с таймаутом (для сравнения)
 */
void* thread1_safe_function(void* arg) {
    print_message("ПОТОК-1", "Запущен (безопасная версия)");
    
    // Используем совместимую версию с trylock
    if (try_lock_with_retry(&mutex1, "mutex1", "ПОТОК-1", 5) == 0) {
        sleep(2);
        
        if (try_lock_with_retry(&mutex2, "mutex2", "ПОТОК-1", 3) == 0) {
            print_message("ПОТОК-1", "Выполняю критическую секцию");
            sleep(1);
            pthread_mutex_unlock(&mutex2);
            print_message("ПОТОК-1", "Освободил mutex2");
        } else {
            print_message("ПОТОК-1", "Не удалось захватить mutex2, освобождаю mutex1");
        }
        
        pthread_mutex_unlock(&mutex1);
        print_message("ПОТОК-1", "Освободил mutex1");
    }
    
    print_message("ПОТОК-1", "Завершил работу (безопасная версия)");
    return NULL;
}

void* thread2_safe_function(void* arg) {
    print_message("ПОТОК-2", "Запущен (безопасная версия)");
    
    // ЗАМЕТКА: Для предотвращения deadlock всегда захватываем мьютексы в одном порядке!
    // В безопасной версии оба потока пытаются захватить мьютексы в одинаковом порядке
    if (try_lock_with_retry(&mutex1, "mutex1", "ПОТОК-2", 5) == 0) {
        sleep(2);
        
        if (try_lock_with_retry(&mutex2, "mutex2", "ПОТОК-2", 3) == 0) {
            print_message("ПОТОК-2", "Выполняю критическую секцию");
            sleep(1);
            pthread_mutex_unlock(&mutex2);
            print_message("ПОТОК-2", "Освободил mutex2");
        } else {
            print_message("ПОТОК-2", "Не удалось захватить mutex2, освобождаю mutex1");
        }
        
        pthread_mutex_unlock(&mutex1);
        print_message("ПОТОК-2", "Освободил mutex1");
    }
    
    print_message("ПОТОК-2", "Завершил работу (безопасная версия)");
    return NULL;
}

/**
 * Основная функция
 */
int main(int argc, char* argv[]) {
    pthread_t thread1, thread2;
    int use_safe_version = 0;
    
    printf("=== ДЕМОНСТРАЦИЯ DEADLOCK ===\n\n");
    
    // Проверка аргументов командной строки
    if (argc > 1) {
        if (strcmp(argv[1], "--safe") == 0) {
            use_safe_version = 1;
            printf("Режим: БЕЗОПАСНАЯ версия (с таймаутами)\n");
        } else if (strcmp(argv[1], "--deadlock") == 0) {
            use_safe_version = 0;
            printf("Режим: DEADLOCK версия (взаимная блокировка)\n");
        }
    } else {
        printf("Режим: DEADLOCK версия (взаимная блокировка)\n");
        printf("Используйте: %s --safe для безопасной версии\n", argv[0]);
        printf("Используйте: %s --deadlock для версии с deadlock\n\n", argv[0]);
    }
    
    printf("Сценарий:\n");
    printf("- ПОТОК-1 захватывает mutex1, затем пытается захватить mutex2\n");
    printf("- ПОТОК-2 захватывает mutex2, затем пытается захватить mutex1\n");
    printf("- Результат: ВЗАИМНАЯ БЛОКИРОВКА (deadlock)\n\n");
    
    if (use_safe_version) {
        printf("Создаю потоки (безопасная версия с таймаутами)...\n");
        
        if (pthread_create(&thread1, NULL, thread1_safe_function, NULL) != 0) {
            perror("Ошибка создания потока 1");
            exit(1);
        }
        
        if (pthread_create(&thread2, NULL, thread2_safe_function, NULL) != 0) {
            perror("Ошибка создания потока 2");
            exit(1);
        }
    } else {
        printf("Создаю потоки (deadlock версия)...\n");
        printf("ПРОГРАММА ЗАВИСНЕТ! Для выхода нажмите Ctrl+C\n\n");
        
        if (pthread_create(&thread1, NULL, thread1_function, NULL) != 0) {
            perror("Ошибка создания потока 1");
            exit(1);
        }
        
        if (pthread_create(&thread2, NULL, thread2_function, NULL) != 0) {
            perror("Ошибка создания потока 2");
            exit(1);
        }
    }
    
    // Ожидание завершения потоков
    printf("\nОжидание завершения потоков...\n");
    
    if (pthread_join(thread1, NULL) != 0) {
        perror("Ошибка ожидания потока 1");
    }
    
    if (pthread_join(thread2, NULL) != 0) {
        perror("Ошибка ожидания потока 2");
    }
    
    // Очистка ресурсов
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    
    printf("\n=== ПРОГРАММА ЗАВЕРШЕНА ===\n");
    
    return 0;
}