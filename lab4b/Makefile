CC = gcc
CFLAGS = -g -Wall -Wextra -lmraa -lm

default: lab4b

lab4b:
	@$(CC) $(CFLAGS) main.c -o lab4b

dist: lab4b
	@tar -czf lab4b-204996814.tar.gz main.c Makefile README test.sh

check: lab4b
	@chmod +x test.sh
	@./test.sh

clean:
	@rm -f lab4b *.gz
