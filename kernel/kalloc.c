// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// One reference counter for every possible physical page.
static int refcount[PHYSTOP / PGSIZE];

void
kinit()
{
  initlock(&kmem.lock, "kmem");

  // freerange() initializes every usable page through kfree().
  // Start each page at one reference so kfree() can reduce it to zero.
  for (uint64 pa = PGROUNDUP((uint64)end);
       pa + PGSIZE <= PHYSTOP;
       pa += PGSIZE)
    refcount[pa / PGSIZE] = 1;

  freerange(end, (void *)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  uint64 index;

  if (((uint64)pa % PGSIZE) != 0 ||
      (char *)pa < end ||
      (uint64)pa >= PHYSTOP)
    panic("kfree");

  index = (uint64)pa / PGSIZE;

  acquire(&kmem.lock);

  if (refcount[index] < 1)
    panic("kfree refcount");

  refcount[index]--;

  // Another page table still refers to this physical page.
  if (refcount[index] > 0) {
    release(&kmem.lock);
    return;
  }

  // Fill with junk only when the last reference disappears.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;
  r->next = kmem.freelist;
  kmem.freelist = r;

  release(&kmem.lock);
}


// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

acquire(&kmem.lock);
r = kmem.freelist;
if (r) {
  kmem.freelist = r->next;
  refcount[(uint64)r / PGSIZE] = 1;
}
release(&kmem.lock);

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk
  return (void *)r;
}

// Add one reference to a shared physical page.
void
kaddref(void *pa)
{
  uint64 index = (uint64)pa / PGSIZE;

  if (((uint64)pa % PGSIZE) != 0 ||
      (char *)pa < end ||
      (uint64)pa >= PHYSTOP)
    panic("kaddref");

  acquire(&kmem.lock);

  if (refcount[index] < 1)
    panic("kaddref refcount");

  refcount[index]++;

  release(&kmem.lock);
}

// Return the current number of references to a physical page.
int
kgetref(void *pa)
{
  int count;
  uint64 index = (uint64)pa / PGSIZE;

  if (((uint64)pa % PGSIZE) != 0 ||
      (char *)pa < end ||
      (uint64)pa >= PHYSTOP)
    panic("kgetref");

  acquire(&kmem.lock);
  count = refcount[index];
  release(&kmem.lock);

  return count;
}


