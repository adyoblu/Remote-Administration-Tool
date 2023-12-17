#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define BUFFSIZE 4096

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

void sendHostname(int sock)
{
    char message[BUFFSIZE];
    char hostname[BUFFSIZE];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        fprintf(stderr, "%s", hostname);
        send(sock, hostname, strlen(hostname), 0);
    } else 
        perror("Eroare la obtinerea numarului PC-ului");
}

void sendProcessList(int sock) {
    //pun output-ul in fisier
    FILE *fp = popen("ps aux > output", "r");
    if (fp == NULL) {
        perror("Eroare la obtinerea listei de procese");
        return;
    }
    fclose(fp);
    //deschide in binar pt a calcula fiecare bit
    FILE *file = fopen("output", "rb");
    if (file == NULL) {
        perror("Eroare la deschiderea fisierului output");
        return;
    }
    //calculeaza dimensiunea
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    //citesc in buffer
    char *buffer = (char *)malloc(fileSize + 1);
    fread(buffer, 1, fileSize, file);
    fclose(file);
    buffer[fileSize] = '\0';

    fileSize += 1;
    // Trimite dimensiunea buffer-ului catre server
    send(sock, &fileSize, sizeof(size_t), 0);
    //trimite fisierul la server
    send(sock, buffer, fileSize-1, 0);
    free(buffer);
    remove("output");
}

void receiveAndExecuteCommand(int sock)
{   
    //fork exec si redirectare prin pipe-uri
    char command[BUFFSIZE];
    ssize_t bytesRead = recv(sock, command, sizeof(command), 0);

    printf("Comanda primita de la server: %s\n", command);
    command[strcspn(command, "\n")] = ' ';//sterge /n de la final
    strcat(command, " > output");
    command[sizeof(command) - 1] = '\0';
    printf("Comanda primita de la server: %s\n", command);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("Eroare la obtinerea listei de procese");
        return;
    }
    fclose(fp);
    //deschide in binar pt a calcula fiecare bit
    FILE *file = fopen("output", "rb");
    if (file == NULL) {
        perror("Eroare la deschiderea fisierului output");
        return;
    }
    //calculeaza dimensiunea
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    //if(fileSize != 0){
    //citesc in buffer
    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL) {
        perror("Eroare la alocarea memoriei pentru buffer");
        fclose(file);
        return;
    }
    fread(buffer, 1, fileSize, file);
    fclose(file);
    buffer[fileSize] = '\0';

    fileSize += 1;
    // Trimite dimensiunea buffer-ului catre server
    send(sock, &fileSize, sizeof(size_t), 0);
    //trimite fisierul la server
    send(sock, buffer, fileSize-1, 0);
    free(buffer);
    remove("output");
    // } else if(fileSize == 0){
    //     send(sock, "0", 1, 0);
    // }
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
        char response[BUFFSIZE];
        recv(sock, response, sizeof(response), 0);
        //printf("\n%s\n", response);
        int responseType = atoi(response);

        switch (responseType) {
            case 1:
                sendHostname(sock);
                break;
            case 2:
                sendProcessList(sock);
                break;
            case 3:
                receiveAndExecuteCommand(sock);
                break;
            case 4:
                restartClient(sock);
                break;
            case 5:
                fprintf(stderr, "S-a terminat executia clientului.");
                exit(0);
                break;
            case 8:
                selectFile(sock);
                break;
            case 9:
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