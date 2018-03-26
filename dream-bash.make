CC=gcc
CFLAGS=-fsanitize=address -g

dream-archiver: dream-bash.c
	$(CC) -o dream-bash dream-bash.c $(CFLAGS)
clean: 
	-rm dream-bash
