#include "utilities.c"

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

    char buffer[BUFFSIZE] = "USER\t\tPID\t%CPU\t%MEM\tVSZ\t\tRSS\t\tTTY\tSTAT\tSTART\tTIME\tCOMMAND\n";
    write(outputFile, buffer, strlen(buffer));
    
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
            unsigned long starttime, vsz;
            long int rss;
            int tty;
            char state;
            sscanf(buffer, "%d %*s %c %*d %*d %*d %d %*d %*u %*u %*u %*u %*u %d %d %*d %*d %*d %*d %*d %*d %lu %lu %ld",
            &pid, &state, &tty, &utime, &stime, &starttime, &vsz, &rss);
            
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

            vsz /= 1024;
            char vszStr[20];
            snprintf(vszStr, sizeof(vszStr), "%lu", vsz);
            write(outputFile, vszStr, strlen(vszStr)); // VSZ

            write(outputFile, "\t\t", 2);

            rss *= getpagesize() / 1024;

            char rssStr[20];
            snprintf(rssStr, sizeof(rssStr), "%ld", rss);
            write(outputFile, rssStr, strlen(rssStr)); // RSS

            write(outputFile, "\t\t", 2);
            
            char tty_str[16];

            convert_tty(tty, tty_str);// TTY
            write(outputFile, tty_str, strlen(tty_str));

            write(outputFile, "\t", 1);

            char status[16];
            snprintf(status, sizeof(status), "%c", state);
            write(outputFile, status, strlen(status));//state
 
            write(outputFile, "\t", 1);

            char times[20];
            struct sysinfo info;
            sysinfo(&info);
            double seconds = info.uptime - ((double)starttime / sysconf(_SC_CLK_TCK));

            int hours = seconds / 3600;
            int minutes = (seconds - hours * 3600) / 60;

            snprintf(times, sizeof(times), "%02d:%02d", hours, minutes);
    
            write(outputFile, times, strlen(times));// STARTTIME
            write(outputFile, "\t", 1);
            char star[20];
            int total_seconds = (int)(utime + stime) / sysconf(_SC_CLK_TCK);

            int min = total_seconds / 60;
            int sec = total_seconds % 60;

            snprintf(star, sizeof(star), "%i:%i", min, sec);

            write(outputFile, star, strlen(star));// TIME


            write(outputFile, "\t", 1);
            
            char poe[50];
            snprintf(poe, sizeof(poe), "/proc/%d/cmdline", pid);
            int sd = open(procPath, O_RDONLY);
            char sta[20];
            ssize_t byts = read(sd, sta, 20);
            close(sd);
            sta[byts] = '\0';
            write(outputFile, sta, strlen(sta));// command
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
    int filenameSize;
    recv(sock, &filenameSize, sizeof(filenameSize), 0);

    char filename[BUFFSIZE];
    ssize_t nameLength = recv(sock, filename, filenameSize, 0);
    filename[nameLength] = '\0'; // Ensure null-terminated string
    fprintf(stderr, "filename: %s\n", filename);

    size_t fileSize;
    recv(sock, &fileSize, sizeof(size_t), 0);

    int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666); // Change flags as needed
    if (file == -1) {
        perror("Error opening file for writing");
        return;
    }

    char buffer[BUFFSIZE];
    ssize_t bytesReceived;

    if ((bytesReceived = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        ssize_t bytesWritten = write(file, buffer, bytesReceived);
        if (bytesWritten == -1) {
            fprintf(stderr, "Error writing to file");
            close(file);
            return;
        }
    }

    if (bytesReceived == -1) {
        fprintf(stderr, "Error receiving file content");
    }

    close(file);
    printf("File '%s' received successfully.\n", filename);
}

void sendFile(int clientSock) {
    int length;
    char filename[BUFFSIZE];
    ssize_t bytesRead;
    
    recv(clientSock, &length, sizeof(length), 0);
    recv(clientSock, filename, length, 0);
    filename[length] = '\0';
    fprintf(stderr, "%s\n", filename);

    off_t fileSize = 0;
    char buffer[BUFFSIZE];

    int file = open(filename, O_RDONLY);
    if (file == -1) {
        fprintf(stderr, "Error opening file for reading");
        send(clientSock, &fileSize, sizeof(off_t), 0);
        send(clientSock, buffer, 0, 0); // Sending an empty buffer
    } else {
        fileSize = lseek(file, 0, SEEK_END);
        lseek(file, 0, SEEK_SET);
        send(clientSock, &fileSize, sizeof(off_t), 0);
    
        if ((bytesRead = read(file, buffer, fileSize)) > 0) {
            send(clientSock, buffer, bytesRead, 0);
        }

        close(file);
        printf("File '%s' sent successfully.", filename);
    }
}

void handleServerActions(int sock) {
    int timeout = 10;
    while (1) {
        char response[1];
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
            case sendfile://sendfile primeste de la server
                receiveFile(sock);
                break;
            case getFile://getFile trimite la server
                sendFile(sock);
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