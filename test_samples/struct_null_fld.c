// Check if a struct field has a NULL value after creation the instance of this struct.
#include <stdio.h>
#include <string.h>

typedef char *t_flname;

typedef struct {
  unsigned t_flcont_len;
  char *t_flcont_val;
} t_flcont;

struct file_inf {
  t_flname name;
  t_flcont cont;
};
typedef struct file_inf file_inf;

struct file_err {
  file_inf file;
};

void check_fields(file_inf *p_file)
{
  if (!p_file) {
    fprintf(stderr, "Invalid pointer p_file=%p\n", p_file);
    return;
  }

  // check p_file->name
  if (p_file->name) {
    printf("p_file->name is NOT NULL: %p\n", p_file->name);
    printf("sizeof p_file->name: %i\n", sizeof(p_file->name));
    printf("strlen p_file->name: %i\n", strlen(p_file->name));
    printf("*p_file->name: %i->'%c'\n", *p_file->name, *p_file->name);
  }
  else
    printf("p_file->name is NULL: %p\n", p_file->name);

  // check p_file->cont.t_flcont_val
  if (p_file->cont.t_flcont_val) {
    printf("p_flcont_val is NOT NULL: %p\n", p_file->cont.t_flcont_val);
    printf("sizeof t_flcont_val: %i\n", sizeof(p_file->cont.t_flcont_val));
    printf("strlen t_flcont_val: %i\n", strlen(p_file->cont.t_flcont_val));
    printf("t_flcont_len: %i\n", p_file->cont.t_flcont_len);
    printf("*t_flcont_val: %i->'%c'\n", *p_file->cont.t_flcont_val, *p_file->cont.t_flcont_val);
  }
  else
    printf("p_file->cont.t_flcont_val is NULL: %p\n", p_file->cont.t_flcont_val);
}

int main()
{
  struct file_err flerr;
  check_fields(&flerr.file);
  return 0;
}

