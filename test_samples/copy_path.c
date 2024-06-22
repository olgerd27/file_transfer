#include <stdio.h>
#include <string.h>

#define LEN_PATH_MAX 4096

int copy_path(const char *path_src, char *path_trg)
{
  return snprintf(path_trg, LEN_PATH_MAX, "%s", path_src);
}

int main()
{
  const char *dir_start = "/home/oleh/space";
  char path_curr_1[LEN_PATH_MAX];
  char path_curr_2[LEN_PATH_MAX];
  int offset_1, offset_2;

  // Variant 1 (initial)
  offset_1 = strlen(dir_start); // define offset as the start dir length that will be copied into path_curr
  strncpy(path_curr_1, dir_start, offset_1 + 1); // strncpy doesn't add/copy '\0', so add "+1" to copy it

  // Variant 2 (proposed)
  offset_2 = copy_path(dir_start, path_curr_2);

  printf("Copied strings are %s.\n", (strcmp(path_curr_1, path_curr_2) == 0 ? "equal" : "NOT equal"));

  printf("path_curr_1: '%s' -> ", path_curr_1);
  int i;
  for (i = 0; i < offset_1 + 1; ++i)
    printf("%i ", path_curr_1[i]);
  printf("\n");

  printf("path_curr_2: '%s' -> ", path_curr_2);
  for (i = 0; i < offset_2 + 1; ++i)
    printf("%i ", path_curr_2[i]);
  printf("\n");

  printf("offset 1 = %d,  offset_2 = %d\n", offset_1, offset_2);

  return 0;
}
