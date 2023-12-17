#include "myqueue.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

ConnectedClient* head = NULL;
ConnectedClient* tail = NULL;

void enqueue(int *client_socket, const char *ip){
	ConnectedClient *newnode = (ConnectedClient*)malloc(sizeof(ConnectedClient));
	newnode->client_socket = client_socket;
	newnode->ip = strdup(ip);
	//printf("IP:%s\n", ip);
	newnode->next = NULL;
	if(tail == NULL)
		head = newnode;
	else
		tail->next = newnode;
	tail = newnode;
}

int* dequeue(){
	if(head == NULL)
		return NULL;
	else {
		int *result = head->client_socket;
		ConnectedClient *temp = head;
		head = head->next;
		if(head == NULL) tail = NULL;
		free(temp->ip);
		free(temp);
		return result;
	}
}

int alegeLista() {
    ConnectedClient *x = head;
	int i;
	fprintf(stderr, "Lista clientilor conectati:\n");
    for (i = 0; x != NULL; fprintf(stderr, "%d. IP: %s\n", i, x->ip), x = x->next, i++) {}
	char buffer[256];
	int option;
    do {
        fprintf(stderr, "Alege un client: ");
        scanf(" %d", &option);
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    } while (option < 0 || option >= i);

    x = head;
    for (i = 0; i < option && x != NULL; i++)
        x = x->next;
    if(*(x->client_socket) > 0)
		return *(x->client_socket);
	return -1;
}

void afisareLista(){
	ConnectedClient *x = head;
	fprintf(stderr, "Lista clientilor conectati:\n");
    for (int i = 0; x != NULL; fprintf(stderr, "%d. IP: %s\n", i, x->ip), x = x->next, i++) {}
}

int verifica_clienti() {
    ConnectedClient *current = head;
    int numar_clienti = 0;

    while (current != NULL) {
        numar_clienti++;
        current = current->next;
    }
	return numar_clienti;
}