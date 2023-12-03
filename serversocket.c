#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define buffersize 1024


typedef struct {
    char ip[INET_ADDRSTRLEN];
    int client_id;
    pid_t pid;
} ConnectedClient;

void sig_fork(int signo) {
    int stat;
    while (waitpid(-1, &stat, WNOHANG) > 0) {
        // așteaptă toate procesele copil
    }
}

void asteptare(){
    char option;
    while (option != 'y' || option != 'Y'){
        printf("Doriti sa va intoarceti la meniul principal? (y): ");
        scanf(" %c", &option);
        if (option == 'y' || option == 'Y') break;
    }
}

void Hostname(int clnt_sock) {
    if (send(clnt_sock, "1", buffersize, 0) == -1) {//trimit 1 ca e prima optiune din meniu pt client
        perror("Eroare la trimiterea opțiunii '1'");
        exit(EXIT_FAILURE);
    }
    printf("Se așteaptă hostname ...\n");
    char buffer[buffersize];
    if (recv(clnt_sock, buffer, buffersize, 0) == -1) {
        perror("Eroare la primirea numelui de host");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Hostname: %s\n", buffer);
    asteptare();
}

void ProcessList(int clnt_sock) {
    send(clnt_sock, "2", buffersize, 0); // trimit 2 ca e a doua opțiune din meniu pt client
    printf("Se asteapta lista de procese ...\n");

    // Primeste dimensiunea buffer-ului
    size_t dimFisier;
    recv(clnt_sock, &dimFisier, sizeof(size_t), 0);
    char buffer[dimFisier];

    memset(buffer, 0, sizeof(buffer)); // Inițializare buffer cu 0
    ssize_t bytesReceived = recv(clnt_sock, buffer, dimFisier, 0);

    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        fprintf(stderr, "%s\n", buffer);
    } else {
        fprintf(stderr, "Eroare la primirea listei de procese.\n");
    }

    asteptare();
}

void ExecuteCommand(int clnt_sock) {
    //Aici client-ul trebuie sa astepte 2 trimiteri consecutive adica cand citeste optiunea 1/2/3/4 si dupa mai asteapta comanda de la admin
    send(clnt_sock, "3", buffersize, 0);//trimit 3 ca e a doua optiune din meniu pt client
    printf("Introduceti comanda de executat: ");
    char command[buffersize];
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    fgets(command, sizeof(command), stdin);
    send(clnt_sock, command, buffersize, 0);
    printf("Se asteapta executia comenzii ...\n");

    size_t dimFisier;
    recv(clnt_sock, &dimFisier, sizeof(size_t), 0);
    char buffer[dimFisier];

    // Primeste raspunsul de la client
    ssize_t bytesReceived = recv(clnt_sock, buffer, dimFisier, 0);
    write(STDOUT_FILENO, buffer, bytesReceived);

    asteptare();
}

int rebootPC(int clnt_sock) {
    send(clnt_sock, "4", buffersize, 0);
    int shutdown_code;
    recv(clnt_sock, &shutdown_code, sizeof(shutdown_code), 0);

    if (shutdown_code != 0){
        printf("Eroare la oprirea PC-ului.\n");
        return -1;
    }

    printf("PC-ul a fost oprit cu succes.\n");
    return 0;
}

void Whitelist(){
    // Afiseaza lista de IP-uri din blacklist
    printf("Lista de IP-uri din blacklist:\n");
    FILE* file = fopen("blacklist", "r");
    if (file != NULL) {
        char ip[INET_ADDRSTRLEN];
        while (fscanf(file, "%s", ip) != EOF)
            printf("- %s\n", ip);
        fclose(file);
    }
    // Ia input de la administrator pentru IP-ul sters
    printf("\nIntroduceti IP-ul pentru whitelist: ");
    char removeIP[INET_ADDRSTRLEN];
    scanf("%s", removeIP);

    FILE* blacklist_file = fopen("blacklist", "r");
    FILE* temp_file = fopen("temp_blacklist", "w+");

    if (blacklist_file != NULL && temp_file != NULL) {
        char ip[INET_ADDRSTRLEN];
        while (fscanf(blacklist_file, "%s", ip) != EOF) {
            if (strcmp(ip, removeIP) != 0) {
                fprintf(temp_file, "%s\n", ip);
            }
        }

        fclose(blacklist_file);
        fclose(temp_file);

        // sterge vechea lista de blacklist si redenumește temp_blacklist
        remove("blacklist");
        rename("temp_blacklist", "blacklist");
    }
}

void print_menu()
{
    printf("\nMeniu:\n");
    printf("0. Iesire\n");
    printf("1. Afiseaza numele PC-ului\n");
    printf("2. Afiseaza lista de procese\n");
    printf("3. Executa comanda\n");
    printf("4. Opreste PC-ul\n");
    printf("5. Afiseaza clientii conectati\n");
    printf("6. Adauga in blacklist\n");
    printf("7. Whitelisting\n");
    printf("----------------------------------------------------------\n");
	printf("Introduceti optiunea dorita:");
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sig_fork;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Eroare la setarea handler-ului pentru SIGCHLD");
        exit(EXIT_FAILURE);
    }

    ConnectedClient connected_clients[100];
    int num_connected_clients = 0;
    int client_id_counter = 1;

    struct timeval timeout;
    timeout.tv_sec = 15; // sec
    timeout.tv_usec = 0; // ms

    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock == -1) {
        perror("Eroare la crearea socket-ului");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(5566);

    int reuse = 1;
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("Eroare la setarea optiunii SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }

    if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("Eroare la bind");
        exit(EXIT_FAILURE);
    }


    listen(serv_sock, 20);

    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    while (1) {
        int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        if (clnt_sock == -1) {
            perror("Eroare la acceptarea conexiunii");
            exit(0);
        }
        
        // Obține adresa IP a clientului
        connected_clients[num_connected_clients].client_id = client_id_counter++;
        inet_ntop(AF_INET, &(clnt_addr.sin_addr), connected_clients[num_connected_clients].ip, INET_ADDRSTRLEN);
        int ok = 0;
        FILE *file = fopen("blacklist", "r");
        if (file != NULL) {
            char line[256];
            while (fgets(line, sizeof(line), file) != NULL) {
                line[strcspn(line, "\n")] = '\0';
                if (strcmp(line, connected_clients[num_connected_clients].ip) == 0) {
                    ok = 1;
                    break;
                }
            }
            fclose(file);
        }
        if(ok == 0){
            // Afiseaza adresa IP a clientului
            printf("Client connected from IP: %s\n", connected_clients[num_connected_clients].ip);

            // set recv and send timeout
            if (setsockopt(clnt_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
                perror("setsockopt for receive timeout failed");
                exit(EXIT_FAILURE);
            }
            if (setsockopt(clnt_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
                perror("setsockopt for send timeout failed");
                exit(EXIT_FAILURE);
            }

            connected_clients[num_connected_clients].pid = fork();
            if (connected_clients[num_connected_clients].pid == -1) {
                printf("ERROR pid\n");
            } else if (connected_clients[num_connected_clients].pid == 0) {
                char option = 0;
                do
                {
                    print_menu();
                    option = getc(stdin);
                    printf("\n\n");

                    
                    switch (option)
                    {
                        case '0':
                            printf("Iesire din program.\n");
                            for (int i = 0; i <= num_connected_clients; ++i)
                                kill(connected_clients[i].pid, SIGTERM);
                            return 0;
                            break;

                        case '1':
                            Hostname(clnt_sock);
                            break;

                        case '2':
                            ProcessList(clnt_sock);
                            break;

                        case '3':
                            ExecuteCommand(clnt_sock);
                            break;

                        case '4':
                            if(rebootPC(clnt_sock) != 0) break;
                            else exit(0);
                            break;

                        case '5':
                            printf("Lista clientilor conectati:\n");
                            for (int i = 0; i <= num_connected_clients; ++i)
                                printf("%d. IP: %s\n", connected_clients[i].client_id, connected_clients[i].ip);
                            asteptare();
                            break;

                        case '6':
                            printf("ATENTIE: Sigur doriti să adaugati IP-ul în blacklist? (y/n): ");
                            char confirmation;
                            if (scanf(" %c", &confirmation) != 1 || (confirmation != 'y' && confirmation != 'Y')) {
                                printf("Optiune invalida. IP-ul NU a fost adaugat în blacklist.\n");
                            } else {
                                if (confirmation == 'y' || confirmation == 'Y') {
                                    FILE *file = fopen("blacklist", "r");
                                    int ipAlreadyExists = 0;
                                    if (file != NULL) {
                                        char line[256];
                                        while (fgets(line, sizeof(line), file) != NULL) {
                                            line[strcspn(line, "\n")] = '\0';
                                            //printf("%s\n", line);
                                            if (strcmp(line, connected_clients[num_connected_clients].ip) == 0) {
                                                ipAlreadyExists = 1;
                                                printf("IP-ul exista deja in blacklist.\n");
                                                break;
                                            }
                                        }
                                        fclose(file);
                                    }
                                    file = fopen("blacklist", "a");

                                    if (!ipAlreadyExists) {
                                        if (file != NULL) {
                                            fprintf(file, "%s\n", connected_clients[num_connected_clients].ip);
                                            printf("IP-ul a fost adaugat în blacklist.\n");
                                        } else {
                                            perror("Eroare la deschiderea fișierului pentru scriere");
                                        }
                                    }
                                    fclose(file);
                                } else {
                                    printf("IP-ul NU a fost adăugat în blacklist.\n");
                                }
                            }

                            asteptare();
                            break;

                        case '7':
                            Whitelist();
                            break;

                        default:
                            break;
                    }
                    system("clear"); 
                } while (option != 0);

                close(clnt_sock);
                //free(buffer);
                exit(0);
            } else {
                close(clnt_sock);
            }

            num_connected_clients++;
        } else {
            fprintf(stderr, "Utilizatorul este in blacklist si conexiunea se va incheia!\n");
            close(clnt_sock);
        }
    }
    close(serv_sock);
    return 0;
}
