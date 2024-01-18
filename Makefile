CC = g++
CFLAGS = -g

all: clientsocket myqueue serversocket

clientsocket: clientsocket.c
	$(CC) $(CFLAGS) clientsocket.c -o clientsocket
serversocket: serversocket.c myqueue
	$(CC) $(CFLAGS) -pthread serversocket.c myqueue -o serversocket
myqueue: myqueue.c myqueue.h
	$(CC) $(CFLAGS) -c myqueue.c -o myqueue

clean:
	rm -f clientsocket.o serversocket.o

