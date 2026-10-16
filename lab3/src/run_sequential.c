#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s seed array_size\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // ---- ребёнок ----
        // argv[0] для новой программы, argv[1] — seed, argv[2] — array_size
        execl("./sequential_min_max", "sequential_min_max",
              argv[1], argv[2], (char *)NULL);

        // если дошли сюда — exec провалился
        perror("execl");
        _exit(1);
    }

    // ---- родитель ----
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("Child exited with code %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Child killed by signal %d\n", WTERMSIG(status));
    }

    return 0;
}