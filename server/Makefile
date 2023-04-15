CC=$(CROSS_COMPILE)gcc

override CFLAGS+=-Wall

all: aesdsocket 

writer : aesdsocket.o
		$(CC) -g -o aesdsocket aesdsocket.o

writer.o : aesdsocket.c
		$(CC) $(CFLAGS) -g -c aesdsocket.c

clean : 
		-rm aesdsocket aesdsocket.o
