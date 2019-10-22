/* NAME: Shawn Ma */
/* EMAIL: breadracer@outlook.com */
/* ID: 204996814 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
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
  "--append ", "--cloexec ", "--create ", "--directory ",
  "--dsync ", "--excl ", "--nofollow ", "--nonblock ",
  "--rsync ", "--sync ", "--trunc ", "--rdonly ", "--rdwr ",
  "--wronly ", "--pipe ", "--command ", "--wait ",
  "--close ", "--verbose ", "--profile ", "--abort ",
  "--catch ", "--ignore ", "--default ", "--pause "
};

const int NUM_OFLAGS = 11;
const char *newline = "\n";

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

int isOption(char *s) {
  if (strlen(s) < 2)
    return 0;
  else if (s[0] == '-' && s[1] == '-')
    return 1;
  else
    return 0;
}

int orFlags(const int *flagv) {
  int flagCombo = 0;
  for (int i = 0; i < NUM_OFLAGS; i++)
    if (flagv[i])
      flagCombo |= offlags[i];
  return flagCombo;
}

void verbose(int option, char *argument) {
  char *text;
  if (argument) {
    text = malloc(sizeof(char) * (strlen(flagstr[option]) + strlen(argument) + 2));
    strcat(text, flagstr[option]);
    strcat(text, argument);
  } else {
    text = malloc(sizeof(char) * (strlen(flagstr[option]) + 2));
    strcat(text, flagstr[option]);
  }
  strcat(text, newline);
  write(1, text, strlen(text));
  return;
}

void sighandler(int signum) {
  fprintf(stderr, "%d caught\n", signum);
  exit(signum);
}

int main(int argc, char **argv) {
  int flag = 0;
  int rflag = -1;
  
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
    {0, 0, 0, 0}
  };

  // Initializing variables
  struct sigaction act;
  int oflagv[11] = {0}; // Set the oflagv vector initially to all 0's
  int verboseflag = 0;
  int fdcounter = 0;
  int *fdv = malloc(sizeof(int) * argc);
  int cmdhead = 0;
  int cmdtail = 0;
  struct command *cmdv = malloc(sizeof(struct command) * argc);
  
  // If fd number in --command not assigned, get -1 and error
  for (int i = 0; i < argc; i++) 
    fdv[i] = -1;
  
  while(1) {
    opt = getopt_long(argc, argv, "", opts, &optindex);
    if (opt == -1)
      break;
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
      if (verboseflag)
	verbose(opt, NULL);
      oflagv[opt] = 1;
      break;
    case RDONLY:
    case RDWR:
    case WRONLY: {
      // TODO: Possibly more flags
      int fd;
      if (oflagv[CREAT] == 1)
	fd = open(optarg, offlags[opt] | orFlags(oflagv), 0666);
      else
	fd = open(optarg, offlags[opt] | orFlags(oflagv));
      if (fd == -1) {
	fprintf(stderr, "Open file failed with flag '%s': %s\n", flagstr[opt], strerror(errno));
	fdcounter++;
	flag = 1;
      }
      else
	fdv[fdcounter++] = fd;
      if (verboseflag)
	verbose(opt, optarg);
      for (int i = 0; i < NUM_OFLAGS; i++)
	oflagv[i] = 0;
      break;	
    }
    case PIPE: {
      int fds[2];
      if (pipe(fds) == -1) {
	fprintf(stderr, "Open pipe failed: %s\n", strerror(errno));
	fdcounter += 2;
	flag = 1;
      } else {
	fdv[fdcounter++] = fds[0];
	fdv[fdcounter++] = fds[1];
      }
      if (verboseflag)
	verbose(opt, NULL);
      break;
    }
    case COMMAND: {
      // Get subcommand in subargc and subargv
      int subargc = 0;
      for (int i = optind - 1; i < argc; i++) {
      	if (isOption(argv[i]))
      	  break;
	subargc++;
      }      
      if (subargc <= 3) {
	fprintf(stderr, "--command: missing command\n");
	flag = 1;
	break;
      }
      if (verboseflag) {
      	for (int i = optind - 2; i < optind + subargc - 1; i++) {
	  write(1, argv[i], strlen(argv[i]));
      	  if (i == optind + subargc - 2)
	    write(1, "\n", 1);
      	  else
	    write(1, " ", 1);
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
      optind += subargc - 1;

      int rc = fork();
      if (rc < 0) {
	fprintf(stderr, "Fork failed: %s\n", strerror(errno));
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
	  fprintf(stderr, "Execvp filed: %s\n", strerror(errno));
	  _exit(1);
	}
      } else {
	struct command cmd;
	cmd.argv = commandargs;
	cmd.argc = subargc - 3;
	cmd.pid = rc;
	cmdv[cmdtail++] = cmd;
      }
      break;
    }
    case WAIT: {
      if (verboseflag)
      	verbose(opt, NULL);
      int wstatus, ecode, rcode, maxe, maxr;
      maxe = flag;
      maxr = rflag;
      int sflag = 0;
      for (int i = cmdhead; i < cmdtail; i++) {
      	int ret = waitpid(cmdv[i].pid, &wstatus, 0);
      	if (ret == -1) {
      	  fprintf(stderr, "wait error: %s\n", strerror(errno));
      	  exit(1);
      	}
      	if (WIFEXITED(wstatus)) {
      	  ecode = WEXITSTATUS(wstatus);
      	  printf("exit %d ", ecode);
      	  if (ecode > maxe)
      	    maxe = ecode;
      	}
      	else if (WIFSIGNALED(wstatus)) {
      	  rcode = WTERMSIG(wstatus);
      	  printf("signal %d ", rcode);
      	  sflag = 1;
      	  if (rcode > maxr)
      	    maxr = rcode;
      	}
      	else {
      	  fprintf(stderr, "child neither exit normally nor via signal\n");
      	  exit(1);
      	}
      	for (int j = 0; j < cmdv[i].argc; j++)
      	  printf("%s ", cmdv[i].argv[j]);
      	printf("%s", newline);
      }
      cmdhead = cmdtail;
      if (sflag && maxr > rflag)
      	rflag = maxr;
      else if (maxe > flag)
      	flag = maxe;
      break;
    }
    case CLOSE: {
      if (verboseflag)
      	verbose(opt, optarg);
      int fnum = atoi(optarg);
      if (close(fdv[fnum]) == -1) {
	fprintf(stderr, "close file %d error: %s\n", fnum, strerror(errno));
	exit(1);
      }
      fdv[fnum] = -1;
      break;      
    }
    case ABORT: {
      if (verboseflag)
	verbose(opt, NULL);
      fflush(stdout);
      int* junk = NULL;
      flag = *junk;
      break;
    }
    case CATCH:
    case IGNORE:
    case DEFAULT: {
      if (verboseflag)
      	verbose(opt, optarg);
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
      break;
    }
    case PAUSE:
      if (verboseflag)
	verbose(opt, NULL);
      pause();
      break;
    case VERBOSE:
      verboseflag = 1;
      break;
    case '?':
      if (!flag)
	flag = 1;
    }
  }

  if (rflag != -1) {
    fflush(stdout);
    signal(rflag, SIG_DFL);
    raise(rflag);
  }
  return flag;
}
