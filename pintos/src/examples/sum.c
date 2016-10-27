#include <stdio.h>
#include <syscall.h>
#include "../userprog/syscall.h"

int main(int argc, char **argv)
{

  printf("Result of Fibonacci : %d\n",syscall_fibonacci(argv[0]));
  printf("Result of Sum : %d\n",syscall_sum_of_four_integers(argv[0],argv[1],argv[2],argv[3]));

  return EXIT_SUCCESS;
}
