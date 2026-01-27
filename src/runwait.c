#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
                "Usage: %s <command> [args...]\n",
                argv[0]);
        return 1;
    }

    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime start failed");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        execvp(argv[1], &argv[1]);
        perror("execvp failed");
        exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid failed");
        return 1;
    }

    if (clock_gettime(CLOCK_MONOTONIC, &end) == -1) {
        perror("clock_gettime end failed");
        return 1;
    }

    double elapsed =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Child PID: %d\n", pid);

    if (WIFEXITED(status)) {
       printf("exit=%d\n", WEXITSTATUS(status));
}   else if (WIFSIGNALED(status)) {
       printf("exit=%d\n", 128 + WTERMSIG(status));
}   else {
       printf("exit=unknown\n");
}


    printf("Elapsed time: %.6f sec\n", elapsed);

    return 0;
}
