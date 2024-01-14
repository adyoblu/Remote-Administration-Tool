#include "utilities.c"
#include "myqueue.h"
/*
trb date comenzi cu sudo
pt mkdir sau alte comenzi se blocheaza execute command
open in loc de FILE*
organizare functii in mai multe fisiere
*/


typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;
int admin_menu_active = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t admin_menu_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_t admin_menu_thread_id;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;

int verify_blacklist(const char *ip) {
    int ok = 0;
    int file = open("blacklist", O_RDONLY);
    
    if (file != -1) {
        char line[256];
        ssize_t bytesRead;
        
        while ((bytesRead = read(file, line, sizeof(line)-1)) > 0) {
            line[bytesRead] = '\0';
            if (strcmp(line, ip) == 0) {
                ok = 1;
                break;
            }
        }
        
        close(file);
    }
    
    return ok;
}


void print_menu()
{   
    printf("\n### Meniu Administrator ###\n");
    printf("0. Iesire\n");
    printf("1. Afiseaza hostname pentru un pc\n");
    printf("2. Afiseaza lista de procese ale unui utilizator\n");
    printf("3. Executa comanda pentru un utilizator\n");
    printf("4. Reporneste PC-ul unui utilizator\n");
    printf("5. Kick client\n");
    printf("6. Afiseaza toti clientii conectati\n");
    printf("7. Blacklist/Whitelist\n");
    printf("8. Ia un fisier\n");
    printf("9. Trimite un fisier\n");
    printf("----------------------------------------------------------\n");
	printf("Introduceti optiunea dorita:\n");
    fflush(stdout);
}

int check(int exp, const char *msg){
	if(exp == SOCKETERROR){
		perror(msg);
		exit(1);
	}
	return exp;
}

void asteptare(){
    char option;
    do {
        printf("Doriti sa va intoarceti la meniul principal? (y): ");
        scanf(" %c", &option);
        fflush(stdin);
    } while (option != 'y' && option != 'Y');
}

void Hostname(int clnt_sock) {
    // Send the option "1" as the first menu option
    send(clnt_sock, "1", 1, 0);
    printf("Waiting for hostname...\n");

    // Receive the length of the hostname
    size_t hostnameLength;
    if (recv(clnt_sock, &hostnameLength, sizeof(hostnameLength), 0) == -1) {
        perror("Error receiving hostname length");
        exit(EXIT_FAILURE);
    }

    hostnameLength = ntohl(hostnameLength);

    char buffer[BUFFSIZE];
    recv(clnt_sock, buffer, hostnameLength, 0);
    buffer[hostnameLength] = '\0';
    fprintf(stderr, "Hostname: %s\n", buffer);
    asteptare();
}

void receiveFile(int clientSock, const char *filename,const char* name) {
    send(clientSock, "9", 1, 0);

    int length = strlen(filename);
    send(clientSock, &length, sizeof(length), 0);//trimit dimensiunea filename
    send(clientSock, filename, length, 0);//trimit filename
    
    int file = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);//creez fisier cu numele propus de admin

    off_t fileSize;
    recv(clientSock, &fileSize, sizeof(off_t), 0);//primesc dimensiune fisier de la client

    char buffer[BUFFSIZE];
    ssize_t bytesReceived;

    if ((bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0)) > 0) {
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
    fprintf(stderr, "Fisierul '%s' a fost primit si salvat cu succes.\n", filename);
    asteptare();
}

void sendFile(int clientSock, const char *filename, const char *name) {
    if (send(clientSock, "8", 1, 0) == -1) {
        fprintf(stderr, "Eroare la trimiterea optiunii 8");
        return;
    }

    int file = open(filename, O_RDONLY);
    if (file == -1) {
        perror("Eroare la deschiderea fisierului");
        return;
    }

    off_t fileSize = lseek(file, 0, SEEK_END);
    lseek(file, 0, SEEK_SET);

    int nameLength = strlen(name);
    send(clientSock, &nameLength, sizeof(nameLength), 0);
    send(clientSock, name, nameLength, 0);
    send(clientSock, &fileSize, sizeof(off_t), 0);

    char buffer[BUFFSIZE];
    ssize_t bytesRead;

    if ((bytesRead = read(file, buffer, sizeof(buffer))) > 0) {
        if (send(clientSock, buffer, bytesRead, 0) == -1) {
            perror("Eroare la trimiterea datelor fisierului");
            close(file);
            return;
        }
    }

    close(file);

    printf("Fisierul '%s' a fost trimis cu succes.\n", filename);
    asteptare();
}

void getProcessesList(int clnt_sock) {
    send(clnt_sock, "2", 1, 0); // trimit 2 ca e a doua opțiune din meniu pt client
    printf("Se asteapta lista de procese ...\n");

    // Primeste dimensiunea buffer-ului
    size_t dimFisier;
    recv(clnt_sock, &dimFisier, sizeof(size_t), 0);
    char buffer[dimFisier];

    memset(buffer, 0, sizeof(buffer));
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
    send(clnt_sock, "3", 1, 0);//trimit 3 ca e a doua optiune din meniu pt client
    printf("Introduceti comanda de executat: ");
    char command[BUFFSIZE];
    int c;
    fgets(command, sizeof(command), stdin);
    send(clnt_sock, command, BUFFSIZE, 0);
    printf("Se asteapta executia comenzii ...\n");
    char result[BUFFSIZE];
    size_t lenght;
    recv(clnt_sock, &lenght, sizeof(lenght), 0);
    ssize_t bytesRead = recv(clnt_sock, result, lenght, 0);
    result[bytesRead+1] = '\0';
    printf("\n%s\n", result);
    asteptare();
}

int rebootPC(int clnt_sock) {
    send(clnt_sock, "4", 1, 0);
    int shutdown_code;
    recv(clnt_sock, &shutdown_code, sizeof(shutdown_code), 0);

    if (shutdown_code != 0){
        printf("Eroare la oprirea PC-ului.\n");
        return -1;
    }

    printf("PC-ul a fost oprit cu succes.\n");
    return 0;
}

void Whitelist() {
    // Afiseaza lista de IP-uri din blacklist
    printf("Lista de IP-uri din blacklist:\n");
    
    int file = open("blacklist", O_RDONLY);
    lseek(file, 0, SEEK_END);

    if (lseek(file, 0, SEEK_CUR) == 0) {
        fprintf(stderr, "Fisierul este gol\n");
        close(file);
        return;
    }
    
    lseek(file, 0, SEEK_SET);

    char ip[INET_ADDRSTRLEN];
    char buffer[BUFFSIZE];
    ssize_t bytesRead;

    while ((bytesRead = read(file, buffer, sizeof(buffer))) > 0) {
        printf("%.*s", (int)bytesRead, buffer);
    }

    close(file);

    // Ia input de la administrator pentru IP-ul sters
    printf("\nIntroduceti IP-ul pentru whitelist: ");
    char removeIP[INET_ADDRSTRLEN];
    scanf("%s", removeIP);

    int blacklist_file = open("blacklist", O_RDONLY);
    int temp_file = open("temp_blacklist", O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (blacklist_file != -1 && temp_file != -1) {
        char ip[INET_ADDRSTRLEN];
        char buffer[BUFFSIZE];
        ssize_t bytesRead;

        while ((bytesRead = read(blacklist_file, buffer, sizeof(buffer))) > 0) {
            char *pos = buffer;
            char *end = buffer + bytesRead;

            while (pos < end) {
                sscanf(pos, "%s", ip);
                pos += strlen(ip) + 1;

                if (strcmp(ip, removeIP) != 0) {
                    dprintf(temp_file, "%s\n", ip);
                }
            }
        }

        close(blacklist_file);
        close(temp_file);

        // sterge vechea lista de blacklist si redenumește temp_blacklist
        remove("blacklist");
        rename("temp_blacklist", "blacklist");
    }
}

void *thread_func(void *arg) {
	while(1){
		int* pclient;
		pthread_mutex_lock(&mutex);
		pthread_cond_wait(&condition_var, &mutex);
		if((pclient = dequeue()) == NULL){
			pthread_cond_wait(&condition_var, &mutex);
            pclient = dequeue();
		}
		pthread_mutex_unlock(&mutex);
		// if(pclient != NULL){
		// 	handle_connection(pclient);
		// }
	}
}

void Blacklist(int clt_sck) {
    printf("ATENTIE: Sigur doriti să adaugati IP-ul în blacklist? (y/n): ");
    char confirmation;
    struct sockaddr_in peer_addr;
    socklen_t peer_len = sizeof(peer_addr);
    getpeername(clt_sck, (struct sockaddr*)&peer_addr, &peer_len);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer_addr.sin_addr, ip, INET_ADDRSTRLEN);

    fprintf(stderr, "%s\n", ip);
    if (scanf(" %c", &confirmation) != 1 || (confirmation != 'y' && confirmation != 'Y')) {
        printf("Optiune invalida. IP-ul NU a fost adaugat în blacklist.\n");
    } else {
        if (confirmation == 'y' || confirmation == 'Y') {
            int file = open("blacklist", O_RDONLY);
            int ipAlreadyExists = 0;
            if (file != -1) {
                char line[256];
                ssize_t bytesRead;
                while ((bytesRead = read(file, line, sizeof(line))) > 0) {
                    line[bytesRead] = '\0';
                    if (strcmp(line, ip) == 0) {
                        ipAlreadyExists = 1;
                        printf("IP-ul exista deja in blacklist.\n");
                        exit(0);
                        break;
                    }
                }
                close(file);
            }
            file = open("blacklist", O_WRONLY | O_CREAT | O_APPEND, 0666);

            if (!ipAlreadyExists) {
                if (file != -1) {
                    dprintf(file, "%s\n", ip);
                    printf("IP-ul a fost adaugat in blacklist.\n");
                } else {
                    perror("Eroare la deschiderea fisierului pentru scriere");
                }
            }
            close(file);
        } else {
            printf("IP-ul NU a fost adaugat în blacklist.\n");
        }
    }
}

void* admin_menu_thread(void* arg) {
    int clt_sck;
    int ok = 0;
    while(1){
        char option = 0, opt = 0;
        do
        {
            pthread_mutex_lock(&admin_menu_mutex);
            if (admin_menu_active == 1){
                print_menu();
                option = getc(stdin);
                printf("\n\n");
                int i = option - '0';
                switch (i)
                {
                    case getHostname:
                        clt_sck = alegeLista();
                        Hostname(clt_sck);
                        break;

                    case getProcesses:
                        clt_sck = alegeLista();
                        getProcessesList(clt_sck);
                        break;

                    case executeCommand:
                        clt_sck = alegeLista();
                        ExecuteCommand(clt_sck);
                        while (getchar() != '\n');
                        break;

                    case reboot:
                        clt_sck = alegeLista();
                        rebootPC(clt_sck);
                        break;

                    case kick:
                        clt_sck = alegeLista();
                        send(clt_sck, "5", 1, 0);
                        pthread_cond_signal(&condition_var);
                        break;

                    case afisareClienti:
                        afisareLista();
                        asteptare();
                        break;
                    case whitelistBlacklist:
                        fprintf(stderr, "1. WhiteList\n2. BlackList\nIntrodu o optiune:\n");
                        getchar();
                        opt = getc(stdin);
                        getchar();
                        printf("\n\n");
                        switch (opt)
                        {
                            case whitelist:
                                Whitelist();
                                break;
                            case blacklist:
                                clt_sck = alegeLista();
                                Blacklist(clt_sck);
                                break;
                        }
                        asteptare();
                        break;
                    case getFile://iau fisier de pe client pe server
                        char buffer[BUFFSIZE];
                        printf("Introdu calea fișierului din cadru clientului: ");
                        scanf("%s",buffer);
                        char buffer1[BUFFSIZE];
                        printf("pune un nume fisierului de copiat: ");
                        scanf("%s",buffer1);
                        clt_sck = alegeLista();
                        receiveFile(clt_sck, buffer, buffer1);
                        break;

                    case sendfile://trimit fisier de pe server la client
                        char buff[BUFFSIZE];
                        printf("Care e calea completa a fisierului? : ");
                        scanf("%s",buffer);
                        char buff1[BUFFSIZE];
                        printf("pune un nume fisierului copiat : ");
                        scanf("%s",buffer1);
                        clt_sck = alegeLista();
                        sendFile(clt_sck,buffer,buffer1);
                        break;

                    case exitServer:
                        printf("Iesire din program.\n");
                        pthread_cond_signal(&condition_var);
                        exit(0);
                        break;
                }
                system("clear");
            }
            if(verifica_clienti() == 0) admin_menu_active = 0;
            pthread_mutex_unlock(&admin_menu_mutex);
        } while (option != 0);
    }
    return NULL;
}

int main(int argc, char **argv) {
    int serv_sock, client_socket, addr_size;
    SA_IN serv_addr, client_addr;

    // Creeaza thread-ul pentru meniul administratorului
    pthread_create(&admin_menu_thread_id, NULL, admin_menu_thread, NULL);

    //creez un nr de thread-uri prestabilite care asteapta conexiuni de la clienti pentru a nu supraincarca sistemul
    for(int i = 0; i < THREAD_POOL_SIZE; ++i)
        pthread_create(&thread_pool[i], NULL, thread_func, NULL);
    //aici creez socket si verific pt erori
    check((serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)), "Fail to create socket");

    //pt adresa si port-ul server-ului
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;//IPv4
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5566);//port

    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//reutilizare
    check(bind(serv_sock, (SA*)&serv_addr, sizeof(serv_addr)), "Bind failed");
    //poate primi pana la 10 de clienti
    check(listen(serv_sock, 10), "Listen failed");

    printf("Waiting for connections...\n");
    while(1){
        addr_size = sizeof(SA_IN);
        check(client_socket = accept(serv_sock, (SA*)&client_addr, (socklen_t*)&addr_size), "accept failed");
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN); 

        if (verify_blacklist(ip) == 1) {
            printf("Clientul cu IP-ul %s este in blacklist. Conexiune inchisa.\n", ip);
            close(client_socket);
        } else {
            int *pclient = (int*)malloc(sizeof(int));
            *pclient = client_socket;
            pthread_mutex_lock(&mutex);
            enqueue(pclient, ip);
            admin_menu_active = 1;
            pthread_mutex_unlock(&mutex);
        }
    }
    return 0;
}