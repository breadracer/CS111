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
enum opts {THREADS, ITERATIONS, YIELD, SYNC, LISTS};
const char *yflagv[] = {"-none", "-i", "-d", "-id", "-l", "-il", "-dl", "-idl"};

struct timer {
  struct timespec start;
  struct timespec end;
  long long elapsd;
};

struct option options[] = {
  {"threads", required_argument, 0, THREADS},
  {"iterations", required_argument, 0, ITERATIONS},
  {"yield", required_argument, 0, YIELD},
  {"sync", required_argument, 0, SYNC},
  {"lists", required_argument, 0, LISTS},
  {0, 0, 0, 0}
};


pthread_mutex_t *mutexes;
int *spin_locks;
int opt_yield = 0;
int syncflag = NOSYNC;
int nthreads = 1;
int niterations = 1;
int nlists = 1;
int *threadnlocks;
SortedList_t *lists;
SortedListElement_t *elements;
long long *threadwaits;

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

long hash(const char *str) {
  long hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}

long long gettimediff(struct timespec *start, struct timespec *end) {
  long long timediff = (end->tv_sec - start->tv_sec) * 1000000000;
  timediff += end->tv_nsec;
  timediff -= start->tv_nsec;
  return timediff;
}

void gettime(struct timespec* t) {
  if (clock_gettime(CLOCK_MONOTONIC, t) == -1)
    error("clock_gettime error", 1);
}

void lock(long index) {
  if (syncflag == SPIN)
    while(__sync_lock_test_and_set(&spin_locks[index], 1) == 1);
  else if (syncflag == MUTEX)
    pthread_mutex_lock(&mutexes[index]);
}

void unlock(long index) {
  if (syncflag == SPIN)
    __sync_lock_release(&spin_locks[index]);
  else if (syncflag == MUTEX)
    pthread_mutex_unlock(&mutexes[index]);
}


// list routines
void* threadfunc(void* ind) {
  int start = niterations * *(int*)ind;
  int end = start + niterations;
  struct timespec tstart, tend;
  long hashind;
  for (int i = start; i < end; i++) {
    hashind = hash(elements[i].key) % nlists;
    gettime(&tstart);
    lock(hashind);
    gettime(&tend);
    threadwaits[*(int*)ind] += gettimediff(&tstart, &tend);
    SortedList_insert(&lists[hashind], &elements[i]);
    unlock(hashind);
  }
  for (int i = 0; i < nlists; i++) {
    hashind = hash(elements[i].key) % nlists;
    gettime(&tstart);
    lock(hashind);
    gettime(&tend);
    threadwaits[*(int*)ind] += gettimediff(&tstart, &tend);
    if (SortedList_length(&lists[i]) == -1)
      error("SortedList_length error: some of the lists are corrupted", 2);
    unlock(hashind);
  }
  for (int i = start; i < end; i++) {
    hashind = hash(elements[i].key) % nlists;
    gettime(&tstart);
    lock(hashind);
    gettime(&tend);
    threadwaits[*(int*)ind] += gettimediff(&tstart, &tend);
    SortedListElement_t *element = SortedList_lookup(&lists[hashind], elements[i].key);
    if (SortedList_delete(element))
      error("SortedList_delete error: corrupted prev/next pointers", 2);
    unlock(hashind);
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
      if (nthreads <= 0)
	error("Invalid thread number", 1);
      break;
    case ITERATIONS:
      niterations = atoi(optarg);
      if (niterations <= 0)
	error("Invalid iteration number", 1);
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
    case LISTS:
      nlists = atoi(optarg);
      if (nlists <= 0)
	error("Invalid list number", 1);
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

  // initialize lists and locks
  lists = malloc(sizeof(SortedList_t) * nlists);
  mutexes = malloc(sizeof(pthread_mutex_t) * nlists);
  spin_locks = malloc(sizeof(int) * nlists);
  if (!lists || !mutexes || !spin_locks)
    error("malloc error", 2);
  for (int i = 0; i < nlists; i++) {
    SortedList_t list = {&lists[i], &lists[i], NULL};
    lists[i] = list;
    pthread_mutex_init(&mutexes[i], NULL);
    spin_locks[i] = 0;    
  }
  
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
  if (clock_gettime(CLOCK_MONOTONIC, &tstart) == -1)
    error("clock_gettime error", 1);
  // create threads
  pthread_t *threads = malloc(sizeof(pthread_t) * nthreads);
  threadwaits = malloc(sizeof(long long) * nthreads);
  threadnlocks = malloc(sizeof(int) * nthreads);
  int *threadinds = malloc(sizeof(int) * nthreads);
  if (!threads || !threadwaits || !threadinds || !threadnlocks)
    error("malloc error", 2);
  for (int i = 0; i < nthreads; i++) {
    threadinds[i] = i;
    threadwaits[i] = 0;
    threadnlocks[i] = 0;
    pthread_t ret = pthread_create(&threads[i], NULL, &threadfunc, &threadinds[i]);
    if (ret)
      error("pthread_create error", 2);
  }
  for (int i = 0; i < nthreads; i++) {
    pthread_t ret = pthread_join(threads[i], NULL);
    if (ret)
      error("pthread_join error", 2);
  }
  for (int i = 0; i < nlists; i++)
    if (SortedList_length(&lists[i]) != 0)
      error("List length is not zero", 2);
  
  // timer ends
  if (clock_gettime(CLOCK_MONOTONIC, &tend) == -1)
    error("clock_gettime error", 1);
  long long timediff = gettimediff(&tstart, &tend);
  long long timeavg = timediff / noperations;

  // obtain average lock-waiting time
  long long sumwait = 0, avgwait = 0;
  if (syncflag != NOSYNC) {
    for (int i = 0; i < nthreads; i++)
      sumwait += threadwaits[i];
    avgwait = sumwait / ((2 * niterations + nlists) * nthreads);
  }
  printf("%s,%d,%d,%d,%d,%lld,%lld,%lld\n", type, nthreads, niterations,
	 nlists, noperations, timediff, timeavg, avgwait);
  return 0;
}
