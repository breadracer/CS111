/* NAME: Shawn Ma */
/* EMAIL: breadracer@outlook.com */
/* ID: 204996814 */

#include "SortedList.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

// global vars
enum sflags {NOSYNC = 110, MUTEX = 109, SPIN = 115};
enum yflags {
  NONE, INSERT, DELETE, INSERT_DELETE,
  LOOKUP, INSERT_LOOKUP, DELETE_LOOKUP, ALL
};
enum opts {THREADS, ITERATIONS, YIELD, SYNC};
const char *yflagv[] = {"-none", "-i", "-d", "-id", "-l", "-il", "-dl", "-idl"};

struct option options[] = {
  {"threads", required_argument, 0, THREADS},
  {"iterations", required_argument, 0, ITERATIONS},
  {"yield", required_argument, 0, YIELD},
  {"sync", required_argument, 0, SYNC},
  {0, 0, 0, 0}
};


pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int spin_lock = 0;
int opt_yield = 0;
int syncflag = NOSYNC;
int nthreads = 1;
int niterations = 1;
int nlists = 1;
SortedList_t list = {&list, &list, NULL};
SortedListElement_t *elements;

// helper functions
void error(char *message, int code) {
  fprintf(stderr, "%s\n", message);
  exit(code);
}

void sighandler(int signum) {
  (void)(signum);
  error("Segmentation fault, SIGSEGV handler called", 2);
}

int check_yield_argument(char *arg) {
  if (strlen(arg) > 3)
    return 0;
  for (unsigned int i = 0; i < strlen(arg); i++)
    if (arg[i] != 'i' && arg[i] != 'd' && arg[i] != 'l')
      return 0;
  return 1;
}

int check_sync_argument(char *arg) {
  if (strlen(arg) > 1)
    return 0;
  if (arg[0] != MUTEX && arg[0] != SPIN)
    return 0;
  return 1;
}

char *generate_random_key() {
  int length = rand() % 5 + 5;
  char *key = malloc(sizeof(char) * (length + 1));
  for (int i = 0; i < length; i++)
    key[i] = rand() % 71 + '0';
  key[length] = '\0';
  return key;
}

void lock() {
  if (syncflag == SPIN)
    while(__sync_lock_test_and_set(&spin_lock, 1) == 1);
  else if (syncflag == MUTEX)
    pthread_mutex_lock(&m);
}

void unlock() {
  if (syncflag == SPIN)
    __sync_lock_release(&spin_lock);
  else if (syncflag == MUTEX)
    pthread_mutex_unlock(&m);
}

// list routines
void* threadfunc(void* ind) {
  int start = niterations * *(int*)ind;
  int end = start + niterations;
  for (int i = start; i < end; i++) {
    lock();
    SortedList_insert(&list, &elements[i]);
    unlock();
  }
  if (SortedList_length(&list) == -1)
    error("SortedList_length error: the list is corrupted", 2);
  for (int i = start; i < end; i++) {
    lock();
    SortedListElement_t *element = SortedList_lookup(&list, elements[i].key);    
    if (SortedList_delete(element))
      error("SortedList_delete error: corrupted prev/next pointers", 2);
    unlock();
  }
  return NULL;
}

int main(int argc, char **argv) {
  // initialize variables
  int opt, optindex;
  char *type = malloc(sizeof(char) * 15);

  // options processing
  while (1) {
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
      if (check_yield_argument(optarg))
	for (unsigned int i = 0; i < strlen(optarg); i++)
	  switch(optarg[i]) {
	  case 'i': opt_yield |= INSERT_YIELD; break;
	  case 'd': opt_yield |= DELETE_YIELD; break;
	  case 'l': opt_yield |= LOOKUP_YIELD; break;
	  }
      else
	error("Invalid argument for --yield", 1);
      break;
    case SYNC:
      if (check_sync_argument(optarg))
	syncflag = optarg[0];
      else
	error("Invalid argument for --sync", 1);
      break;
    case '?':
      error("Invalid command-line parameter", 1);
    }
  }
  int nelements = nthreads * niterations;
  int noperations = nelements * 3;
  strcat(type, "list");
  strcat(type, yflagv[opt_yield]);    
  switch(syncflag) {
  case NOSYNC:
    strcat(type, "-none"); break;
  case MUTEX:
    strcat(type, "-m"); break;
  case SPIN:
    strcat(type, "-s"); break;
  }
  
  // catch signal
  signal(SIGSEGV, sighandler);

  // initialize list elements
  elements = malloc(sizeof(SortedListElement_t) * nelements);
  if (!elements)
    error("malloc error", 2);
  srand(time(0));
  for (int i = 0; i < nelements; i++) {
    char *key = generate_random_key();
    SortedListElement_t element = {NULL, NULL, key};
    elements[i] = element;
  }

  // timer starts
  struct timespec tstart, tend;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tstart);

  // create threads
  pthread_t *threads = malloc(sizeof(pthread_t) * nthreads);
  int *threadinds = malloc(sizeof(int) * nthreads);
  if (!threads)
    error("malloc error", 2);  
  for (int i = 0; i < nthreads; i++) {
    threadinds[i] = i;
    pthread_t ret = pthread_create(&threads[i], NULL, &threadfunc, &threadinds[i]);
    if (ret)
      error("pthread_create error", 2);
  }
  for (int i = 0; i < nthreads; i++) {
    pthread_t ret = pthread_join(threads[i], NULL);
    if (ret)
      error("pthread_join error", 2);
  }
  int length = SortedList_length(&list);
  if (length != 0)
    error("List length is not zero", 2);
  
  // timer ends
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);
  long long timediff = (tend.tv_sec - tstart.tv_sec) * 1000000000;
  timediff += tend.tv_nsec;
  timediff -= tstart.tv_nsec;
  long long timeavg = timediff / noperations;
  printf("%s,%d,%d,%d,%d,%lld,%lld\n", type, nthreads, niterations,
	 nlists, noperations, timediff, timeavg);
  return 0;
}
