# xv6: Priority Scheduler with Aging and Copy-on-Write Fork

This project extends the **xv6-riscv** educational operating system with
two major kernel modifications:

1. A priority-based scheduler with an aging mechanism to prevent starvation.
2. A Copy-on-Write (COW) `fork()` implementation that defers page copying.

All changes preserve compatibility with xv6's original behavior and have
been validated against both custom tests and the full `usertests` suite.

---

## Overview

The original xv6 scheduler uses a round-robin policy. This project
replaces it with a fixed-priority scheduler that ages waiting processes,
and replaces the eager-copy `fork()` with a COW strategy that shares
physical pages until a write occurs.

## Main Features

- 5-level priority scheduling (0 = highest, 4 = lowest)
- Aging: a process gains one effective priority level every 20 ticks
  while waiting in the `RUNNABLE` state
- `setpriority(int priority)` system call
- COW fork with per-physical-page reference counters
- Copy-on-Write page-fault resolution in `usertrap()` and `copyout()`
- Compatible with single-CPU and multi-CPU configurations
- Passes the complete `usertests -q` regression suite

## Original xv6 Behavior

- **Scheduler**: round-robin across all `RUNNABLE` processes with no
  notion of priority.
- **fork()**: `uvmcopy()` eagerly allocates new physical pages and copies
  all parent mappings into the child.
- **Memory**: no reference counters; `kfree()` immediately returns a page
  to the free list.

## Priority Scheduler with Aging

### Priorities

| Priority | Meaning       |
|--------:|---------------|
| 0        | Highest       |
| 1        | High          |
| 2        | Default       |
| 3        | Low           |
| 4        | Lowest        |

Configuration constants (defined in `kernel/param.h`):

```c
#define PRIORITY_MIN     0
#define PRIORITY_MAX     4
#define DEFAULT_PRIORITY 2
#define AGING_INTERVAL  20   // ticks
```

Child processes inherit the base priority of their parent.

### Aging

To prevent starvation, the scheduler computes an effective priority:

```
effective priority = base priority - wait time / AGING_INTERVAL
```

Every 20 ticks spent in the `RUNNABLE` state, the process effectively
moves up one priority level. The stored base priority is never modified.

### System Call

```c
int setpriority(int priority);
```

Sets the calling process's base priority (0–4). Returns 0 on success,
-1 on error.

Example:

```c
if (setpriority(0) < 0) {
    printf("invalid priority\n");
    exit(1);
}
```

### Aging Test Result

```
High-priority process started: tick=9387
Low-priority process entered:  tick=9388
Low-priority process resumed:  tick=9469  waited=81
High-priority process finished: tick=9507
```

A priority-4 process needs to climb 4 levels:

```
4 levels × 20 ticks = 80 ticks
```

The process resumed after 81 ticks, before the high-priority process
finished — confirming that aging prevents starvation.

## Copy-on-Write Fork

### Original Problem

The original `fork()` calls `uvmcopy()`, which:

1. Allocates new physical pages for the child.
2. Copies all page content from the parent.
3. Maps the copies into the child's page table.

This wastes time and memory, especially when the child calls `exec()`
immediately.

### COW Solution

With Copy-on-Write:

```
fork()
├── parent and child share physical pages
├── write permission is temporarily removed
├── pages are marked with PTECOW
└── actual copying occurs only on a write
```

A reserved PTE bit is used:

```c
#define PTE_COW (1L << 8)
```

### Reference Counters

Each physical page has a reference counter:

| References | Meaning                                     |
|-----------:|---------------------------------------------|
| 1          | Owned by a single address space             |
| 2          | Shared by parent and child                  |
| 3          | Shared by parent, child, and grandchild     |
| 0          | Page is returned to the free list           |

New functions:

```c
void kaddref(void *pa);
int  kgetref(void *pa);
```

`kfree()` only returns a page to the free list when its counter reaches
zero.

### Page-Fault Resolution

When a process writes to a COW page:

```
store page fault
→ usertrap()
→ cowfault()
```

**Multiple references:** allocate a new page, copy the content, map the
private copy, decrement the original counter, restore write permission.

**Single reference:** clear `PTE_COW`, restore `PTE_W`, and continue
without copying.

`copyout()` is also adapted to resolve COW pages when the kernel writes
to user memory.

---

## Development Environment

- **OS**: Fedora 44
- **QEMU**: with RISC-V support (`riscv64-softmmu`)
- **Toolchain**: `riscv64-linux-gnu-*`
- **GCC fix**: `-Wno-error=unused-but-set-variable` added for GCC 16
  compatibility.

## Repository Structure

```
docs/
├── evidence/         screenshots and logs
├── notes/            design and analysis notes
└── results/          priority, aging, and COW results
```

## Modified Files

| File                  | Change                                           |
|-----------------------|--------------------------------------------------|
| `kernel/param.h`      | Priority and aging constants                     |
| `kernel/proc.h`       | `priority` and `ready_since` fields              |
| `kernel/proc.c`       | Scheduler, priority inheritance, state transitions |
| `kernel/syscall.h`    | `setpriority` syscall number                     |
| `kernel/syscall.c`    | Syscall registration                             |
| `kernel/sysproc.c`    | `sys_setpriority` implementation                 |
| `kernel/riscv.h`      | `PTE_COW` bit definition                         |
| `kernel/kalloc.c`     | Reference counters                               |
| `kernel/vm.c`         | `uvmcopy()`, `cowfault()`, `copyout()`           |
| `kernel/trap.c`       | COW store page-fault handling                    |
| `user/schedtest.c`    | Priority scheduling test                         |
| `user/agingtest.c`    | Aging test                                       |
| `user/cowtest.c`      | Copy-on-Write test                               |
| `user/usys.pl`        | `setpriority` stub                               |
| `user/user.h`         | `setpriority` declaration                        |
| `Makefile`            | Test program entries                             |

## Build Instructions

```bash
make clean
make
```

## Running xv6

With one CPU (recommended for deterministic scheduling tests):

```bash
make qemu CPUS=1
```

With multiple CPUs:

```bash
make qemu
```

Exit QEMU: `Ctrl+A` then `X`.

## Tests

Run each test from the xv6 shell:

```
schedtest
```

Tests processes with priorities 0, 2, and 4.

```
agingtest
```

Verifies that a low-priority process resumes after approximately 80 ticks.

```
cowtest
```

Creates a parent, child, and grandchild sharing 16 pages. Each process
writes to its pages and verifies isolation.

```
usertests -q
```

The full xv6 regression suite.

## Expected Results

### Scheduler

- Priority 0 processes are scheduled before priority 2 and 4.
- Processes with the same priority share CPU time fairly.
- Aging lets low-priority processes make progress.
- Works with both 1 and multiple CPUs.

### Copy-on-Write

```
=== Copy-on-Write test ===
Parent initialized 16 pages with P
Grandchild has private copies: OK
Child has private copies: OK
Parent pages remained unchanged: OK
=== Copy-on-Write test passed ===
```

### Full Regression

```
ALL TESTS PASSED
```

---

## Git Branches and Tags

| Tag / Branch                          | Description                        |
|---------------------------------------|------------------------------------|
| `baseline-original`                   | Unmodified MIT xv6-riscv           |
| `baseline-fedora44`                   | Baseline ported to Fedora 44       |
| `scheduler-priority-aging-v1`         | First stable scheduler release     |
| `cow-fork-v1`                         | First stable COW release           |
| `feature/scheduler-priority-aging`    | Final project branch               |

The implementation reference commit is `f83ee36`.

To explore a tag:

```bash
git checkout baseline-original
```

To return to the final version:

```bash
git checkout feature/scheduler-priority-aging
```

---

## Final Report

The full project report is available at:

```
docs/report/Informe_Final_xv6_Omar_Esteban_Agredo.pdf
```

## Video Demonstration

[Video demonstration](https://youtu.be/xQVmq0OP8is)

---

## Author

**Omar Esteban Agredo**  
Course: Operating Systems  
Institution: Universidad del Valle

## Academic Context

This project was developed as the final assignment for the Operating
Systems course at Universidad del Valle. It is based on xv6-riscv and
is intended for educational purposes only.

## References

- [MIT 6.1810 / 6.S081: Operating System Engineering](https://pdos.csail.mit.edu/6.1810/)
- [xv6-riscv source code](https://github.com/mit-pdos/xv6-riscv)
- Lions, John. *Commentary on UNIX 6th Edition*. Peer to Peer
  Communications, 2000.

xv6 is a re-implementation of Unix Version 6 (v6) by Dennis Ritchie and
Ken Thompson, maintained by the MIT PDOS group. All original authors and
contributors retain their respective credits.
