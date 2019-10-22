#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2_list-1.png ... cost per operation vs threads and iterations
#	lab2_list-2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2_list-3.png ... threads and iterations that run (protected) w/o failure
#	lab2_list-4.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","


set title "List-1: Throughput vs. Number of Threads for Mutex and Spin-lock"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title 'list with mutex' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title 'list with spin' with linespoints lc rgb 'green'


set title "List-2: Timing Mutex Waits"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Average time per operation (ns)"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):($7) \
     	title 'completion time' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):($8) \
     	title 'waiting for lock' with linespoints lc rgb 'green'

     
set title "List-3: Check correctness of list"
set xlabel "Threads"
set xrange [0.75:]
set ylabel "Successful iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
     "< grep 'list-id-none,' lab2b_list.csv" using ($2):($3) \
     with points lc rgb 'blue' title "None", \
     "< grep 'list-id-m,' lab2b_list.csv" using ($2):($3) \
     with points lc rgb 'yellow' title "Mutex", \
     "< grep 'list-id-s,' lab2b_list.csv" using ($2):($3) \
     with points lc rgb 'red' title "Spin-Lock", \


set title "List-4: Throughput with Mutex"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_4.png'

plot \
     "< grep 'list-none-m,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title 'lists=1' with linespoints lc rgb 'orange', \
     "< grep 'list-none-m,.*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title 'lists=4' with linespoints lc rgb 'purple', \
     "< grep 'list-none-m,.*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title 'lists=8' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,.*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title 'lists=16' with linespoints lc rgb 'yellow', \

set title "Graph 5: Throughput with Spin-lock"
set xlabel "Threads"
set logscale x 2
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_5.png'
plot \
     "< grep 'list-none-s,.*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title 'lists=1' with linespoints lc rgb 'orange', \
     "< grep 'list-none-s,.*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title 'lists=4' with linespoints lc rgb 'purple', \
     "< grep 'list-none-s,.*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title 'lists=8' with linespoints lc rgb 'green', \
     "< grep 'list-none-s,.*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     	title 'lists=16' with linespoints lc rgb 'yellow'