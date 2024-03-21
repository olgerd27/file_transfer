/*
 * A program to read the directory content, save the directory's entris to a list, 
 * print and delete this list.
 * This program imitates the behaviour that may be used in the RPC client-server system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

struct DirList
{
  char *ent_name;
  struct DirList *next;
};

typedef struct DirList DirList;

DirList * create_item(const char *name)
{
  DirList *dl = malloc(sizeof(DirList));
  dl->ent_name = malloc(strlen(name) + 1);
  strcpy(dl->ent_name, name);
  dl->next = NULL;
  return dl;
}

void print_dir(DirList *dl)
{
  while (dl) {
    printf("%s\n", dl->ent_name);
    dl = dl->next;
  }
}

void free_list(DirList *dl)
{
  if (!dl) return;
  DirList *next = dl;
  do {
    next = dl->next;
    free(dl->ent_name);
    free(dl);
    dl = next;
  } while (next);
}

DirList * get_dir_cont(const char *basedir)
{
  DIR *hdir;
  struct dirent *dent;
  DirList *dl = NULL;
  DirList *dl_prev = NULL;
  DirList *dl_first = NULL;

  if ( (hdir = opendir(basedir)) == NULL ) {
    fprintf(stderr, "Cannot open directory %s\n", basedir); 
    return (DirList *)NULL;
  }

  errno = 0;
  while ( (dent = readdir(hdir)) != NULL ) {
    dl = create_item(dent->d_name);
    if (dl_prev) dl_prev->next = dl;
    else         dl_first = dl;
    dl_prev = dl;
  }
  if (errno)
    perror("Error reading directory");
  closedir(hdir);
  return dl_first;
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s DIR\n", argv[0]);
    exit(1);
  }
  DirList *lst_dir = get_dir_cont(argv[1]);
  if (lst_dir == NULL) return 2;
  print_dir(lst_dir);
  free_list(lst_dir);
  return 0;
}
