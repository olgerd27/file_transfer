/*
 * A program to read the directory content, save the directory's entris to a string, 
 * print and delete this list.
 * This program imitates the behaviour that may be used in the RPC client-server system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#define DIR_SIZE 8192

void print_dir_cont(char *dc)
{
  if (!dc) return;
  printf("%s", dc);
}

/*
 * RC: 0 - OK, !0 - ERROR
 */
char * get_dir_cont(const char *dir_name, char *dc)
{
  DIR *hdir;
  struct dirent *dent;

  if ( (hdir = opendir(dir_name)) == NULL ) {
    fprintf(stderr, "Cannot open directory %s\n", dir_name); 
    return NULL;
  }

  errno = 0;
  dc[0] = '\0';
  while ( (dent = readdir(hdir)) != NULL )
    sprintf(dc, "%s%s\n", dc, dent->d_name);
  closedir(hdir);
  if (errno)
    perror("Error reading directory");
  return dc;
}

int main(int argc, char *argv[])
{
  char dir_cont[DIR_SIZE];
  if (argc != 2) {
    fprintf(stderr, "Usage: %s DIR\n", argv[0]);
    exit(1);
  }
  if (!get_dir_cont(argv[1], dir_cont))
    return 1;
  print_dir_cont(dir_cont);
  return 0;
}
