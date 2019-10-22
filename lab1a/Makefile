# NAME: Shawn Ma
# EMAIL: breadracer@outlook.com
# ID: 204996814

CC = gcc
CFLAGS = -g -Wall -Wextra

default: all

all:
	@$(CC) $(CFLAGS) main.c -o simpsh

check: clean all
	@echo "hello" > input.txt
	@touch output.txt
	@touch error.txt
	@./simpsh --rdonly input.txt --wronly output.txt
	@if [ $$? -ne 0 ]; then \
		echo "test case 2 failed\n"; \
	fi
	@printf "" > output.txt
	@printf "" > error.txt
	@./simpsh --rdonly input.txt --wronly output.txt --wronly error.txt \
	--command 0 1 2 cat
	@if [ ! -s output.txt ] && [ $$? -ne 0 ]; then \
		echo "test case 1 failed\n"; \
	fi
	@printf "" > output.txt
	@printf "" > error.txt
	@./simpsh --verbose --rdonly input.txt --wronly output.txt --wronly error.txt \
	--command 0 1 2 cat > verbose.txt
	@if [ ! -s output.txt ] && [ ! -s verbose.txt ] && [ $$? -ne 0 ]; then \
		echo "test case 3 failed\n"; \
	fi
	@echo "All tests passed"
	@rm -f *.txt

dist:
	@tar -czf lab1-204996814.tar.gz Makefile main.c README

clean:
	@rm -f simpsh *.gz
