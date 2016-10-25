#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "devices/shutdown.h"
#include "devices/input.h"
#include "lib/kernel/console.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);
void syscall_halt(void);
void syscall_exit(int status);
pid_t syscall_exec(const char *cmd_line);
int syscall_wait(pid_t pid);
int syscall_read(int fd, void *buffer, unsigned size);
int syscall_write(int fd, const void *buffer, unsigned size);
int syscall_fibonacci(int n);
int syscall_sum_of_four_integers(int a, int b, int c, int d);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int sysnum = *(int *)f->esp;
  
  printf ("system call!\n");
  thread_exit ();

  // need to handle a variety of system calls
  // access to system call by esp pointer in intr_frame
}


void
syscall_halt(void)
{
  shutdown_power_off();
  return;
}

void 
syscall_exit(int status)
{
  struct thread *cur = thread_current();
  
  // TODO
  printf("Terminating the current user program!\n");
  thread_exit();
  return;
}

pid_t
syscall_exec(const char *cmd_line)
{
  return process_execute(cmd_line);
  // type casting...? tid_t -> pid_t
}

int
syscall_wait(pid_t pid)
{
  return process_wait((tid_t)pid);
  // need to add more codes on process_wait
}

int
syscall_read(int fd, void *buffer, unsigned size)
{
  unsigned i=0;
  if(!fd)
  {
    do{
      *((uint8_t *)buffer + i) = input_getc();
      i++;
    }while(i<size && *((uint8_t *)buffer + i) != '\n');
  }
  return i;
}

int
syscall_write(int fd, const void *buffer, unsigned size)
{
  if(fd == 1)
  {
    putbuf((char *)buffer,(size_t)size);
    return size;
  }
  // if any error occurs, change to iteration
  // increasing the size by i
}

int
syscall_fibonacci(int n)
{
  int pre1 = 0, pre2 = 1, i;
  int result = 0;
  if(n == 1)
    return pre2;
  for(i=1; i<n; i++)
  {
    result = pre1 + pre2;
    pre1 = pre2;
    pre2 = result;
  }
  return result;
}
int
syscall_sum_of_four_integers(int a, int b, int c, int d)
{
  return a+b+c+d;
}
