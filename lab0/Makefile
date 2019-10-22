CC = gcc
CFLAGS = -g -Wall -Wextra

default: all

all:
	@$(CC) $(CFLAGS) lab0.c -o lab0

check: clean all
	@echo "hello" > input.txt
	@./lab0 --input input.txt > output.txt
	@if test $$? -ne 0; then \
		echo "Incorrect return value: $$? instead of 0"; \
	fi
	@./lab0 --output output.txt < input.txt
	@if test $$? -ne 0; then \
		echo "Incorrect return value: $$? instead of 0"; \
	fi
	@./lab0 --input input.txt --output output.txt
	@if test $$? -ne 0; then \
		echo "Incorrect return value: $$? instead of 0"; \
	fi
	@./lab0 < input.txt > output.txt
	@if test $$? -ne 0; then \
		echo "Incorrect return value: $$? instead of 0"; \
	fi
	@./lab0 --input false.txt 2> /dev/null || \
	if test $$? -ne 2; then \
		echo "Incorrect return value: $$? instead of 2"; \
	fi
	@chmod -w output.txt
	@./lab0 --output output.txt < input.txt 2> /dev/null || \
	if test $$? -ne 3; then \
		echo "Incorrect return value: $$? instead of 3"; \
	fi
	@chmod +w output.txt
	@./lab0 --output < input.txt 2> /dev/null || \
	if test $$? -ne 1; then \
		echo "Incorrect return value: $$? instead of 1"; \
	fi
	@./lab0 --segfault --catch < input.txt > output.txt 2> /dev/null || \
	if test $$? -ne 4; then \
		echo "Incorrect return value: $$? instead of 4"; \
	fi
	@echo "All tests passed"
	@rm -f lab0 *.txt

dist:
	@tar -czf lab0-204996814.tar.gz Makefile lab0.c README backtrace.png breakpoint.png

clean:
	@rm -f lab0 *.txt *.gz
