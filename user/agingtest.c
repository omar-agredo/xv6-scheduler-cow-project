#include "kernel/types.h"
#include "user/user.h"

#define HIGH_RUNTIME 120

static void
high_priority_work(void)
{
  int start = uptime();

  printf("High-priority process started: pid=%d tick=%d\n",
         getpid(), start);

  while (uptime() - start < HIGH_RUNTIME) {
    for (volatile int i = 0; i < 100000; i++)
      ;
  }

  printf("High-priority process finished: pid=%d tick=%d\n",
         getpid(), uptime());

  exit(0);
}

static void
low_priority_work(void)
{
  int before_sleep;
  int resumed;

  before_sleep = uptime();

  printf("Low-priority process entered: pid=%d tick=%d\n",
         getpid(), before_sleep);

  // Sleep briefly so the process later becomes RUNNABLE while
  // the priority-0 process is still consuming the CPU.
  pause(1);

  resumed = uptime();

  printf("Low-priority process resumed: pid=%d tick=%d waited=%d\n",
         getpid(),
         resumed,
         resumed - before_sleep);

  exit(0);
}

int
main(void)
{
  int gate[2];
  int high_pid;
  int low_pid;
  char signal;

  printf("=== Scheduler aging test ===\n");
  printf("The priority-4 process should resume through aging while priority 0 remains active.\n");

  if (pipe(gate) < 0) {
    printf("agingtest: pipe failed\n");
    exit(1);
  }

  high_pid = fork();

  if (high_pid < 0) {
    printf("agingtest: fork failed\n");
    exit(1);
  }

  if (high_pid == 0) {
    close(gate[1]);

    if (setpriority(0) < 0)
      exit(1);

    read(gate[0], &signal, 1);
    high_priority_work();
  }

  low_pid = fork();

  if (low_pid < 0) {
    printf("agingtest: fork failed\n");
    exit(1);
  }

  if (low_pid == 0) {
    close(gate[1]);

    if (setpriority(4) < 0)
      exit(1);

    read(gate[0], &signal, 1);
    low_priority_work();
  }

  close(gate[0]);

  write(gate[1], "G", 1);
  write(gate[1], "G", 1);
  close(gate[1]);

  wait(0);
  wait(0);

  printf("=== Aging test finished ===\n");
  exit(0);
}
