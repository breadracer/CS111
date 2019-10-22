/* NAME: Shawn Ma */
/* EMAIL: breadracer@outlook.com */
/* ID: 204996814 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <mraa/gpio.h>
#include <mraa/aio.h>
#include <getopt.h>
#include <poll.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

sig_atomic_t volatile run_flag = 1;

char const CENTEGRADE = 'C';
char const FAHRENHEIT = 'F';

int const B = 4275;               // B value of the thermistor
int const R0 = 100000;            // R0 = 100k

enum cmd_line_options {SCALE, PERIOD, LOG};

struct states {
  char volatile scale_flag;
  int volatile period;
  int volatile stop_flag;
  int log_flag;
  int buf_tail;
  int buf_head;
  int lenbuf;
};

// helper functions
void error(char *message, int code) {
  fprintf(stderr, "%s\n", message);
  exit(code);
}

void interruption_handler(void *arg) {
  (void)arg;
  run_flag = 0;
}

void sig_handler(int sig) {
  if (sig == SIGINT)
    run_flag = 0;
}

float ctof(float c) {
  return c * 9.0 / 5.0 + 32.0;
}

float get_temperature(int analog_read) {
  float R = 1023.0 / analog_read - 1.0;
  R = R0 * R;
  float temperature = 1.0 / (log(R / R0) / B + 1.0 / 298.15) - 273.15; 
  return temperature;
}

int main(int argc, char **argv) {
  // declare and initialize variables
  struct states state = {FAHRENHEIT, 1, 0, 0, 0, 0, 2};
  char *buffer = malloc(sizeof(char) * 32);
  if (!buffer)
    error("malloc error", 1);
  
  // configure command-line options
  int opt, optindex;
  struct option options[] = {
    {"scale", required_argument, 0, SCALE},
    {"period", required_argument, 0, PERIOD},
    {"log", required_argument, 0, LOG},
    {0, 0, 0, 0}    
  };  

  // command line options processing
  while (1) {
    opt = getopt_long(argc, argv, "", options, &optindex);
    if (opt == -1)
      break;
    switch (opt) {
    case SCALE:
      if (strlen(optarg) > 1 || (optarg[0] != CENTEGRADE && optarg[0] != FAHRENHEIT))
	error("Invalid scale", 1);
      state.scale_flag = optarg[0];
      break;
    case PERIOD:
      if (atoi(optarg) <= 0)
	error("Invalid period number", 1);
      state.period = atoi(optarg);
      break;
    case LOG: {
      int fd = open(optarg, O_CREAT | O_WRONLY | O_TRUNC, 0666);
      if (fd == -1)
	error("open failed", 1);
      dup2(fd, STDOUT_FILENO);
      state.log_flag = 1;
      break;
    }
    case '?':
      error("Invalid command-line parameter", 1);
    }
  }

  // get input
  uint16_t value;
  mraa_gpio_context button;
  mraa_aio_context sensor;
  button = mraa_gpio_init(60);
  sensor = mraa_aio_init(1);
  mraa_gpio_dir(button, MRAA_GPIO_IN);
  mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, interruption_handler, NULL);

  signal(SIGINT, sig_handler);
  
  struct pollfd stdin_fd = {STDIN_FILENO, POLLIN, 0};
  while (run_flag) {
    value = mraa_aio_read(sensor);
    time_t now = time(0);
    char time_str[9];
    int poll_flag = 1;
    while (poll_flag) {
      poll(&stdin_fd, 1, 0);
      if (stdin_fd.revents & POLLIN) {
	char buf;
	if (read(STDIN_FILENO, &buf, 1)) {
	  buffer[state.buf_tail++] = buf;
	  if (state.buf_tail == state.lenbuf - 1) {
	    char *new_buffer = realloc(buffer, sizeof(char) * state.lenbuf * 2);
	    if (!new_buffer)
	      error("realloc error", 1);
	    buffer = new_buffer;
	    state.lenbuf *= 2;
	  }
	  buffer[state.buf_tail] = 0;
	}
	// check arguments
	if (buf == '\n') {
	  if (state.log_flag)
	    printf("%s", buffer + state.buf_head);
	  if (strcmp(buffer + state.buf_head, "OFF\n") == 0)
	    run_flag = 0;
	  else if (strcmp(buffer + state.buf_head, "SCALE=C\n") == 0)
	    state.scale_flag = CENTEGRADE;
	  else if (strcmp(buffer + state.buf_head, "SCALE=F\n") == 0)
	    state.scale_flag = FAHRENHEIT;
	  else if (strcmp(buffer + state.buf_head, "STOP\n") == 0)
	    state.stop_flag = 1;
	  else if (strcmp(buffer + state.buf_head, "START\n") == 0)
	    state.stop_flag = 0;	  
	  else if (strstr(buffer + state.buf_head, "PERIOD=") == buffer + state.buf_head) {
	    int length = state.buf_tail - state.buf_head;
	    char *period_str = malloc(sizeof(char) * (length - 7));
	    if (!period_str)
	      error("malloc error", 1);
	    strncpy(period_str, buffer + state.buf_head + 7, length - 8);
	    period_str[length - 8] = 0;
	    int period = atoi(period_str);
	    if (period > 0)
	      state.period = period;
	  }
	  else if (!strstr(buffer + state.buf_head, "LOG "))
	    fprintf(stderr, "Invalid command: %s", buffer + state.buf_head);
	  state.buf_head = state.buf_tail;
	}
      }
      if (time(0) - now >= state.period && !state.stop_flag)
	poll_flag = 0;
      if (run_flag == 0) {
	char t_str_shutdown[9];
	time_t t_shutdown = time(0);
	strftime(t_str_shutdown, sizeof(t_str_shutdown), "%T", localtime(&t_shutdown));
	printf("%s SHUTDOWN\n", t_str_shutdown);
	mraa_aio_close(sensor);
	mraa_gpio_close(button);
	exit(0);
      }
    }

    // report on temperature
    now = time(0);
    strftime(time_str, sizeof(time_str), "%T", localtime(&now));
    float temperature = get_temperature(value);
    if (state.scale_flag == FAHRENHEIT)
      temperature = ctof(temperature);
    printf("%s %.1f\n", time_str, temperature);
    fflush(stdout);
  }

  mraa_aio_close(sensor);
  mraa_gpio_close(button);
  return 0;
}
