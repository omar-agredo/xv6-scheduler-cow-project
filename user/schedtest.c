#include "kernel/types.h"
#include "user/user.h"

#define CHILDREN 3
#define WORK_ITERATIONS 60000000

static void
cpu_work(void)
{
  volatile uint64 result = 0;

  for (uint64 i = 0; i < WORK_ITERATIONS; i++)
    result += i % 7;

  // Prevent the compiler from considering the work unused.
  if (result == 0)
    printf("unexpected result\n");
}

static void
run_child(int number, int priority, int ready_fd, int gate_fd)
{
  char signal = 'R';
  char start_signal;
  int start_tick;
  int finish_tick;

  if (setpriority(priority) < 0) {
    printf("Child %d: invalid priority %d\n", number, priority);
    exit(1);
  }

  // Tell the parent that this child configured its priority.
  write(ready_fd, &signal, 1);

  // Wait until every child is ready.
  if (read(gate_fd, &start_signal, 1) != 1) {
    printf("Child %d: gate error\n", number);
    exit(1);
  }

  start_tick = uptime();

  printf("Child %d started: pid=%d priority=%d tick=%d\n",
         number, getpid(), priority, start_tick);

  cpu_work();

  finish_tick = uptime();

  printf("Child %d finished: pid=%d priority=%d tick=%d elapsed=%d\n",
         number,
         getpid(),
         priority,
         finish_tick,
         finish_tick - start_tick);

  exit(0);
}

int
main(void)
{
  int ready_pipe[2];
  int gate_pipe[2];
  int priorities[CHILDREN] = {0, 2, 4};
  char signal;

  printf("=== Priority scheduler test ===\n");
  printf("Priority 0 = highest, priority 4 = lowest\n");

  if (pipe(ready_pipe) < 0 || pipe(gate_pipe) < 0) {
    printf("schedtest: pipe failed\n");
    exit(1);
  }

  for (int i = 0; i < CHILDREN; i++) {
    int pid = fork();

    if (pid < 0) {
      printf("schedtest: fork failed\n");
      exit(1);
    }

    if (pid == 0) {
      close(ready_pipe[0]);
      close(gate_pipe[1]);

      run_child(i + 1,
                priorities[i],
                ready_pipe[1],
                gate_pipe[0]);
    }
  }

  close(ready_pipe[1]);
  close(gate_pipe[0]);

  // Wait until all children have configured their priorities.
  for (int i = 0; i < CHILDREN; i++) {
    if (read(ready_pipe[0], &signal, 1) != 1) {
      printf("schedtest: ready signal failed\n");
      exit(1);
    }
  }

  printf("All children ready. Releasing workload...\n");

  // Release all children at approximately the same moment.
  for (int i = 0; i < CHILDREN; i++)
    write(gate_pipe[1], "G", 1);

  close(ready_pipe[0]);
  close(gate_pipe[1]);

  for (int i = 0; i < CHILDREN; i++)
    wait(0);

  printf("=== All children finished ===\n");
  exit(0);
}
