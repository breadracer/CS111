/* NAME: Shawn Ma */
/* EMAIL: breadracer@outlook.com */
/* ID: 204996814 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

enum flags {
  APPEND, CLOEXEC, CREAT, DIRECTORY, DSYNC, EXCL,
  NOFOLLOW, NONBLOCK, RSYNC, SYNC, TRUNC, RDONLY,
  RDWR, WRONLY, PIPE, COMMAND, WAIT, CLOSE, VERBOSE,
  PROFILE, ABORT, CATCH, IGNORE, DEFAULT, PAUSE
};

const char *flagstr[] = {
  "--append", "--cloexec", "--create", "--directory",
  "--dsync", "--excl", "--nofollow", "--nonblock",
  "--rsync", "--sync", "--trunc", "--rdonly", "--rdwr",
  "--wronly", "--pipe", "--command", "--wait",
  "--close", "--verbose", "--profile", "--abort",
  "--catch", "--ignore", "--default", "--pause"
};

const int NUM_OFLAGS = 11;
const char *newline = "\n";
const char *space = " ";

const int offlags[] = {
  O_APPEND, O_CLOEXEC, O_CREAT, O_DIRECTORY,
  O_DSYNC, O_EXCL, O_NOFOLLOW, O_NONBLOCK, O_RSYNC,
  O_SYNC, O_TRUNC, O_RDONLY, O_RDWR, O_WRONLY
};

struct command {
  int argc;
  pid_t pid;
  char **argv;
};

int isopt(char *s) {
  if (strlen(s) < 2)
    return 0;
  else if (s[0] == '-' && s[1] == '-')
    return 1;
  else
    return 0;
}

int orflags(const int *flagv) {
  int flagCombo = 0;
  for (int i = 0; i < NUM_OFLAGS; i++)
    if (flagv[i])
      flagCombo |= offlags[i];
  return flagCombo;
}

void updatetime(struct timeval *utime, struct timeval *stime, int who,
		struct timeval *utimediff, struct timeval *stimediff) {
  struct rusage usage;
  if (getrusage(who, &usage) == -1) {
    fprintf(stderr, "getrusage error: %s\n", strerror(errno));
    exit(1);
  }
  if (utimediff)
    timersub(&usage.ru_utime, utime, utimediff);
  if (stimediff)
    timersub(&usage.ru_stime, stime, stimediff);
  *utime = usage.ru_utime;
  *stime = usage.ru_stime;
  /* double ut = utime->tv_sec + utime->tv_usec * 1e-6; */
  /* double st = stime->tv_sec + stime->tv_usec * 1e-6; */
  /* printf("current utime: %f, current stime: %f\n", ut, st); */
}

void printopt(int option, char *argument) {
  char *text;
  if (argument) {
    text = malloc(sizeof(char) * (strlen(flagstr[option]) + strlen(argument) + 3));
    strcat(text, flagstr[option]);
    strcat(text, space);
    strcat(text, argument);
  } else {
    text = malloc(sizeof(char) * (strlen(flagstr[option]) + 3));
    strcat(text, flagstr[option]);
    strcat(text, space);
  }
  strcat(text, newline);
  write(1, text, strlen(text));
  return;
}

void printprof(int option, char *argument, int who,
	       struct timeval *utime, struct timeval *stime) {
  struct rusage usage;
  if (getrusage(who, &usage) == -1) {
    fprintf(stderr, "getrusage error: %s\n", strerror(errno));
    exit(1);
  }
  struct timeval res;
  timersub(&usage.ru_utime, utime, &res);
  double ut = res.tv_sec + res.tv_usec * 1e-6;
  timersub(&usage.ru_stime, stime, &res);
  double st = res.tv_sec + res.tv_usec * 1e-6;
  if (who == RUSAGE_SELF) {
    if (argument)
      printf("%s %s: user CPU time: %f, system CPU time: %f\n",
	     flagstr[option], argument, ut, st);
    else
      printf("%s: user CPU time: %f, system CPU time: %f\n",
	     flagstr[option], ut, st);    
  } else if (who == RUSAGE_CHILDREN) {
      printf("waited child processes: user CPU time: %f, system CPU time: %f\n", ut, st);
  }
}

void sighandler(int signum) {
  fprintf(stderr, "%d caught\n", signum);
  exit(signum);
}

int main(int argc, char **argv) {
  int flag = 0;
  int sflag = -1;
  
  // Process arguments in sequence
  int opt, optindex;
  static struct option opts[] = {
    {"append", no_argument, 0, APPEND},
    {"cloexec", no_argument, 0, CLOEXEC},
    {"creat", no_argument, 0, CREAT},
    {"directory", no_argument, 0, DIRECTORY},
    {"dsync", no_argument, 0, DSYNC},
    {"excl", no_argument, 0, EXCL},
    {"nofollow", no_argument, 0, NOFOLLOW},
    {"nonblock", no_argument, 0, NONBLOCK},
    {"rsync", no_argument, 0, RSYNC},
    {"sync", no_argument, 0, SYNC},
    {"trunc", no_argument, 0, TRUNC},
    {"command", required_argument, 0, COMMAND},
    {"wait", no_argument, 0, WAIT},
    {"rdonly", required_argument, 0, RDONLY},
    {"rdwr", required_argument, 0, RDWR},
    {"wronly", required_argument, 0, WRONLY},
    {"pipe", no_argument, 0, PIPE},
    {"abort", no_argument, 0, ABORT},
    {"catch", required_argument, 0, CATCH},
    {"default", required_argument, 0, DEFAULT},
    {"pause", no_argument, 0, PAUSE},
    {"ignore", required_argument, 0, IGNORE},
    {"close", required_argument, 0, CLOSE},
    {"verbose", no_argument, 0, VERBOSE},
    {"profile", no_argument, 0, PROFILE},
    {0, 0, 0, 0}
  };

  // Initializing variables
  int oflagv[11] = {0}; // Set the oflagv vector initially to all 0's
  int vflag = 0;
  int pflag = 0;
  int fdcounter = 0;
  int *fdv = malloc(sizeof(int) * argc);
  int cmdhead = 0;
  int cmdtail = 0;
  struct sigaction act;
  struct command *cmdv = malloc(sizeof(struct command) * argc);
  struct timeval utimeg = {0, 0};
  struct timeval stimeg = {0, 0};
  struct timeval utimec = {0, 0};
  struct timeval stimec = {0, 0};  
  
  // If fd number in --command not assigned, get -1 and error
  for (int i = 0; i < argc; i++) 
    fdv[i] = -1;
  
  while(1) {
    opt = getopt_long(argc, argv, "", opts, &optindex);
    if (opt == -1)
      break;
    updatetime(&utimeg, &stimeg, RUSAGE_SELF, NULL, NULL);
    switch(opt) {
    case APPEND:
    case CLOEXEC:
    case CREAT:
    case DIRECTORY:
    case DSYNC:
    case EXCL:
    case NOFOLLOW:
    case NONBLOCK:
    case RSYNC:
    case SYNC:
    case TRUNC:
      if (vflag)
	printopt(opt, NULL);
      oflagv[opt] = 1;
      if (pflag)
	printprof(opt, NULL, RUSAGE_SELF, &utimeg, &stimeg);
      break;
    case RDONLY:
    case RDWR:
    case WRONLY: {
      if (vflag)
	printopt(opt, optarg);
      int fd;
      if (oflagv[CREAT] == 1)
	fd = open(optarg, offlags[opt] | orflags(oflagv), 0666);
      else
	fd = open(optarg, offlags[opt] | orflags(oflagv));
      if (fd == -1) {
	fprintf(stderr, "Open file failed with flag '%s': %s\n",
		flagstr[opt], strerror(errno));
	fdcounter++;
	flag = 1;
      }
      else
	fdv[fdcounter++] = fd;
      for (int i = 0; i < NUM_OFLAGS; i++)
	oflagv[i] = 0;
      if (pflag)
	printprof(opt, NULL, RUSAGE_SELF, &utimeg, &stimeg);
      break;	
    }
    case PIPE: {
      if (vflag)
	printopt(opt, NULL);
      int fds[2];
      if (pipe(fds) == -1) {
	fprintf(stderr, "Open pipe failed: %s\n", strerror(errno));
	fdcounter += 2;
	flag = 1;
      } else {
	fdv[fdcounter++] = fds[0];
	fdv[fdcounter++] = fds[1];
      }
      if (pflag)
	printprof(opt, NULL, RUSAGE_SELF, &utimeg, &stimeg);
      break;
    }
    case COMMAND: {
      // Get subcommand in subargc and subargv
      int subargc = 0;
      for (int i = optind - 1; i < argc; i++) {
      	if (isopt(argv[i]))
      	  break;
	subargc++;
      }      
      if (subargc <= 3) {
	fprintf(stderr, "--command: missing command\n");
	flag = 1;
	break;
      }
      if (vflag) {
      	for (int i = optind - 2; i < optind + subargc - 1; i++) {
	  write(1, argv[i], strlen(argv[i]));
      	  if (i == optind + subargc - 2)
	    write(1, newline, 1);
      	  else
	    write(1, space, 1);
	}
      }
      char **subargv = malloc(sizeof(char*) * subargc);
      for (int i = optind - 1; i < optind + subargc - 1; i++)
	subargv[i - optind + 1] = argv[i];      
      // Get information of the subcommand
      char **commandargs = malloc(sizeof(char*) * (subargc - 2));
      int *commandfds = malloc(sizeof(int) * 3);
      for (int i = 0; i < 3; i++)
	commandfds[i] = atoi(subargv[i]);
      for (int i = 3; i < subargc; i++)
	commandargs[i - 3] = subargv[i];
      commandargs[subargc - 3] = NULL;
      for (int i = 0; i < 3; i++)
	if (fdv[commandfds[i]] == -1) {
	  fprintf(stderr, "Error file descriptor %d\n", commandfds[i]);
	  exit(1);
	}

      int rc = fork();
      if (rc < 0) {
	fprintf(stderr, "fork failed: %s\n", strerror(errno));
	exit(1);
      } else if (rc == 0) {
	// Duplicate the file descriptors
	for (int i = 0; i < 3; i++)
	  dup2(fdv[commandfds[i]], i);
	for (int i = 0; i < fdcounter; i++)
	  close(fdv[i]);
	// Execute
	int ret = execvp(commandargs[0], commandargs);
	if (ret == -1) {
	  fprintf(stderr, "execvp filed: %s\n", strerror(errno));
	  _exit(1);
	}
      } else {
	struct command cmd;
	cmd.argv = commandargs;
	cmd.argc = subargc - 3;
	cmd.pid = rc;
	cmdv[cmdtail++] = cmd;
	if (pflag) {
	  for (int i = optind - 2; i < optind + subargc - 1; i++) {
	    printf("%s", argv[i]);
	    if (i == optind + subargc - 2) {
	      struct rusage usage;
	      if (getrusage(RUSAGE_SELF, &usage) == -1) {
		fprintf(stderr, "getrusage error: %s\n", strerror(errno));
		exit(1);
	      }
	      struct timeval res;
	      timersub(&usage.ru_utime, &utimeg, &res);
	      double ut = res.tv_sec + res.tv_usec * 1e-6;
	      timersub(&usage.ru_stime, &stimeg, &res);
	      double st = res.tv_sec + res.tv_usec * 1e-6;
	      printf(": user CPU time: %f, system CPU time: %f\n", ut, st);
	    }
	    else
	      printf("%s", space);
	  }
	}
      }
      optind += subargc - 1;
      break;
    }
    case WAIT: {
      if (vflag)
      	printopt(opt, NULL);
      struct timeval totalutime = {0, 0};
      struct timeval totalstime = {0, 0};
      int wstatus, ecode, scode;
      for (int i = cmdhead; i < cmdtail; i++) {
      	if (waitpid(cmdv[i].pid, &wstatus, 0) == -1) {
      	  fprintf(stderr, "wait error: %s\n", strerror(errno));
      	  exit(1);
      	}
	updatetime(&utimeg, &stimeg, RUSAGE_SELF, NULL, NULL);
      	if (WIFEXITED(wstatus)) {
      	  ecode = WEXITSTATUS(wstatus);
      	  printf("exit %d ", ecode);
      	  if (ecode > flag)
      	    flag = ecode;
      	}
      	else if (WIFSIGNALED(wstatus)) {
      	  scode = WTERMSIG(wstatus);
      	  printf("signal %d ", scode);
      	  if (scode > sflag)
      	    sflag = scode;
      	}
      	else {
      	  fprintf(stderr, "child neither exit normally nor via signal\n");
      	  exit(1);
      	}
      	for (int j = 0; j < cmdv[i].argc; j++)
      	  printf("%s ", cmdv[i].argv[j]);
      	printf("%s", newline);
	struct timeval subutime, substime, sum;
	updatetime(&utimeg, &stimeg, RUSAGE_SELF, &subutime, &substime);
	timeradd(&totalutime, &subutime, &sum);
	totalutime = sum;
	timeradd(&totalstime, &substime, &sum);
	totalstime = sum;
      }
      cmdhead = cmdtail;      
      if (pflag) {
	double ut = totalutime.tv_sec + totalutime.tv_usec * 1e-6;
	double st = totalstime.tv_sec + totalstime.tv_usec * 1e-6;
	printf("%s: user CPU time: %f, system CPU time: %f\n",
	   flagstr[opt], ut, st);
      }
      if (pflag) {	  
	printprof(0, NULL, RUSAGE_CHILDREN, &utimec, &stimec);
      }
      updatetime(&utimec, &stimec, RUSAGE_CHILDREN, NULL, NULL);
      break;
    }
    case CLOSE: {
      if (vflag)
      	printopt(opt, optarg);
      int fnum = atoi(optarg);
      if (close(fdv[fnum]) == -1) {
	fprintf(stderr, "close file %d error: %s\n", fnum, strerror(errno));
	exit(1);
      }
      fdv[fnum] = -1;
      if (pflag)
	printprof(opt, optarg, RUSAGE_SELF, &utimeg, &stimeg);
      break;      
    }
    case ABORT: {
      if (vflag)
	printopt(opt, NULL);
      fflush(stdout);
      int* junk = NULL;
      flag = *junk;
      if (pflag)
	printprof(opt, NULL, RUSAGE_SELF, &utimeg, &stimeg);
      break;
    }
    case CATCH:
    case IGNORE:
    case DEFAULT: {
      if (vflag)
      	printopt(opt, optarg);
      if (opt == CATCH)
	act.sa_handler = sighandler;
      else if (opt == IGNORE)
	act.sa_handler = SIG_IGN;
      else
	act.sa_handler = SIG_DFL;
      if (sigaction(atoi(optarg), &act, NULL) == -1) {
	fprintf(stderr, "sigaction error: %s\n", strerror(errno));
	exit(1);
      }
      if (pflag)
	printprof(opt, optarg, RUSAGE_SELF, &utimeg, &stimeg);
      break;
    }
    case PAUSE:
      if (vflag)
	printopt(opt, NULL);
      pause();
      if (pflag)
	printprof(opt, NULL, RUSAGE_SELF, &utimeg, &stimeg);
      break;
    case VERBOSE:
      vflag = 1;
      if (pflag)
	printprof(opt, NULL, RUSAGE_SELF, &utimeg, &stimeg);
      break;
    case PROFILE:
      if (vflag)
	printopt(opt, NULL);
      pflag = 1;
      break;
    case '?':
      if (!flag)
	flag = 1;
    }
  }

  if (sflag != -1) {
    fflush(stdout);
    signal(sflag, SIG_DFL);
    raise(sflag);
  }
  return flag;
}
