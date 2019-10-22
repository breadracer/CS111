# NAME: Shawn Ma
# EMAIL: breadracer@outlook.com
# ID: 204996814

CC = gcc
CFLAGS = -g -Wall -Wextra

default: all

all:
	@$(CC) $(CFLAGS) main.c -o simpsh

check: clean all
	@chmod +x test.sh
	@./test.sh

dist:
	@tar -czf lab1-204996814.tar.gz Makefile main.c README report.pdf test.sh

clean:
	@rm -f simpsh *.gz
