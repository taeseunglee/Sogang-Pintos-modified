#include "userprog/syscall.h"
#include "userprog/process.h"
#include "userprog/exception.h"
#include "lib/user/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <stdbool.h>
#include <string.h>
#include "threads/malloc.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#include "devices/shutdown.h"
#include "devices/input.h"
#include "lib/kernel/console.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);
void syscall_halt(void);
void syscall_exit(int status);
pid_t syscall_exec(const char *cmd_line);
int syscall_wait(pid_t pid);
int syscall_read(int fd, void *buffer, unsigned size);
int syscall_write(int fd, const void *buffer, unsigned size);
int syscall_fibonacci(int n);
int syscall_sum_of_four_integers(int a, int b, int c, int d);

bool syscall_create(const char *file, unsigned initial_size);
bool syscall_remove(const char *file);
int syscall_open(const char *file);
int syscall_filesize(int fd);
void syscall_seek(int fd, unsigned position);
unsigned syscall_tell(int fd);
void syscall_close(int fd);

/* syscall handler helper */
// check argc and argv is valid virtual address using esp
// We only use this function in syscall.c
static bool is_valid_arg (const void* esp, int argc);
static bool is_valid_arg3 (const void* esp, int argc);
struct file* search_file(int fd);

#ifndef __ESP_ARGV__

#define __ESP_ARGV__
#define ESP_ARGV_PTR(ESP, INDEX) ((void*)((uintptr_t)(ESP) + ((INDEX) + 1) * 4)) // 4 means sizeof(uintptr_t)
// #define ESP_ARGC_PTR(ESP) ((void*)((uintptr_t)(ESP) + 4))  // DELETE?
#define ESP_ARGV3_PTR(ESP, INDEX) ((void*)((uintptr_t)(ESP) + ((INDEX) + 5) * 4)) 
// #define ESP_ARGC3_PTR(ESP) ((void*)((uintptr_t)(ESP) + 20))

#endif

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f /* UNUSED */) 
{
  int sysnum = *(int *)f->esp;
  void *temp_esp = f->esp;

  /* Debugging */
  //  hex_dump((uintptr_t)f->esp, (const char*)f->esp, 330, 1);

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
              if (!is_valid_arg(temp_esp, 1))
                {
                  syscall_exit(-1);
                  break;
                }
              //              hex_dump((uintptr_t)f->esp, (const char*)f->esp, 330, 1);
//              printf("[syscall_handler]EXIT!\n");
              syscall_exit(*(int*)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_EXEC:
            {
              if (!is_valid_arg(temp_esp, 1))
                {
                  syscall_exit(-1);
                  break;
                }
              // put command line in the system call
              f->eax = syscall_exec(*(char**)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_WAIT:
            {
              if (!is_valid_arg (temp_esp, 1))
                {
                  syscall_exit(-1);
                  break;
                }
              f->eax = syscall_wait(*(pid_t*)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_CREATE:
            {
              if (!is_valid_arg (temp_esp, 2))
                {
                  syscall_exit(-1);
                  break;
                }
              f->eax = syscall_create(*(char**)ESP_ARGV_PTR(temp_esp, 0),
                                      (unsigned)*(int *)ESP_ARGV_PTR(temp_esp,1)
                                      );
            }
          break;
        case SYS_REMOVE:
            {
              if (!is_valid_arg (temp_esp, 1))
                {
                  syscall_exit(-1);
                  break;
                }
              f->eax = syscall_remove(*(char**)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_OPEN:
            {
              if (!is_valid_arg (temp_esp, 1))
                {
                  syscall_exit(-1);
                  break;
                }
              f->eax = syscall_open(*(char**)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_FILESIZE:
            {
              if (!is_valid_arg (temp_esp, 1))
                {
                  syscall_exit(-1);
                  break;
                }
              f->eax = syscall_filesize(*(int*)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_READ:
            {
              if (!is_valid_arg3 (temp_esp, 3))
                {
                  syscall_exit(-1);
                  break;
                }
              f->eax = syscall_read(*(int*)ESP_ARGV3_PTR(temp_esp, 0),
                           (void*)*(int*)ESP_ARGV3_PTR(temp_esp, 1),
                           (size_t)*(int*)ESP_ARGV3_PTR(temp_esp, 2)
                           );
            }
          break;
        case SYS_WRITE:
            {
              if (!is_valid_arg3(temp_esp, 3))
                {
                  syscall_exit(-1);
                  break;
                }
              f->eax = syscall_write (*(int*)ESP_ARGV3_PTR(temp_esp, 0),
                                      (void*)*(int*)ESP_ARGV3_PTR(temp_esp, 1),
                                      (size_t)*(int*)ESP_ARGV3_PTR(temp_esp, 2)
                                      );
            }
          break;
        case SYS_SEEK:
            {
              if (!is_valid_arg (temp_esp, 2))
                {
                  syscall_exit(-1);
                  break;
                }
              syscall_seek(*(int*)ESP_ARGV_PTR(temp_esp, 0),
                           (unsigned)*(int*)ESP_ARGV_PTR(temp_esp, 1)
                           );
            }
          break;
        case SYS_TELL:
            {
              if (!is_valid_arg (temp_esp, 1))
                {
                  syscall_exit(-1);
                  break;
                }
              f->eax = syscall_tell(*(int*)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_CLOSE:
            {
              if (!is_valid_arg (temp_esp, 1))
                {
                  syscall_exit(-1);
                  break;
                }
              syscall_close(*(int*)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_FIBO: // Calculate fibonacci
            {
              if (!is_valid_arg(temp_esp, 1))
                {
                  syscall_exit(-1);
                  break;
                }
              f->eax = syscall_fibonacci (*(int*)ESP_ARGV_PTR(temp_esp, 0));
            }
          break;
        case SYS_SUM: // Sum Of four integers
            {
              if (!is_valid_arg3(temp_esp, 4))
                {
                  syscall_exit(-1);
                  break;
                }
              f->eax = syscall_sum_of_four_integers(*(int*)ESP_ARGV3_PTR(temp_esp, 0),
                                                   *(int*)ESP_ARGV3_PTR(temp_esp, 1),
                                                   *(int*)ESP_ARGV3_PTR(temp_esp, 2),
                                                   *(int*)ESP_ARGV3_PTR(temp_esp, 3)
                                                   );
            }
          break;
        }
    }


  //  printf ("system call!\n");
  //  thread_exit ();

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

  //printf("Terminating the current user program!\n");
  //cur->status = THREAD_DYING;
  cur->parent->exit_status = status;
  printf("%s: exit(%d)\n", cur->name, status);
  cur->normal_termin = true;
  
//  printf("I'm child!\n");
//  printf("Child tid : %d | Parent tid : %d\n", cur->tid, cur->parent->tid);
  sema_up (&cur->parent->wait_sema);
//  printf("Wake up my parent!!\n");
  thread_exit();
//  printf("after thread_exit\n");
  return;
}

pid_t
syscall_exec(const char *cmd_line)
{
  /*
  struct thread *parent = thread_current();
  struct list_elem *e = list_begin(&parent -> child_list);
  struct thread *child = list_entry(e, struct thread, child_elem);

  if(child->tid == parent->tid)
  {
    while(!child->normal_termin);
  }
  */
  if (!is_valid_ptr(cmd_line))
    syscall_exit(-1);

  return (pid_t)process_execute(cmd_line);
}

int
syscall_wait(pid_t pid)
{
  return process_wait((tid_t)pid);
}

int
syscall_read(int fd, void *buffer, unsigned size)
{
  if (!is_valid_ptr(buffer))
    syscall_exit(-1);

  unsigned i=0;
  if(fd == 0)
  {
      do{
        *((uint8_t *)buffer + i) = input_getc();
        i++;
      }while(i<size && (*((uint8_t *)buffer + i) != '\n'));
      // }while(i<size && (*((uint8_t *)buffer + i) != '\n' || *((uint8_t *)buffer + i) != '\0'));
      return i;
  }
  else
  {
    struct file* file = search_file(fd);
    if(file == NULL) return -1;
    
    int result;
    lock_acquire(&filesys_lock);
    result = (int)file_read(file,buffer,(off_t)size);
    lock_release(&filesys_lock);
    return result;
  }
  return -1;
}

int
syscall_write(int fd, const void *buffer, unsigned size)
{
  if (!is_valid_ptr(buffer))
    syscall_exit(-1);
  
  if(fd == 1)
    {
      //for (i = 0; i < size; i++)
      //  if(!*((char *)buffer + i)) break;
      putbuf((char *)buffer,(size_t)size);
      return size;
    }
  else
    {
      struct file* file = search_file(fd);
      if(file == NULL) return -1;
    
      int result;
      lock_acquire(&filesys_lock);
      result = (int)file_write(file,buffer,(off_t)size);
      lock_release(&filesys_lock);
      return result;
    }
  return -1;
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

bool
syscall_create(const char *file, unsigned initial_size)
{
  if (!is_valid_ptr(file) || !(*file))
    syscall_exit(-1);
  
  bool result;
  lock_acquire(&filesys_lock);
  result = filesys_create(file,(off_t)initial_size);
  lock_release(&filesys_lock);
  return result;
}
bool
syscall_remove(const char *file)
{
  if (!is_valid_ptr(file) || !(*file))
    syscall_exit(-1);
  
  bool result;
  lock_acquire(&filesys_lock);
  result = filesys_remove(file);
  lock_release(&filesys_lock);
  return result;
}
int
syscall_open(const char *file)
{
  if (!is_valid_ptr(file) || !(*file))
    syscall_exit(-1);
  
  struct file* op_file;
  lock_acquire(&filesys_lock);
  op_file = filesys_open(file);
  lock_release(&filesys_lock);

  // need initializing
}
int
syscall_filesize(int fd)
{
    struct file* file = search_file(fd);
    if(file == NULL) return -1;
    
    int result;
    lock_acquire(&filesys_lock);
    result = (int)file_length(file);
    lock_release(&filesys_lock);
    return result;
}
void
syscall_seek(int fd, unsigned position)
{
    struct file* file = search_file(fd);
    if(file == NULL) return -1;
    
    lock_acquire(&filesys_lock);
    file_seek(file,(off_t)position);
    lock_release(&filesys_lock);
    return ;
}
unsigned
syscall_tell(int fd)
{
    struct file* file = search_file(fd);
    if(file == NULL) return -1;
    
    unsigned result;
    lock_acquire(&filesys_lock);
    result = (unsigned)file_tell(file);
    lock_release(&filesys_lock);
    return result;
}
void
syscall_close(int fd)
{
    struct file* file = search_file(fd);
    if(file == NULL) return -1;
    
    lock_acquire(&filesys_lock);
    file_close(file);
    lock_release(&filesys_lock);
    return ;
    // need freeing
}

// ex) is_valid = is_valid_arg (f->esp);
static bool
is_valid_arg (const void* esp, int argc)
{
  int i;
  void* argv_ptr = NULL;

  for (i = 0; i < argc; i++)
    {
      argv_ptr = ESP_ARGV_PTR(esp, i); // ith argv pointer

      if (!is_valid_ptr(argv_ptr)) {//is_valid_ptr((void*)(*(char**)argv_ptr))) {
//        printf("false...\n");
        return false;
      }
    }

  return true;
}

static bool
is_valid_arg3 (const void* esp, int argc)
{
  int i;
  void* argv_ptr = NULL;

  for (i = 0; i < argc; i++)
    {
      argv_ptr = ESP_ARGV3_PTR(esp, i); // ith argv pointer

      if (!is_valid_ptr(argv_ptr)) {// || !is_valid_ptr((void*)(*(char**)argv_ptr)))
        return false;
      }
    }

  return true;
}
struct file* search_file(int fd)
{
  struct thread* current = thread_current();
  struct list_elem *e;
  struct list_elem *chosen = NULL;

  for(e = list_begin(&current->filelist); e != list_end(&current->filelist); e = list_next(e))
  {
    if(fd == list_entry(e,struct file_list,ptr)->fd)
    {
      chosen = e;
      return list_entry(chosen,struct file_list,ptr)->file;
    }
  }
  return NULL;
}
