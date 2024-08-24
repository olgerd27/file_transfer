/*
 * A program to read the directory content using the scandir system function.
 */
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
 
#define FALSE 0
#define TRUE !FALSE
 
extern  int alphasort();
char pathname[MAXPATHLEN];
 
int main()
{
  int count, i;
  struct direct **files;
  int file_select();

  if (getcwd(pathname, MAXPATHLEN) == NULL ) {
    printf("Error getting path\n");
    exit(0);
  }
  printf("Current Working Directory = %s\n",pathname);
  count = scandir(pathname, &files, file_select, alphasort);

  /* If no files found, make a non-selectable menu item */
  if (count <= 0) {
    printf("No files in this directory\n");
    exit(0);
  }
  printf("Number of files = %d\n",count);
  for (i = 1; i < count + 1; ++i)
    printf("%s  ",files[i-1]->d_name);
  printf("\n"); /* flush buffer */
}

int file_select(struct direct *entry)
{
  if ((strcmp(entry->d_name, ".") == 0) || 
      (strcmp(entry->d_name, "..") == 0))
    return (FALSE);
  else
    return (TRUE);
}
