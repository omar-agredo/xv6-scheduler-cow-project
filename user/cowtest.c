#include "kernel/types.h"
#include "user/user.h"

#define PAGES 16
#define PAGE_SIZE 4096

int
main(void)
{
  char *memory;
  int pid;
  int status = 0;

  printf("=== Copy-on-Write test ===\n");

  memory = sbrk(PAGES * PAGE_SIZE);

  if (memory == (char *)-1) {
    printf("cowtest: sbrk failed\n");
    exit(1);
  }

  // Force every page to exist before fork().
  for (int i = 0; i < PAGES; i++)
    memory[i * PAGE_SIZE] = 'P';

  printf("Parent initialized %d pages with P\n", PAGES);

  pid = fork();

  if (pid < 0) {
    printf("cowtest: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    int grandchild;

    grandchild = fork();

    if (grandchild < 0) {
      printf("cowtest: grandchild fork failed\n");
      exit(1);
    }

    if (grandchild == 0) {
      // The grandchild writes its own private copies.
      for (int i = 0; i < PAGES; i++)
        memory[i * PAGE_SIZE] = 'G';

      for (int i = 0; i < PAGES; i++) {
        if (memory[i * PAGE_SIZE] != 'G') {
          printf("Grandchild verification failed on page %d\n", i);
          exit(1);
        }
      }

      printf("Grandchild has private copies: OK\n");
      exit(0);
    }

    // The child writes its own private copies.
    for (int i = 0; i < PAGES; i++)
      memory[i * PAGE_SIZE] = 'C';

    wait(&status);

    if (status != 0) {
      printf("cowtest: grandchild failed\n");
      exit(1);
    }

    for (int i = 0; i < PAGES; i++) {
      if (memory[i * PAGE_SIZE] != 'C') {
        printf("Child verification failed on page %d\n", i);
        exit(1);
      }
    }

    printf("Child has private copies: OK\n");
    exit(0);
  }

  wait(&status);

  if (status != 0) {
    printf("cowtest: child failed\n");
    exit(1);
  }

  // The child and grandchild must not change the parent's pages.
  for (int i = 0; i < PAGES; i++) {
    if (memory[i * PAGE_SIZE] != 'P') {
      printf("Parent page changed unexpectedly: page %d\n", i);
      exit(1);
    }
  }

  printf("Parent pages remained unchanged: OK\n");
  printf("=== Copy-on-Write test passed ===\n");

  exit(0);
}
