all: aesdsocket
LDFLAGS ?=  -pthread -lrt
CCFLAGS ?=  -Wall -g -c
EXTRA_CFLAGS = -DUSE_AESD_CHAR_DEVICE=1
	
aesdsocket.o: aesdsocket.c
		                        $(CC) $(LDFLAGS) $(CCFLAGS) $(EXTRA_CFLAGS)  -c aesdsocket.c

aesdsocket: aesdsocket.o
		                        $(CC) $(LDFLAGS) aesdsocket.o -o aesdsocket

clean :
		                        rm -f aesdsocket.o aesdsocket

