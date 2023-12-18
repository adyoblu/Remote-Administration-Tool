#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

#define BUFFSIZE 4096
#define getHostname 1
#define getProcesses 2
#define executeCommand 3
#define reboot 4
#define kick 5
#define sendfile 8
#define getFile 9

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
        send(sock, hostname, strlen(hostname), 0);
    } else 
        perror("Eroare la obtinerea numarului PC-ului");
}

void sendProcessesList(int sock) {
    DIR *dir = opendir("/proc");
    if (dir == NULL) {
        perror("Eroare la deschiderea directorului /proc");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && isdigit(entry->d_name[0])) {
            char path[BUFFSIZE];
            snprintf(path, sizeof(path), "/proc/%.255s/stat", entry->d_name);

            FILE *file = fopen(path, "r");
            if (file != NULL) {
                char processName[BUFFSIZE];
                fscanf(file, "%*d %s", processName);  // Citeste numele procesului

                fclose(file);

                size_t nameLength = strlen(processName) + 1;

                // Trimite dimensiunea numelui procesului către server
                send(sock, &nameLength, sizeof(size_t), 0);
                // Trimite numele procesului la server
                send(sock, processName, nameLength, 0);
            }
        }
    }

    closedir(dir);
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
    // Primește numele fișierului
    char filename[BUFFSIZE];
    ssize_t bytesRead = recv(sock, filename, sizeof(filename), 0);
    if (bytesRead <= 0) {
        perror("Eroare la primirea numelui fisierului");
        return;
    }

    // Primește dimensiunea fișierului
    size_t fileSize;
    recv(sock, &fileSize, sizeof(size_t), 0);

    // Deschide fișierul pentru scriere
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Eroare la deschiderea fisierului pentru scriere");
        return;
    }

    // Primește conținutul fișierului și scrie-l în fișier
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
        char response[2];
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