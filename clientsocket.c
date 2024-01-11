#include "utilities.c"

void sendFile(int sock, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Eroare la deschiderea fisierului");
        return;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    send(sock, &fileSize, sizeof(size_t), 0);

    char buffer[BUFFSIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(sock, buffer, bytesRead, 0);
    }

    fclose(file);
} 

void selectFile(int sock)
{
    char filename[BUFFSIZE];
    ssize_t bytesRead = recv(sock, filename, sizeof(filename), 0);
    if (bytesRead <= 0) {
        perror("Eroare la primirea numelui fișierului");
        return;
    }
    sendFile(sock, filename);
}

void sendMessage(int sock)
{
    char message[BUFFSIZE];
    printf("Introduceti mesajul: ");
    fgets(message, sizeof(message), stdin);
    send(sock, message, strlen(message), 0);
    char response[BUFFSIZE];
    recv(sock, response, sizeof(response), 0);
    printf("Raspuns de la server: %s\n", response);
}

void Hostname(int sock)
{
    char message[BUFFSIZE];
    char hostname[BUFFSIZE];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        fprintf(stderr, "%s", hostname);
        size_t hostnameLength = strlen(hostname);
        size_t networkByteOrder = htonl(hostnameLength);
        send(sock, &networkByteOrder, sizeof(networkByteOrder), 0);
        send(sock, hostname, hostnameLength, 0);
    } else 
        perror("Eroare la obtinerea numarului PC-ului");
}

void sendProcessesList(int sock) {
    DIR *procDir = opendir("/proc");
    if (procDir == NULL) {
        perror("Eroare la deschiderea directorului /proc");
        return;
    }

    int outputFile = open("output", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (outputFile == -1) {
        perror("Eroare la deschiderea fisierului output");
        closedir(procDir);
        return;
    }

    char buffer[BUFFSIZE];
    int len = snprintf(buffer, 59, "USER\t\tPID\t%%CPU\t%%MEM\tVSZ\tRSS\tTTY\tSTAT\tSTART\tTIME\tCOMMAND\n");
    write(outputFile, buffer, len);
    
    struct dirent *entry;
    while ((entry = readdir(procDir)) != NULL) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name) > 0) {
            char procPath[BUFFSIZE];
            snprintf(procPath, sizeof(procPath), "/proc/%s/stat", entry->d_name);
            
            if(access(procPath, F_OK) != 0){
                continue;
            }

            int procFile = open(procPath, O_RDONLY);

            ssize_t bytesRead = read(procFile, buffer, sizeof(buffer));
            bytesRead++;
            buffer[bytesRead] = '\n';
            close(procFile);
            char *user = getProcessUserFromLs(entry->d_name);
            write(outputFile, user, strlen(user));//USER
            write(outputFile, "\t\t", 2);
            write(outputFile, entry->d_name, strlen(entry->d_name));//PID
            write(outputFile, "\t", 1);

            int utime, stime, pid;
            unsigned long starttime;
            long int rss;
            sscanf(buffer, "%d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %d %d %*d %*d %*d %*d %*d %*d %lu %*d",
            &pid, &utime, &stime, &starttime);

            double cpu_usage = getCPUusage(utime, stime, starttime);
            char cpuUsageStr[20];
            snprintf(cpuUsageStr, sizeof(cpuUsageStr), "%.2f", cpu_usage);
            write(outputFile, cpuUsageStr, strlen(cpuUsageStr));//%CPU

            write(outputFile, "\t", 1);
            double mem_usage = getMEMusage(pid, buffer, rss);
            char memUsage[20];
            snprintf(memUsage, sizeof(memUsage), "%.2f", mem_usage);
            write(outputFile, memUsage, strlen(memUsage));//%MEM
            
            write(outputFile, "\t", 1);
            //printf("PID %d, mem: %.2f%%\n", pid, mem_usage);
            

            // COMMAND: Read the command from /proc/<pid>/cmdline
            // snprintf(procPath, sizeof(procPath), "/proc/%s/cmdline", entry->d_name);
            // procFile = open(procPath, O_RDONLY);
            // if (procFile != -1) {
            //     bytesRead = read(procFile, buffer, sizeof(buffer));
            //     write(outputFile, &bytesRead, sizeof(bytesRead));
            //     close(procFile);
            // } else {
            //     perror("Error opening cmdline file");
            // }
            write(outputFile, "\n", 1);
        }
    }


    close(outputFile);

    int file = open("output", O_RDONLY);
    if (file == -1) {
        perror("Eroare la deschiderea fisierului output");
        closedir(procDir);
        return;
    }

    off_t fileSize = lseek(file, 0, SEEK_END);
    lseek(file, 0, SEEK_SET);

    // Citeste în buffer
    char *bufferRead = (char *)malloc(fileSize);
    ssize_t bytesRead = read(file, bufferRead, fileSize);
    close(file);
    bufferRead[fileSize+1] = '\0';
    // Trimite dimensiunea buffer-ului către server
    send(sock, &fileSize, sizeof(size_t), 0);
    // Trimite fișierul la server
    send(sock, bufferRead, bytesRead, 0);
    free(bufferRead);

    closedir(procDir);
}


void ExecuteCommand(int sock)
{   
    //fork exec si redirectare prin pipe-uri
    char command[BUFFSIZE];
    ssize_t bytesRead = recv(sock, command, sizeof(command), 0);
    if (bytesRead == -1) { //check for error
        perror("recv");
        exit(1);
    }
    if (bytesRead == 0) { //check for closed connection
        printf("Server closed connection\n");
        close(sock); //close socket
        exit(0);
    }
    printf("Comanda primita de la server: %s\n", command);
    command[strcspn(command, "\n")] = '\0'; //replace \n with \0
    if (strcmp(command, "exit") == 0) //va iesi
        exit(0);

    int pipefd[2], length;
    char *args[100];
    if(pipe(pipefd)){
        fprintf(stderr, "Failed to create pipe");
        exit(1);
    }

    pid_t pid = fork();
    char path[BUFFSIZE];

    if(pid == 0)
    {
        close(1); //inchid stdout
        dup2(pipefd[1], 1); //duplicare la stdout
        close(pipefd[0]); //inchid read
        close(pipefd[1]); //inchid write
        //parse command and store tokens in args
        int i = 0;
        char *token = strtok(command, " ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;//NULL
        execvp(args[0], args); //execute
        perror("execvp"); //if execvp returns, there is an error
        exit(1);
    }
    else if(pid > 0)
    {
        close(pipefd[1]);
        memset(path, 0, BUFFSIZE);
        if((length = read(pipefd[0], path, BUFFSIZE-1)) >= 0){
            send(sock, &length, sizeof(length), 0);
            if(send(sock, path, strlen(path), 0) != strlen(path)){
                fprintf(stderr,"Failed");
                exit(1);
            }
            fflush(NULL);
            memset(path,0,BUFFSIZE);
        }
        close(pipefd[0]);
    } else {
        printf("Error !\n");
        exit(0);
    }
}

void restartClient(int sock) {
    int result = system("reboot");
    send(sock, &result, sizeof(result), 0);
    if (result == -1){
        perror("Eroare la executarea comenzii de restart");
    }
}

void receiveFile(int sock) {
    //numele fisierului
    char filename[BUFFSIZE];
    ssize_t bytesRead = recv(sock, filename, sizeof(filename), 0);
    if (bytesRead <= 0) {
        perror("Eroare la primirea numelui fisierului");
        return;
    }

    //dimensiunea fisierului
    size_t fileSize;
    recv(sock, &fileSize, sizeof(size_t), 0);

    //scriere in fisier
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Eroare la deschiderea fisierului pentru scriere");
        return;
    }

    //scriere continut in fisier
    char buffer[BUFFSIZE];
    size_t totalBytesReceived = 0;

    while (totalBytesReceived < fileSize) {
        bytesRead = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            perror("Eroare la primirea datelor fisierului");
            fclose(file);
            return;
        }

        fwrite(buffer, 1, bytesRead, file);
        totalBytesReceived += bytesRead;
    }

    fclose(file);
    printf("Fisierul '%s' a fost primit si salvat cu succes.\n", filename);
}

void handleServerActions(int sock) {
    int timeout = 10;
    while (1) {
        char response[BUFFSIZE];
        recv(sock, response, sizeof(response), 0);
        printf("\n%s\n", response);
        int responseType = atoi(response);
        switch (responseType) {
            case getHostname:
                Hostname(sock);
                break;
            case getProcesses:
                sendProcessesList(sock);
                break;
            case executeCommand:
                ExecuteCommand(sock);
                break;
            case reboot:
                restartClient(sock);
                break;
            case kick:
                fprintf(stderr, "S-a terminat executia clientului.");
                exit(0);
                break;
            case sendfile:
                selectFile(sock);
                break;
            case getFile:
                receiveFile(sock);
                break;
        }
        timeout--;
        if(timeout < 0) break;
    }
}

int main(int argc, char *argv[]) {
 
    int sock = socket(AF_INET, SOCK_STREAM, 0);
  
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  
    serv_addr.sin_family = AF_INET;  
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");  
    serv_addr.sin_port = htons(5566); 
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Eroare la conectare");
        exit(EXIT_FAILURE);
    }
    
    handleServerActions(sock);
    close(sock);
    return 0;
}