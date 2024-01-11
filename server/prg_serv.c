/*
 * prg_serv.c: implementation of the server functions for file transfers.
 */

#include <unistd.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* Created by rpcgen */

extern int errno; // is global, defined in the system's standard C library

void reset_errmsg(char **errmsg)
{
  if (*errmsg == NULL)
    *errmsg = (char*)malloc(SIZE_ERRMSG);
  else
    memset(*errmsg, 0, SIZE_ERRMSG);
}

void reset_upld(errinf *err)
{
  if (errno) errno = 0;
  if (err->num) err->num = 0;
}

errinf * upload_file_1_svc(file *file_upld, struct svc_req *)
{
  static errinf res_err; /* must be static */
  printf("[main] 0\n");

  // Reset the results of the previous call
  reset_upld(&res_err);
  printf("[main] 1\n");

  // Check the file existence
  if (access(file_upld->name, F_OK) == 0) {
    res_err.num = 1;
    reset_errmsg(&res_err.errinf_u.msg);
    sprintf(res_err.errinf_u.msg, 
            "Error #%i: The specified file '%s' name already exists.\n"
            "Please select a different name for your file.\n", res_err.num, file_upld->name); 
    return &res_err;
  }
  printf("[main] 2\n");

  // Open the file
  FILE *hfile = fopen(file_upld->name, "wb");
  if (hfile == NULL) {
    res_err.num = 2;
    reset_errmsg(&res_err.errinf_u.msg);
    sprintf(res_err.errinf_u.msg, 
            "Error #%i: Cannot open the file: '%s'.\nSystem error message:\n"
            "%s (errno=%i).\n", res_err.num, file_upld->name, strerror(errno), errno); 
    return &res_err;
  }
  printf("3\n");

  // Write to the file
  fwrite(file_upld->cont.t_flcont_val, 1, file_upld->cont.t_flcont_len, hfile);
  if (ferror(hfile)) {
    res_err.num = 3;
    reset_errmsg(&res_err.errinf_u.msg);
    sprintf(res_err.errinf_u.msg, 
            "Error #%i: Cannot write to the file: '%s'.\nSystem error message:\n"
            "%s (errno=%i).\n", res_err.num, file_upld->name, strerror(errno), errno);
    return &res_err;
  }

  fclose(hfile);
  return &res_err;
}

t_flcont * download_file_1_svc(t_flname *, struct svc_req *)
{
  static t_flcont ret_flcont;
  return &ret_flcont;
}

