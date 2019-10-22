/* NAME: Shawn Ma */
/* EMAIL: breadracer@outlook.com */
/* ID: 204996814 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

// global vars
enum sflags {NOSYNC = 110, MUTEX = 109, SPIN = 115, CAS = 99};
enum opts {THREADS, ITERATIONS, YIELD, SYNC};

struct option options[] = {
  {"threads", required_argument, 0, THREADS},
  {"iterations", required_argument, 0, ITERATIONS},
  {"yield", no_argument, 0, YIELD},
  {"sync", required_argument, 0, SYNC},
  {0, 0, 0, 0}
};

struct arguments {
  long long *counter;
  int niters;
  int yflag;
  int sflag;
};

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int spin_lock = 0;

// helper functions
void error(char *message, int code) {
  fprintf(stderr, "%s\n", message);
  exit(code);
}

// adder routines
void add(long long *pointer, long long value, int yflag, int sflag) {
  long long sum, tmp, tmpsum;
  if (sflag == CAS) {
    do {
      tmp = *pointer;
      tmpsum = tmp + value;
      if (yflag && sched_yield() != 0)
	error("sched_yield error", 1);
    } while(__sync_val_compare_and_swap(pointer, tmp, tmpsum) != tmp);
  } else {
    if (sflag == SPIN)
      while(__sync_lock_test_and_set(&spin_lock, 1) == 1);
    else if (sflag == MUTEX)
      pthread_mutex_lock(&m);   
    sum = *pointer + value;
    if (yflag && sched_yield() != 0)
      error("sched_yield error", 1);
    *pointer = sum;
    if (sflag == SPIN)
      __sync_lock_release(&spin_lock);
    else if (sflag == MUTEX)
      pthread_mutex_unlock(&m);
  }
}

void* threadfunc(void* argument) {
  struct arguments *args = (struct arguments*)argument;
  int yflag = args->yflag, sflag = args->sflag, niters = args->niters;
  long long *counter = args->counter;
  for (int i = 0; i < niters; i++)
    add(counter, 1, yflag, sflag);
  for (int i = 0; i < niters; i++)
    add(counter, -1, yflag, sflag);
  return NULL;
}

int main(int argc, char **argv) {
  // initialize variables
  int opt, optindex;
  int nthreads = 1;
  int niterations = 1;
  int yieldflag = 0;
  int syncflag = NOSYNC;
  long long counter = 0;
  char *type = malloc(sizeof(char) * 15);
  
  // scan options
  while(1) {
    opt = getopt_long(argc, argv, "", options, &optindex);
    if (opt == -1)
      break;
    switch(opt) {
    case THREADS:
      nthreads = atoi(optarg);
      break;
    case ITERATIONS:
      niterations = atoi(optarg);
      break;
    case YIELD:
      yieldflag = 1;
      break;
    case SYNC:
      syncflag = optarg[0];
      if ((syncflag != CAS && syncflag != MUTEX
	   && syncflag != SPIN) || strlen(optarg) > 1)
	error("Invalid argument for --sync", 1);
      break;
    case '?':
      error("Invalid command-line parameter", 1);
    }
  }
  int noperations = nthreads * niterations * 2;
  strcat(type, "add");
  if (yieldflag)
    strcat(type, "-yield");
  switch(syncflag) {
  case NOSYNC:
    strcat(type, "-none"); break;
  case MUTEX:
    strcat(type, "-m"); break;
  case SPIN:
    strcat(type, "-s"); break;
  case CAS:
    strcat(type, "-c"); break;
  }
  
  // main adder operations  
  struct timespec tstart, tend;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tstart);
  pthread_t *threads = malloc(sizeof(pthread_t) * nthreads);
  if (!threads)
    error("malloc error", 2);
  struct arguments args = {&counter, niterations, yieldflag, syncflag};  
  for (int i = 0; i < nthreads; i++) {
    pthread_t ret = pthread_create(&threads[i], NULL, &threadfunc, &args);
    if (ret)
      error("pthread_create error", 2);
  }
  for (int i = 0; i < nthreads; i++) {
    pthread_t ret = pthread_join(threads[i], NULL);
    if (ret)
      error("pthread_join error", 2);
  }
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);
  long long timediff = (tend.tv_sec - tstart.tv_sec) * 1000000000;
  timediff += tend.tv_nsec;
  timediff -= tstart.tv_nsec;
  long long timeavg = timediff / noperations;
  printf("%s,%d,%d,%d,%lld,%lld,%lld\n", type, nthreads, niterations,
	 2 * nthreads * niterations, timediff, timeavg, counter);

  return 0;
}
