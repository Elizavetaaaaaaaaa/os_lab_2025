#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    // argv[0] - имя самой программы
    // argv[1] - seed для генератора случайных чисел
    // argv[2] - размер массива
    if (argc != 3) {
        printf("Usage: %s seed arraysize\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс - запускаем sequential_min_max
        char *args[] = {"./sequential_min_max", argv[1], argv[2], NULL};
        execvp(args[0], args);
        
        // Если execvp вернул управление, значит произошла ошибка
        perror("execvp failed");
        exit(1);
    } else if (pid > 0) {
        // Родительский процесс
        
        // Переменная для хранения статуса завершения дочернего процесса
        int status;
        
        // Ожидаем завершения дочернего процесса
        waitpid(pid, &status, 0);
        
        // Проверяем, нормально ли завершился дочерний процесс
        if (WIFEXITED(status)) {
            printf("Child process exited with status: %d\n", WEXITSTATUS(status));
        } else {
            // Процесс завершился аномально (сигнал и т.д.)
            printf("Child process terminated abnormally\n");
        }
    } else {
        perror("fork failed");  
        return 1;        
    }
    return 0;
}