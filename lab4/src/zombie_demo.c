#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>



int main() {
    printf("=== Демонстрация зомби процессов ===\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс: PID = %d, PPID = %d\n", getpid(), getppid());
        printf("Дочерний процесс завершается, но родитель не вызвал wait()\n");
        exit(0);
    } else {
        // Родительский процесс
        printf("Родительский процесс: PID = %d, создал дочерний %d\n", getpid(), pid);
        printf("Родительский процесс спит 30 секунд...\n");
        printf("В это время выполните 'ps aux | grep %d' чтобы увидеть зомби\n", pid);
        
        sleep(30); // Даем время для наблюдения зомби
        
        printf("Родительский процесс вызывает wait()...\n");
        wait(NULL); // Убираем зомби
        printf("Зомби процесс убран\n");
        
        sleep(5); // Даем время убедиться, что зомби исчез
    }
    
    return 0;
}