CC = gcc
CFLAGS = -g -Wall -Wextra -lmraa -lm

default: all

all: lab4c_tcp lab4c_tls

lab4c_tcp:
	@$(CC) $(CFLAGS) lab4c_tcp.c -o lab4c_tcp

lab4c_tls:
	@$(CC) $(CFLAGS) -lssl -lcrypto lab4c_tls.c -o lab4c_tls

dist: all
	@tar -czf lab4c-204996814.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README

clean:
	@rm -f lab4c_tcp lab4c_tls *.gz
