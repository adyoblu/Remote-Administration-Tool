#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/wait.h>
#define BUFFSIZE 4096
#define MAX_BUFFER_SIZE 1024
#define exitServer 0
#define afisareClienti 6
#define whitelistBlacklist 7
#define whitelist '1'
#define blacklist '2'
#define getHostname 1
#define getProcesses 2
#define executeCommand 3
#define reboot 4
#define kick 5
#define sendfile 8
#define getFile 9
#define SOCKETERROR (-1)
#define THREAD_POOL_SIZE 10

double getMEMusage(pid_t pid, char* buffer, long int rss){
    char path[128];
    snprintf(path, sizeof(path), "/proc/%d/statm", pid);
    int pro = open(path, O_RDONLY);
    ssize_t byt = read(pro, buffer, sizeof(buffer) - 1);
    close(pro);
    buffer[byt] = '\0';
    sscanf(buffer, "%*d %ld", &rss);
    long int pagesize = sysconf(_SC_PAGESIZE);

    // Calculate total physical memory for the system
    long int phys_pages = sysconf(_SC_PHYS_PAGES);
    long int phys_mem = phys_pages * pagesize;

    // Calculate %Mem
    double mem_usage = ((double)rss * pagesize) / phys_mem * 100;
    return mem_usage;
}

double getCPUusage(int utime, int stime, unsigned int starttime){
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        perror("Error getting system information");
    }

    // Calculate %CPU
    double total_time = utime + stime;
    double seconds = info.uptime - ((double)starttime / sysconf(_SC_CLK_TCK));

    // Check for division by zero
    double cpu_usage = 0;
    if (seconds != 0) {
        cpu_usage = 100 * (total_time / sysconf(_SC_CLK_TCK) / seconds);
    }
    return cpu_usage;
}

char* getProcessUserFromLs(const char *pid) {
    char command[BUFFSIZE];
    snprintf(command, sizeof(command), "ls -l /proc/%s/stat", pid);

    FILE *lsOutput = popen(command, "r");
    if (lsOutput == NULL)
        return NULL;

    char buffer[BUFFSIZE];
    if (fgets(buffer, sizeof(buffer), lsOutput) != NULL) {
        char *copy = strdup(buffer);
        char *token = strtok(copy, " ");

        for (int i = 0; i < 2; ++i) {
            token = strtok(NULL, " ");
        }

        char *username = strdup(token);
        strtok(username, "\n");

        pclose(lsOutput);
        free(copy);
        return username;
    }

    pclose(lsOutput);
    return NULL;
}