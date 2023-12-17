CC = g++
CFLAGS = -g

all: clientsocket myqueue serversocket

clientsocket: clientsocket.c
	$(CC) $(CFLAGS) clientsocket.c -o clientsocket.o
serversocket: serversocket.c myqueue.o
	$(CC) $(CFLAGS) serversocket.c myqueue.o -o serversocket.o
myqueue: myqueue.c myqueue.h
	$(CC) $(CFLAGS) -c myqueue.c -o myqueue.o

clean:
	rm -f clientsocket.o serversocket.o

