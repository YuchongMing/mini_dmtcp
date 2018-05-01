#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>
#include "lib.h"

int flag = 0;
ucontext_t ucp;

void handle_signal(int signal);
void gen_ckpt();
void write_header(int fd);
void write_memory(int fd);

__attribute__((constructor)) void
preload()
{
  struct sigaction sa;
  printf("The pid is: %d\n", getpid());

  sa.sa_handler = &handle_signal;
  sa.sa_flags = SA_RESTART;
  sigfillset(&sa.sa_mask);

  if (sigaction(SIGUSR2, &sa, NULL) == -1) {
    perror("Error: cannot handle SIGUSR2");
  }
}

/**
 * Listening to the SIGUSR2 signal, and execute the gen_ckpt() when the process 
 * recieves the SIGUSR2 signal.
 */
void
handle_signal(int signal)
{
  const char* signal_name;
  if (signal == SIGUSR2) {
    signal_name = "SIGUSR2";
  } else {
    fprintf(stderr, "Caught wrong signal: %d\n", signal);
    return;
  }
  printf("\nCatch signal %s, generating check point...\n", signal_name);
  gen_ckpt();
  return;
}
/**
 * Generate the binary checkpoint file "myckpt" of the current running process. 
 * The information includes the context on cpu and the memory section.
 */
void
gen_ckpt()
{
  int ckpt_fd = creat("myckpt", S_IRWXU);
  if (ckpt_fd == -1) {
    printf("Error: Cannot open file, %s\n", strerror(errno));
    exit(1);
  }
  int gc = getcontext(&ucp);
  if (flag == 1) {
    return;
  }
  flag = 1;
  if (gc == -1) {
    fprintf(stderr, "\nError: Cannot get register value\n");
    return;
  }
  int fw = write(ckpt_fd, &ucp, sizeof(ucontext_t));
  if (fw == -1) {
    fprintf(stderr, "\nError: Fail to write the register to the file\n");
  }
  write_memory(ckpt_fd);

  close(ckpt_fd);
}
/**
 * Writing the memory section of the process into the file.
 */
void
write_memory(int ckpt_fd)
{
  char* path = "/proc/self/maps";
  int maps_fd = open(path, O_RDONLY);
  if (maps_fd == -1) {
    printf("Cannot open memory map file. \n");
  }

  MR* mr;
  char* sAddr;
  char* eAddr;
  int len = 300;
  char buffer[len];

  while (ckpt_getline(maps_fd, buffer, len) > 0) {
    char* line = &buffer[0];
    if (strstr(line, "[vsyscall]") != NULL || strstr(line, "[vvar]") != NULL) {
      continue;
    }

    mr = malloc(sizeof(*mr));
    int fstIndex = indexOf(line, '-');
    int secIndex = indexOf(line, ' ');
    sAddr = malloc(1 + fstIndex);
    eAddr = malloc(secIndex - fstIndex);
    strncpy(sAddr, line, fstIndex);
    strncpy(eAddr, line + fstIndex + 1, secIndex - fstIndex - 1);

    mr->startAddr = (void*)strtoul(sAddr, NULL, 16);
    mr->endAddr = (void*)strtoul(eAddr, NULL, 16);
    mr->size = mr->endAddr - mr->startAddr;
    line[secIndex + 1] == '-' ? (mr->isReadable = 0) : (mr->isReadable = 1);
    line[secIndex + 2] == '-' ? (mr->isWriteable = 0) : (mr->isWriteable = 1);
    line[secIndex + 3] == '-' ? (mr->isExecutable = 0) : (mr->isExecutable = 1);

    if (mr->isReadable == 0) {
      free(mr);
      free(sAddr);
      free(eAddr);
      continue;
    }

    int fwh = write(ckpt_fd, mr, sizeof(MR));
    if (fwh == -1) {
      fprintf(stderr, "\nError: Fail to write the memory header to the file\n");
      printf("Start address: %p\n", mr->startAddr);
    }

    int fwb = write(ckpt_fd, mr->startAddr, mr->size);
    if (fwb == -1) {
      fprintf(stderr, "\nFail to write the memory data to the file\n");
      printf("Start address: %p\n", mr->startAddr);
      printf("%s", line);
    }
    free(mr);
    free(sAddr);
    free(eAddr);
  }

  close(maps_fd);
}