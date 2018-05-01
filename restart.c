#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>
#include "lib.h"

#define NEW_STACK_ADDR 0x5300000;
#define NEW_STACK_END 0x5301000;

ucontext_t ucp;
char ckpt_image[1000];

void restore_memory();
void map_ckpt(int fd);
void unmap_org_stack();

/**
 * Program entrance, mmap new memory section for the checkpoint image file.
 */
int
main(int argc, char const* argv[])
{
  if (argc == 1 || argc > 2) {
    fprintf(stderr, "\nSyntax: ./retore file_path\n");
    return 1;
  }
  strcpy(ckpt_image, argv[1]);

  void* stack = (void*)NEW_STACK_ADDR;
  void* end = (void*)NEW_STACK_END;
  void* mstack = mmap(stack, end - stack, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  if (mstack == (void*)-1) {
    int errsv = errno;
    printf("mmap() failed. Failed to map new stack, errno: %d\n", errno);
  }
  asm volatile("mov %0,%%rsp" : : "g"(end) : "memory");
  restore_memory();
}

/**
 * Restore the memory from checkpoint image file, than restore the context onto 
 * cpu.
 */
void
restore_memory()
{
  unmap_org_stack();

  int mem_fd = open(ckpt_image, O_RDONLY);
  if (mem_fd == -1) {
    fprintf(stderr, "\nError opening file\n");
    exit(1);
  }

  int fducp = read(mem_fd, &ucp, sizeof(ucontext_t));
  if (fducp < sizeof(ucontext_t)) {
    fprintf(stderr, "\nError: Fail to read register into data segment\n");
  }

  map_ckpt(mem_fd);

  close(mem_fd);
  int st = setcontext(&ucp);
  if (st == -1) {
    printf("Error: Fail to load register");
  }
}

/**
 * Read the addresses of each memory segment from the checkpoint image file, 
 * map it into a corresponding memory section for the new process. 
 */
void
map_ckpt(int mem_fd)
{
  MR* mr = malloc(sizeof(*mr));
  int fdmr = 0;
  while ((fdmr = read(mem_fd, mr, sizeof(*mr))) > 0) {
    if (fdmr < sizeof(*mr)) {
      fprintf(stderr, "\nError: Fail to read mem header\n");
      printf("Start address: %p\n", mr->startAddr);
    }
    mmap(mr->startAddr, mr->size, PROT_READ | PROT_WRITE | PROT_EXEC,
         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

    int fdmem = read(mem_fd, mr->startAddr, mr->size);
    if (fdmem < mr->size) {
      fprintf(stderr, "\nError: Fail to read file data into the memory\n");
      printf("Start address: %p\n", mr->startAddr);
    }

    int prot = PROT_READ;
    if (mr->isWriteable && mr->isExecutable) {
      prot |= PROT_WRITE | PROT_EXEC;
    } else if (mr->isWriteable) {
      prot |= PROT_WRITE;
    } else if (mr->isExecutable) {
      prot |= PROT_EXEC;
    }

    int mp = mprotect(mr->startAddr, mr->size, prot);
    if (mp == -1) {
      fprintf(stderr, "\nError: Fail to change the premission of the memory\n");
      printf("Start address: %p\n", mr->startAddr);
    }
  }
}

/**
 * Unmap the stack used by the myrestart program. The reason is that the stack 
 * might be located at an address range that conflicts with what the target 
 * program was using prior to checkpointing.
 */
void
unmap_org_stack()
{
  char* path = "/proc/self/maps";
  int maps_fd = open(path, O_RDONLY);
  if (maps_fd == -1) {
    printf("Cannot open memory map file. \n");
  }
  int len = 300;
  char buffer[len];
  while (ckpt_getline(maps_fd, buffer, len) > 0) {
    char* line = &buffer[0];
    if (strstr(line, "[stack]") == NULL) {
      continue;
    }
    int fstIndex = indexOf(line, '-');
    int secIndex = indexOf(line, ' ');
    char* sAddr = malloc(1 + fstIndex);
    char* eAddr = malloc(secIndex - fstIndex);
    strncpy(sAddr, line, fstIndex);
    strncpy(eAddr, line + fstIndex + 1, secIndex - fstIndex - 1);
    void* startAddr = (void*)strtoul(sAddr, NULL, 16);
    void* endAddr = (void*)strtoul(eAddr, NULL, 16);
    free(sAddr);
    free(eAddr);
    int mun = munmap(startAddr, endAddr - startAddr);
    if (mun != 0) {
      fprintf(stderr, "\nError: Fail to unmap the orginal stack\n");
    }
    close(maps_fd);
    return;
  }
  close(maps_fd);
  fprintf(stderr, "\nError: Fail to unmap the orginal stack\n");
}
