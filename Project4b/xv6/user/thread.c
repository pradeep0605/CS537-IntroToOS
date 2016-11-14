#include "types.h"
#include "stat.h"
#include "user.h"
#include "x86.h"

#define PGSIZE (4096)
#define NPROC (64)

typedef struct thread {
  void *stack;
  int tid;
  int in_use;
} thread_t;

thread_t g_threads[NPROC];

thread_t * alloc_thread()
{
  int i = 0;
  for (i = 0; i < NPROC; ++i) {
    if (!g_threads[i].in_use) {
      g_threads[i].in_use = 1;
      return &g_threads[i];
    }
  }
  return NULL;
}

int thread_create(void (*start_routine) (void*), void *data)
{
  thread_t* curr_thread = alloc_thread();
  if (NULL == curr_thread) return -1;
  void* stack = (void*)malloc(PGSIZE);
  memset(stack, 0, PGSIZE);
  if (stack == NULL) {
    return -1;
  }
  curr_thread->stack = stack;
  curr_thread->tid = clone(start_routine, data, stack);
  return curr_thread->tid;
}

int thread_join(int tid)
{
  int i = 0;
  for (i = 0; i < NPROC; ++i) {
    if (g_threads[i].tid == tid) {
      join(&(g_threads[i].stack));
      g_threads[i].in_use = 0;
      free(g_threads[i].stack);
      break;
    }
  }
  return g_threads[i].tid;
}

void lock_init(lock_t* lock)
{
  lock->locked = 0;
}

void lock_acquire(lock_t* lock)
{
  while(xchg(&lock->locked, 1) != 0)
    ;
}

void lock_release(lock_t* lock)
{
  lock->locked = 0; 
}
