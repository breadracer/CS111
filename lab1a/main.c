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

int isOption(char *s) {
  int length = 0;
  for (char *curr = s; *curr != 0 && length < 2; curr++)
    length++;
  if (length < 2)
    return 0;
  else if (s[0] == '-' && s[1] == '-')
    return 1;
  else
    return 0;
}

int main(int argc, char **argv) {
  // TODO: Check arguments
  int flag = 0;
  
  // Process arguments in sequence
  int fdcounter = 0;
  static struct option opts[] = {
    {"command", required_argument, 0, 'c'},
    {"rdonly", required_argument, 0, 'r'},
    {"wronly", required_argument, 0, 'w'},
    {"verbose", no_argument, 0, 'v'},
    {0, 0, 0, 0}
  };
  int opt, optindex;
  int verboseflag = 0;
  // TODO: More possible combination of fds
  int *fdv = malloc(sizeof(int) * argc);

  // If fd number in --command not assigned, get -1 and error
  for (int i = 0; i < argc; i++)
    fdv[i] = -1;
  
  while(1) {
    opt = getopt_long(argc, argv, "", opts, &optindex);
    if (opt == -1)
      break;
    switch(opt) {
    case 'c': {
      // Get subcommand in subargc and subargv
      int subargc = 0;
      for (int i = optind - 1; i < argc; i++) {
      	if (isOption(argv[i]))
      	  break;
	subargc++;
      }
      //printf("length of subcommand: %d\nsubcommand: ", subargc);
      char **subargv = malloc(sizeof(char*) * subargc);
      for (int i = optind - 1; i < optind + subargc - 1; i++) {
	subargv[i - optind + 1] = argv[i];
	//printf("%s ", subargv[i - optind + 1]);
      }
      //printf("\n");
      
      // Get information of the subcommand
      //printf("file descriptors of subcommand: ");
      char **commandargs = malloc(sizeof(char*) * subargc);
      int *commandfds = malloc(sizeof(int) * 3);
      for (int i = 0; i < 3; i++) {
	commandfds[i] = atoi(subargv[i]);
	//printf("%d ", commandfds[i]);
      }
      //printf("\n");
      //printf("command to be executed in child: ");
      for (int i = 3; i < subargc; i++) {
	commandargs[i - 3] = subargv[i];
      }
      commandargs[subargc - 3] = NULL;
      //printf("\n");

      if (verboseflag) {
      	for (int i = optind - 2; i < optind + subargc - 1; i++) {
	  char *buf = argv[i];
	  int length = 0;
	  for (char *curr = buf; *curr; curr++)
	    length++;
	  write(1, argv[i], length);
      	  if (i == optind + subargc - 2)
	    write(1, "\n", 1);	  
	      //      	    printf("%s\n", argv[i]);
      	  else
	    write(1, " ", 1);
	    //      	    printf("%s ", argv[i]);	  
	}

      }

      // Duplicate the file descriptors
      for (int i = 0; i < 3; i++) {
	if (fdv[commandfds[i]] == -1) {
	  fprintf(stderr, "Error file descriptor %d\n", commandfds[i]);
	  exit(1);
	}
	dup2(fdv[commandfds[i]], i);
      }

      optind += subargc - 1;
      // TODO: Execute the subcommand
      int rc = fork();
      
      if (rc < 0) {
	
      } else if (rc == 0) {

	// Execute
	int ret = execvp(commandargs[0], commandargs);
	if (ret == -1) {}
	//printf("This should't print out\n");
      } else {
	/* wait(NULL); */
	//printf("All good!\n");
      }      
      break;
    }
    case 'r': {
      //printf("file to be read-only: %s\nfile descriptor: %d\n", optarg, fdcounter);
      // TODO: Possibly more flags
      int ifd = open(optarg, O_RDONLY);
      if (ifd == -1) {
	fprintf(stderr, "Open read-only file failed\n");
	fdcounter++;
	flag = 1;
      }
      else
	fdv[fdcounter++] = ifd;
      if (verboseflag) {
	write(1, "--rdonly ", 9);
	char *buf = optarg;
	int length = 0;
	for (char *curr = buf; *curr; curr++)
	  length++;
	write(1, optarg, length);
	write(1, "\n", 1);
      }
	
	//	printf("--rdonly %s\n", optarg);
      break;
    }
    case 'w': {
      //printf("file to be write-only: %s\nfile descriptor: %d\n", optarg, fdcounter);
      // TODO: Possibly more flags
      int ofd = open(optarg, O_WRONLY);
      if (ofd == -1) {
	fprintf(stderr, "Open write-only file failed\n");
	fdcounter++;
	flag = 1;
      }
      else
	fdv[fdcounter++] = ofd;
      if (verboseflag) {
	write(1, "--wronly ", 9);
	char *buf = optarg;
	int length = 0;
	for (char *curr = buf; *curr; curr++)
	  length++;
	write(1, optarg, length);
	write(1, "\n", 1);
      }
      break;
    }
    case 'v':
      verboseflag = 1;
      break;
    case '?':
      exit(1);
    }
  }
  return flag;
}
