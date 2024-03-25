#include <stdio.h>
#include <unistd.h>

int main()
{
  int i;
  for (i = 0; i < 10; i++){
    printf("\rValue of i is: %d", i*10);
    fflush(stdout);
    sleep(1);
  }
  printf("\n");
  return 0;
}
