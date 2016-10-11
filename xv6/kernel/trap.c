#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "sysfunc.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;
uint starvTicks;
extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  struct proc* queues [4][NPROC]; // Rule 1
} ptable;
void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  
  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(proc->killed)
      exit();
    proc->tf = tf;
    syscall();
    if(proc->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpu->id == 0){
      acquire(&tickslock);
      ticks++;
      //cprintf("Incrementing tick prio [ticks:%d] [pid:%d] [name:%s]\n", proc->ticks[proc->priority], proc->pid, proc->name);
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpu->id, tf->cs, tf->eip);
    lapiceoi();
    break;
   
  default:
    if(proc == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpu->id, tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            proc->pid, proc->name, tf->trapno, tf->err, cpu->id, tf->eip, 
            rcr2());
    proc->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running 
  // until it gets to the regular system call return.)
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(proc && proc->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER) {
    //cprintf("Yielding from [pid:%d] [name:%s]\n", proc->pid, proc->name);
//    cprintf("[PID:%d],Tick[%d]:[%d]\n", proc->pid, proc->priority, proc->ticks[proc->priority]);
    proc->ticks[proc->priority] = proc->ticks[proc->priority] + 1;  // Rule 4
    proc->accumulatedTicks[proc->priority] = proc->accumulatedTicks[proc->priority] + 1;  // Rule 4
//    cprintf("[PID:%d],Tick[%d]:[%d]\n", proc->pid, proc->priority, proc->ticks[proc->priority]);
    starvTicks++;
    acquire(&ptable.lock);
    struct proc *p = NULL,**q = NULL;
       if (100 == starvTicks) {
      starvTicks = 0;
      uint now = sys_uptime();
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if (p->state == RUNNABLE && now - 100 > p->lastTick) {  // Starvation occurs
          int currPrio = p->priority;
          if (currPrio > 0) {
            p->priority--;
            //cprintf("Priority upgraded from %d -> %d [pid :%d] [name :%s]\n", currPrio , p->priority ,p->pid, p->name);
            for(q = &ptable.queues[currPrio][0]; q < &ptable.queues[currPrio][NPROC]; q++) {
              if (*q == p) {
                *q = NULL;
                break;
              }
            }
            for(q = &ptable.queues[p->priority][0]; q < &ptable.queues[p->priority][NPROC]; q++) {
              if (*q == NULL) {
                *q = p;
                break;
              }
            }
            int j = p->priority;
            for (; j <= currPrio; j++) {
              p->ticks[j] = 0;
            }
          }
        }
      }
    }
    release(&ptable.lock);
    yield();
  }

  // Check if the process has been killed since we yielded
  if(proc && proc->killed && (tf->cs&3) == DPL_USER)
    exit();
}
