#include <stdio.h>

#ifndef SOME_VALUE
#define SOME_VALUE 0
#endif

int main()
{
  printf("some value = %i\n", SOME_VALUE);
  return 0;
}
