#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define BUF_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    char *endptr;
    long pid = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || pid <= 0) {
        fprintf(stderr, "Invalid PID: %s\n", argv[1]);
        return 1;
    }

    char path[BUF_SIZE];
    FILE *fp;

    /* -------- /proc/<pid>/stat -------- */
    snprintf(path, sizeof(path), "/proc/%ld/stat", pid);
    fp = fopen(path, "r");
    if (!fp) {
        perror("Error opening stat");
        return 1;
    }

    char buf[BUF_SIZE];
    if (!fgets(buf, sizeof(buf), fp)) {
        perror("Failed to read stat file");
        fclose(fp);
        return 1;
    }
    fclose(fp);

    char *start = strchr(buf, '(');
    char *end = strrchr(buf, ')');
    if (!start || !end || end <= start) {
        fprintf(stderr, "Malformed stat file\n");
        return 1;
    }

    char comm[256];
    size_t len = end - start - 1;
    if (len >= sizeof(comm)) len = sizeof(comm) - 1;
    strncpy(comm, start + 1, len);
    comm[len] = '\0';

    char state;
    long ppid, utime, stime;
    if (sscanf(end + 2,
               "%c %ld %*d %*d %*d %*u %*u %*u %*u %*u %ld %ld",
               &state, &ppid, &utime, &stime) != 4) {
        fprintf(stderr, "Failed to parse stat fields\n");
        return 1;
    }

    double cpu_time = (utime + stime) /
                      (double) sysconf(_SC_CLK_TCK);

    /* -------- /proc/<pid>/status -------- */
    snprintf(path, sizeof(path), "/proc/%ld/status", pid);
    fp = fopen(path, "r");
    if (!fp) {
        perror("Error opening status");
        return 1;
    }

    char line[BUF_SIZE];
    long rss = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "VmRSS: %ld kB", &rss) == 1) {
            break;
        }
    }
    fclose(fp);

    /* -------- /proc/<pid>/cmdline -------- */
    snprintf(path, sizeof(path), "/proc/%ld/cmdline", pid);
    fp = fopen(path, "r");

    char cmdline[BUF_SIZE] = "";
    if (fp) {
        size_t n = fread(cmdline, 1,
                          sizeof(cmdline) - 1, fp);
        fclose(fp);
        for (size_t i = 0; i < n; i++) {
            if (cmdline[i] == '\0')
                cmdline[i] = ' ';
        }
        cmdline[n] = '\0';
    }

    const char *display_name =
        cmdline[0] ? cmdline : comm;

    printf("Process: %s\n", display_name);
    printf("State: %c\n", state);
    printf("Parent PID: %ld\n", ppid);
    printf("CPU time: %.2f sec\n", cpu_time);
    printf("Resident Memory: %ld kB\n", rss);

    return 0;
}

