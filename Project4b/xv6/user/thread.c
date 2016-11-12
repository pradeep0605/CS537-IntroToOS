#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE  (4096)
#define NPROC (64)

typedef struct thread {
  void (*func) (void*);
  void *arg;
  void *stack;
  int tid;
  int in_use;
} thread_t;

thread_t zero;
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

void free_thread(thread_t* thread)
{
  *thread = zero;
}

int thread_create(void (*func) (void*), void *data)
{
  thread_t *thread = alloc_thread();
  if (thread == NULL) {
    return -1;
  }

  thread->stack = (void *) malloc(PGSIZE);
  memset(thread->stack, 0, PGSIZE);
  if (thread->stack == NULL) {
    free_thread(thread);
    return -1;
  }

  thread->func = func;
  thread->arg = data;

  printf(1, "USER: func = %d, arg = %d, stack = %d\n", thread->func,
    thread->arg, thread->stack);
  thread->tid = clone(thread->func, thread->arg, thread->stack);

  return thread->tid;
}  









