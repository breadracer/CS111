#include <stdio.h>
#include <time.h>

int main(void) {
  time_t now = time(0);
  char time_str[9];
  strftime(time_str, sizeof(time_str), "%T", localtime(&now));
  printf("%s", time_str);
  return 0;
}
