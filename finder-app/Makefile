CC=$(CROSS_COMPILE)gcc

override CFLAGS+=-Wall

all: writer

writer : writer.o
		$(CC) -o writer writer.o

writer.o : writer.c
		$(CC) $(CFLAGS) -c writer.c

clean : 
		-rm writer writer.o
