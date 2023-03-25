C=gcc
all:
	rm -rf *.o writer
	aarch64-none-linux-gnu-gcc writer.c -o writer

writer:
	$(CROSS_COMPILE)$(C) writer.c -o writer
	
clean:
	rm -rf *.o writer

