#include "userprog/syscall.h"
#include "userprog/process.h"
#include "lib/user/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <stdbool.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#include "devices/shutdown.h"
#include "devices/input.h"
#include "lib/kernel/console.h"

static void syscall_handler (struct intr_frame *);
void syscall_halt(void);
void syscall_exit(int status);
pid_t syscall_exec(const char *cmd_line);
int syscall_wait(pid_t pid);
int syscall_read(int fd, void *buffer, unsigned size);
int syscall_write(int fd, const void *buffer, unsigned size);
int syscall_fibonacci(int n);
int syscall_sum_of_four_integers(int a, int b, int c, int d);

/* syscall handler helper */
// check argc and argv is valid virtual address using esp
// We only use this function in syscall.c
static bool is_valid_arg (const void* esp);
static bool is_valid_arg3 (const void* esp);

#ifndef __ESP_ARGV__
#define ESP_ARGV_PTR(ESP, INDEX) ((void*)((uintptr_t)(ESP) + ((INDEX) + 3) * 4)) // 4 means sizeof(uintptr_t)
#define ESP_ARGC_PTR(ESP) ((void*)((uintptr_t)(ESP) + 4))
#define ESP_ARGV3_PTR(ESP, INDEX) ((void*)((uintptr_t)(ESP) + ((INDEX) + 5) * 4)) 
#define ESP_ARGC3_PTR(ESP) ((void*)((uintptr_t)(ESP) + 20))
#endif

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f/* UNUSED*/) 
{
  int sysnum = *(int *)f->esp;
  void *temp_esp = f->esp;

  /* Debugging */
  hex_dump((uintptr_t)f->esp, (const char*)f->esp, 330, 1);

  // check that syscall number is valid
  if (-1 < sysnum && sysnum < NUM_SYSCALL)
    {
      switch(sysnum)
        {
        case SYS_HALT:
            {
              syscall_halt ();
            }
          break;
        case SYS_EXIT:
            {
              if (!is_valid_arg(temp_esp))
                {
                  thread_exit();
                  break;
                }
              syscall_exit(*(int*)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_EXEC:
            {
              if (!is_valid_arg(temp_esp))
                {
                  thread_exit();
                  break;
                }
              syscall_exec(*(char**)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_WAIT:
            {
              if (!is_valid_arg (temp_esp))
                {
                  thread_exit();
                  break;
                }
              syscall_wait(*(pid_t*)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_READ:
            {
              if (!is_valid_arg3 (temp_esp))
                {
                  thread_exit();
                  break;
                }
              syscall_read(*(int*)ESP_ARGV3_PTR(temp_esp, 0),
                           (void*)*(int*)ESP_ARGV3_PTR(temp_esp, 1),
                           (size_t)*(int*)ESP_ARGV3_PTR(temp_esp, 2)
                           );
            }
          break;
        case SYS_WRITE:
            {
              if (!is_valid_arg3(temp_esp))
                {
                  thread_exit();
                  break;
                }
              syscall_write (*(int*)ESP_ARGV3_PTR(temp_esp, 0),
                             (void*)*(int*)ESP_ARGV3_PTR(temp_esp, 1),
                             (size_t)*(int*)ESP_ARGV3_PTR(temp_esp, 2)
                             );
            }
          break;
        case SYS_FIBO: // Calculate fibonacci
            {
              if (!is_valid_arg(temp_esp))
                {
                  thread_exit();
                  break;
                }
              syscall_fibonacci (*(int)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_SUM: // Sum Of four integers
            {
              if (if(is_valid_arg3(temp_esp)))
                {
                  thread_exit();
                  break;
                }
              syscall_sum_of_four_integers(*(int*)ESP_ARGV3_PTR(temp_esp, 0),
                                           *(int*)ESP_ARGV3_PTR(temp_esp, 1),
                                           *(int*)ESP_ARGV3_PTR(temp_esp, 2),
                                           *(int*)ESP_ARGV3_PTR(temp_esp, 3),
                                           );
            }
          break;
        }
    }
  else
    thread_exit();


//  printf ("system call!\n");
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

// ex) is_valid = is_valid_arg (f->esp);
static bool
is_valid_arg (const void* esp)
{
  int i;
  void* argv_ptr = NULL;
  int argc = 0;

  if (!is_user_vaddr(ESP_ARGC_PTR(esp)))
      return false;

  // the ptr of argc is User Vaddr!
  argc = *(int*)ESP_ARGC_PTR(esp);

  for (i = 0; i < argc; i++)
    {
      argv_ptr = ESP_ARGV_PTR(esp, i); // ith argv pointer
      
      if (!is_user_vaddr(argv_ptr) || !is_user_vaddr((void*)(*(char**)argv_ptr)))
        return false;
    }

  return true;
}

static bool
is_valid_arg3 (const void* esp)
{
  int i;
  void* argv_ptr = NULL;
  int argc = 3;

  for (i = 0; i < argc; i++)
    {
      argv_ptr = ESP_ARGV3_PTR(esp, i); // ith argv pointer
      
      if (!is_user_vaddr(argv_ptr) || !is_user_vaddr((void*)(*(char**)argv_ptr)))
        return false;
    }

  return true;
}
