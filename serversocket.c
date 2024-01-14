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

int verify_blacklist(const char *ip){
    int ok = 0;
    FILE *file = fopen("blacklist", "r");
    if (file != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), file) != NULL) {
            line[strcspn(line, "\n")] = '\0';
            if (strcmp(line, ip) == 0) {
                ok = 1;
                break;
            }
        }
        fclose(file);
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

    while ((bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0)) > 0) {
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

void sendFile(int clientSock, const char *filename, const char *name) {//trimit fisier de pe server la client
    if (send(clientSock, "8", 1, 0) == -1) {
        fprintf(stderr, "Eroare la trimiterea optiunii 8");
        return;
    }
    
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Eroare la deschiderea fisierului");
        return;
    }
    
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    int x = strlen(filename);
    send(clientSock, &x, sizeof(x), 0);//trimit dimensiune nume
    send(clientSock, name, strlen(filename), 0);//trimit nume
    send(clientSock, &fileSize, sizeof(size_t), 0);//trimit dimensiune fisier

    char buffer[BUFFSIZE];
    size_t bytesRead;//trimit date fisier
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(clientSock, buffer, bytesRead, 0);
    }

    fclose(file);

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

void Whitelist(){
    // Afiseaza lista de IP-uri din blacklist
    printf("Lista de IP-uri din blacklist:\n");
    FILE* file = fopen("blacklist", "r");
    fseek(file, 0, SEEK_END);

    if (ftell(file) == 0) {
        fprintf(stderr, "Fisierul este gol\n");
        fclose(file);
        return;
    }
    fseek(file, 0, SEEK_SET);
    if (file != NULL) {
        char ip[INET_ADDRSTRLEN];
        while (fscanf(file, "%s", ip) != EOF)
            printf("%s\n", ip);
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

void Blacklist(int clt_sck){
    printf("ATENTIE: Sigur doriti să adaugati IP-ul în blacklist? (y/n): ");
    char confirmation;
    SA_IN peer_addr;
    socklen_t peer_len = sizeof(peer_addr);
    getpeername(clt_sck, (struct sockaddr*)&peer_addr, &peer_len);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer_addr.sin_addr, ip, INET_ADDRSTRLEN);

    fprintf(stderr, "%s\n", ip);
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
                    if (strcmp(line, ip) == 0) {
                        ipAlreadyExists = 1;
                        printf("IP-ul exista deja in blacklist.\n");
                        exit(0);
                        break;
                    }
                }
                fclose(file);
            }
            file = fopen("blacklist", "a");

            if (!ipAlreadyExists) {
                if (file != NULL) {
                    fprintf(file, "%s\n", ip);
                    printf("IP-ul a fost adaugat in blacklist.\n");
                } else {
                    perror("Eroare la deschiderea fisierului pentru scriere");
                }
            }
            fclose(file);
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