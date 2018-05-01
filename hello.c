#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char const* argv[])
{
  while (1) {
    printf(".");
    fflush(stdout);
    sleep(1);
  }
  return 0;
}