#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    } 
    else if (pid == 0) {
        // --- ДОЧЕРНИЙ ПРОЦЕСС ---
        printf("Child process (PID: %d) is running...\n", getpid());
        printf("Child process is finishing and exiting.\n");
        exit(0); // Ребенок завершается
    } 
    else {
        // --- РОДИТЕЛЬСКИЙ ПРОЦЕСС ---
        printf("Parent process (PID: %d) created child (PID: %d).\n", getpid(), pid);
        
        // Мы НЕ вызываем wait() или waitpid().
        // Ребенок уже завершился, но запись о нем осталась в таблице процессов.
        printf("Parent is sleeping. The child is now a ZOMBIE.\n");
        printf("Open another terminal and run: ps -el | grep Z\n");
        
        sleep(60); 

        printf("Parent process is now cleaning up (calling wait).\n");
        
        // Убираем зомби (собираем статус завершения)
        wait(NULL); 
        
        printf("Zombie process is gone. Parent exits.\n");
    }

    return 0;
}