#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
  char *buff = malloc(4096);
  strcpy(buff, "Cannot open file in 'wbx' mode\n");
  printf("buff:\nlen = %d\ntext: '%s'\n", strlen(buff), buff);

  const char *msg = "The file already exists or could not be opened in the write mode.\n";
  printf("msg:\nlen = %d\ntext: '%s'\n", strlen(msg), msg);

  memmove(buff + strlen(buff), msg, strlen(msg) + 1);
  printf("buff:\nlen = %d\ntext: '%s'\n", strlen(buff), buff);

  return 0;
}
