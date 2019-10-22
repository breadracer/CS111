# NAME: Shawn Ma
# EMAIL: breadracer@outlook.com
# ID: 204996814

CC = gcc
CFLAGS = -g -Wall -Wextra -pthread -lprofiler

default: list

list:
	@$(CC) $(CFLAGS) lab2_list.c SortedList.c -o lab2_list

tests: default
	@chmod +x test.sh
	@./test.sh

graphs:
	@chmod +x lab2_list.gp
	@gnuplot lab2_list.gp

profile: profile.gperf

profile.gperf: list
	@-rm -f ./raw.gperf
	@LD_PRELOAD=/usr/local/cs/gperftools-2.7/lib/libprofiler.so.0 \
	CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	@pprof --text ./lab2_list ./raw.gperf > profile.out
	@pprof --list=threadfunc ./lab2_list ./raw.gperf >> profile.out
	@rm -f ./raw.gperf

dist: profile tests graphs
	@tar -czf lab2b-204996814.tar.gz lab2_list.c SortedList.h SortedList.c \
	Makefile README test.sh lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png \
	lab2b_5.png lab2b_list.csv lab2_list.gp profile.out

clean:
	@rm -f lab2_list *.gz
