#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

const int ARG_OPT = 1;
const int NON_ARG_OPT = 2;
const int ARG_OPT_W_ARG = 3;
const char *USAGE_MSG = "Usage: lab0 [--input] [filename] [--output] [filename] [--segfault] [--catch] [--dump-core]\n";

void sighandler(int signum) {  
  fprintf(stderr, "Segmentation fault, signum: %d, SIGSEGV handler called\n", signum);
  exit(4);
}

int compare(char *a, char* b) {
  while (*a && *b) {
    if (*a != *b)
      return 0;
    a++;
    b++;
  }
  if (*a || *b)
    return 0;
  return 1;
}

int checkopts(char *a) {
  const int NUM_ARG_OPTS = 2;
  const int NUM_NONARG_OPTS = 3;
  char *argopts[] = {"--input", "--output"};
  char *nonargopts[] = {"--segfault", "--catch", "--dump-core"};

   
  for (int i = 0; i < NUM_ARG_OPTS; i++)
    if (compare(a, argopts[i]))
      return ARG_OPT;
  for (int j = 0; j < NUM_NONARG_OPTS; j++)
    if (compare(a, nonargopts[j]))
      return NON_ARG_OPT;

  int length = 0;
  for (char *curr = a; *curr != 0; curr++)
    length++;


  if (length >= 7) {
    char temp[9];
    for (int i = 0; i < 7; i++) {
      temp[i] = *(a + i);
    }
    temp[7] = '=';
    temp[8] = 0;
    if (compare(temp, "--input=")) {
      return ARG_OPT_W_ARG;
    }
  }

  if (length >= 8) {
    char temp[10];
    for (int i = 0; i < 8; i++) {
      temp[i] = *(a + i);
    }
    temp[8] = '=';
    temp[9] = 0;
    if (compare(temp, "--output=")) {
      return ARG_OPT_W_ARG;
    }
  }
 
  return 0;
}

int main(int argc, char **argv) {
  char **currarg = argv + 1;
  for (int i = 0; i < argc - 1; i++) {
    char *currchar = *(currarg + i);
    int check = checkopts(currchar);
    if (!check) {
      fprintf(stderr, "%s", USAGE_MSG);
      exit(1);
    }
    if (check == ARG_OPT) {
      if (i == argc - 2 || checkopts(*(currarg + i + 1))) {
	fprintf(stderr, "%s", USAGE_MSG);
	exit(1);
      }
      i++;
    }
  }
  int *empty;
  char *inpath = NULL;
  char *outpath = NULL;
  int segfault = 0;
  int segsignal = 0;
  int opt;
  int ifd, ofd;
  while(1) {
    int optindex = 0;
    static struct option longopts[] = {
      {"input", required_argument, 0, 'i'},
      {"output", required_argument, 0, 'o'},
      {"segfault", no_argument, 0, 's'},
      {"catch", no_argument, 0, 'c'},
      {"dump-core", no_argument, 0, 'd'},
      {0, 0, 0, 0}
    };

    opt = getopt_long(argc, argv, "", longopts, &optindex);
    if (opt == -1)
      break;
    switch(opt) {
    case 'i':
      inpath = optarg;
      if((ifd = open(optarg, O_RDONLY)) < 0) {
	fprintf(stderr, "--input: cannot open '%s', %s", inpath, strerror(errno));
	exit(2);	
      }
      break;
    case 'o':
      outpath = optarg;
      if((ofd = creat(optarg, 0666)) < 0) {
	fprintf(stderr, "--output: cannot create or truncate '%s', %s", outpath, strerror(errno));
	exit(3);
      }
      break;
    case 's':
      segfault = 1;
      break;
    case 'c':
      if (!segfault)
	segsignal = 1;
      break;
    case 'd':
      segsignal = 0;
      break;
    case '?':
      break;
    }
  }
  
  if (segsignal)
    signal(SIGSEGV, sighandler);
  
  if (segfault) {
    empty = NULL;
    segfault = *empty;
  }

  char *buf = malloc(sizeof(char));
  int count = 0;
  if (inpath) {          
    close(0);
    dup(ifd);
    close(ifd);
    struct stat statbuf;
    fstat(0, &statbuf);
    count = statbuf.st_size;
    char *newbuf = realloc(buf, (count + 1) * sizeof(char));
    buf = newbuf;
  } else {
    int ret;
    char c;
    while((ret = read(0, &c, 1))) {
      count++;
      char *newbuf = realloc(buf, sizeof(char) * (count + 1));
      buf = newbuf;
      buf[count - 1] = c;
    }
  }
  
  if (read(0, buf, count) < 0) {
    fprintf(stderr, "read error: %s", strerror(errno));
    exit(-1);	
  }
  if (outpath) {
    close(1);
    dup(ofd);
    close(ofd);
  }
  if (write(1, buf, count) < 0) {
    fprintf(stderr, "write error: %s", strerror(errno));
    exit(-1);
  }

  return 0;
}
