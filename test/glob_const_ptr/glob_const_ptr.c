#include <stdio.h>
#include <stdlib.h>

const int *p_argint;
const char *p_argstr;

int main(int argc, char *argv[])
{
  if (argc < 3) {
    printf("Usage: %s [arg_int] [arg_string]\n", argv[0]);
    return 1;
  }

  /* 
   * Working with int-type argument 
   */
  int argint = atoi(argv[1]);
  p_argint = &argint;
  printf("argv[1]='%s'  argint=%i  *p_argint=%i\n", argv[1], argint, *p_argint);

  // Trying to change the value the const int pointer points to - getting a compilation error
  // *p_argint = 6;
  // printf("argv[1]='%s'  argint=%i  *p_argint=%i\n", argv[1], argint, *p_argint);

  /*
   * Working with string-type argument
   */
  p_argstr = argv[2];
  printf("argv[2]: val='%s' ptr=%p,  p_argint: val='%s' ptr=%p\n", 
         argv[2], (void*)argv[2], p_argstr, (void*)p_argstr);
  
  // Trying to change the value the const string pointer points to - getting a compilation error
  // If remove 'const' for p_argstr -> compilation and execution will be successful
  // p_argstr[1] = '?';
  // printf("argv[2]: val='%s' ptr=%p,  p_argint: val='%s' ptr=%p\n", 
  //        argv[2], (void*)argv[2], p_argstr, (void*)p_argstr);

  return 0;
}
