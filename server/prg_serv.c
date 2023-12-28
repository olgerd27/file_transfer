/*
 * prg_serv.c: implementation of the server functions for file transfers.
 */

#include <unistd.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* Created by rpcgen */

extern int errno; // is global, defined in the system's standard C library

void reset_upld(errinf *err)
{
  errno = 0;
  err->num = 0;
  memset(err->errinf_u.msg, 0, sizeof(err->errinf_u.msg));
}

errinf * upload_file_1_svc(file *file_upld, struct svc_req *)
{
  static errinf res_err; /* must be static */

  // Reset the results of the previous call
  reset_upld(&res_err);

  // Check the file existence
  if (access(file_upld->name, F_OK) == 0) {
    res_err.num = 1;
    sprintf(res_err.errinf_u.msg, 
            "Error #%i: The specified file '%s' name already exists.\n"
            "Please select a different name for your file.\n", res_err.num, file_upld->name); 
    return &res_err;
  }

  // Open the file
  FILE *hfile = fopen(file_upld->name, "wb");
  if (hfile == NULL) {
    res_err.num = 2;
    sprintf(res_err.errinf_u.msg, 
            "Error #%i: Cannot open the file: '%s'.\nSystem error message:\n"
            "%s (errno=%i).\n", res_err.num, file_upld->name, strerror(errno), errno); 
    return &res_err;
  }

  // Write to the file
  fwrite(file_upld->cont.t_flcont_val, 1, file_upld->cont.t_flcont_len, hfile);
  if (ferror(hfile)) {
    res_err.num = 3;
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

