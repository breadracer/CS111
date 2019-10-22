# NAME: Shawn Ma
# EMAIL: breadracer@outlook.com
# ID: 204996814

CC = gcc
CFLAGS = -g -Wall -Wextra -pthread

default: all

all: add list

add:
	@$(CC) $(CFLAGS) lab2_add.c -o lab2_add

list:
	@$(CC) $(CFLAGS) lab2_list.c SortedList.c -o lab2_list

tests: all
	@chmod +x test.sh
	@./test.sh 2> /dev/null

graphs:
	@chmod +x lab2_add.gp
	@chmod +x lab2_list.gp
	@gnuplot lab2_add.gp
	@gnuplot lab2_list.gp

dist: default tests graphs
	@tar -czf lab2a-204996814.tar.gz lab2_add.c lab2_list.c SortedList.h SortedList.c \
	Makefile README test.sh lab2_add-1.png lab2_add-2.png lab2_add-3.png \
	lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png \
	lab2_list-4.png lab2_add.csv lab2_list.csv lab2_add.gp lab2_list.gp

clean:
	@rm -f lab2_add lab2_list *.gz
