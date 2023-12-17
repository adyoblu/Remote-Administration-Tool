#ifndef MYQUEUE_H_
#define MYQUEUE_H_
#include <stdlib.h>
#include <arpa/inet.h>

struct node {
    char *ip;
    struct node* next;
    int* client_socket;
};
typedef struct node ConnectedClient;
void afisareLista();
int alegeLista();
void enqueue(int *client_socket, const char *ip);
int verifica_clienti();
int* dequeue();
#endif