#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define CHILDREN 3
#define WORK 30000000

int
main(void)
{
  int i;

  printf("=== Scheduler baseline test ===\n");

  for (i = 0; i < CHILDREN; i++) {
    int pid = fork();

    if (pid < 0) {
      printf("Error: fork failed\n");
      exit(1);
    }

    if (pid == 0) {
      volatile unsigned long counter;

      printf("Child %d started: pid=%d\n", i + 1, getpid());

      for (counter = 0; counter < WORK; counter++) {
        // CPU-intensive work
      }

      printf("Child %d finished: pid=%d\n", i + 1, getpid());
      exit(0);
    }
  }

  for (i = 0; i < CHILDREN; i++) {
    wait(0);
  }

  printf("=== All children finished ===\n");
  exit(0);
}
