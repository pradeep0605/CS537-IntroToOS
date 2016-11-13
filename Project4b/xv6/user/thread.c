#include "types.h"
#include "stat.h"
#include "user.h"

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
  printf(1, "USER: func = %p, arg = %d, stack = %d\n", start_routine, *(int*)data, stack + 4096);
  curr_thread->tid = clone(start_routine, data, stack);
  return curr_thread->tid;
}

int thread_join(int tid)
{
  int i = 0;
  for (i = 0; i < NPROC; ++i) {
    if (g_threads[i].tid == tid) {
      g_threads[i].in_use = 0;
      join(&(g_threads[i].stack));
      free(g_threads[i].stack);
      break;
    }
  }
  return g_threads[i].tid;
}
