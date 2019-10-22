CC = gcc
CFLAGS = -g -Wall -Wextra

default: lab3a

lab3a:
	@$(CC) $(CFLAGS) main.c -o lab3a

dist: lab3a
	@tar -czf lab3a-204996814.tar.gz ext2_fs.h main.c Makefile README


clean:
	@rm -f lab3a *.gz
