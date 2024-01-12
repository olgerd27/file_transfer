/*
 * prg_serv.c: implementation of the server functions for file transfers.
 */

#include <unistd.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* Created by rpcgen */

extern int errno; // is global, defined in the system's standard C library

/*
 * Memory allocation to store the error message
 */
void malloc_errmsg(char **errmsg)
{
  if (*errmsg == NULL)
    *errmsg = (char*)malloc(SIZE_ERRMSG);
}

/*
 * Reset the error information that remained from the previous call
 * when error has appeared
 */
void reset_err(errinf *err)
{
  if (errno) errno = 0;
  if (err->num) {
    err->num = 0;
    if (err->errinf_u.msg)
      memset(err->errinf_u.msg, 0, SIZE_ERRMSG);
  }
}

errinf * upload_file_1_svc(file *file_upld, struct svc_req *)
{
  static errinf res_err; /* must be static */
  printf("[main] 0\n");
  printf("errno=%d  res_err: num=%d  msg(%p):\n%s\n", 
         errno, res_err.num, (void*)res_err.errinf_u.msg, res_err.errinf_u.msg);

  // Reset the results of the previous call
  reset_err(&res_err);
  printf("[main] 1\n");
  printf("errno=%d  res_err: num=%d  msg(%p):\n%s\n", 
         errno, res_err.num, (void*)res_err.errinf_u.msg, res_err.errinf_u.msg);

  // Check the file existence
  // TODO: if file exists provide a choice to the client what to do next: 
  // Specify different name, Overwrite, Cancel
  if (access(file_upld->name, F_OK) == 0) {
    res_err.num = 1;
    malloc_errmsg(&res_err.errinf_u.msg);
    sprintf(res_err.errinf_u.msg, 
            "The specified file '%s' already exists.\n"
            "Please choose a different name to save a file.", file_upld->name); 
    printf("[main 1.1]\n");
    printf("errno=%d  res_err: num=%d  msg(%p):\n%s\n", 
           errno, res_err.num, (void*)res_err.errinf_u.msg, res_err.errinf_u.msg);
    return &res_err;
  }
  printf("[main] 2\n");

  // Open the file
  FILE *hfile = fopen(file_upld->name, "wb");
  if (hfile == NULL) {
    res_err.num = 2;
    malloc_errmsg(&res_err.errinf_u.msg);
    sprintf(res_err.errinf_u.msg, 
            "Cannot open the file: '%s' in the write mode.\n"
            "System error #%i message:\n%s.",
            file_upld->name, errno, strerror(errno)); 
    printf("[main 2.1]\n");
    printf("errno=%d  res_err: num=%d  msg(%p):\n%s\n", 
           errno, res_err.num, (void*)res_err.errinf_u.msg, res_err.errinf_u.msg);
    return &res_err;
  }
  printf("[main] 3\n");

  // Write to the file
  fwrite(file_upld->cont.t_flcont_val, 1, file_upld->cont.t_flcont_len, hfile);
  if (ferror(hfile)) {
    res_err.num = 3;
    malloc_errmsg(&res_err.errinf_u.msg);
    sprintf(res_err.errinf_u.msg, 
            "Cannot write to the file: '%s'.\n"
            "System error #%i message:\n%s.", 
            file_upld->name, errno, strerror(errno));
    printf("[main 3.1]\n");
    printf("errno=%d  res_err: num=%d  msg(%p):\n%s\n", 
           errno, res_err.num, (void*)res_err.errinf_u.msg, res_err.errinf_u.msg);
    fclose(hfile);
    return &res_err;
  }
  printf("[main] 4\n");

  fclose(hfile);
  printf("[main] 5\n");

  return &res_err;
}

t_flcont * download_file_1_svc(t_flname *, struct svc_req *)
{
  static t_flcont ret_flcont;
  return &ret_flcont;
}

