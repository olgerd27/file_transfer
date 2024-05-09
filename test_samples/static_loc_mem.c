#include <stdio.h>
#include <string.h>

#define SIZE 10

struct file_inf
{
  char *name;
};
typedef struct file_inf file_inf;

void alloc_fname(file_inf *file)
{
  static char buf[SIZE];
  file->name = buf;
}

int main()
{
  file_inf flinf;
  alloc_fname(&flinf);
  strcpy(flinf.name, "backup.txt");
  printf("filename: '%s'\n", flinf.name);
  return 0;
}
