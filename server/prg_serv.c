/*
 * prg_serv.c: implementation of the server functions for file transfers.
 */

#include <unistd.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* Created by rpcgen */

extern int errno; // is global, defined in the system's standard C library
static t_errmsg errmsg;

// TODO: check if these resets are actually required
void reset_upld(int *rc)
{
  *rc = 0;
  errno = 0;
  memset(errmsg, 0, sizeof(errmsg));
}

int * upload_file_1_svc(file *file_upld, struct svc_req *)
{
  static int rc; /* must be static */

  // Reset the results of the previous call
  reset_upld(&rc);

  // Check the file existence
  if (access(file_upld->name, F_OK) == 0) {
    rc = 1;
    sprintf(errmsg, "Error #%i: The specified file '%s' name already exists.\n"
                    "Please select a different name for your file.\n", rc, file_upld->name); 
    return &rc;
  }

  // Open the file
  FILE *hfile = fopen(file_upld->name, "wb");
  if (hfile == NULL) {
    rc = 2;
    sprintf(errmsg, "Error #%i: Cannot open the file: '%s'.\nSystem error message:\n"
                    "%s (errno=%i).\n", rc, file_upld->name, strerror(errno), errno); 
    return &rc;
  }

  // Write to the file
  fwrite(file_upld->content.t_flcont_val, 1, file_upld->content.t_flcont_len, hfile);
  if (ferror(hfile)) {
    rc = 3;
    sprintf(errmsg, "Error #%i: Cannot write to the file: '%s'.\nSystem error message:\n"
                    "%s (errno=%i).\n", rc, file_upld->name, strerror(errno), errno);
    return &rc;
  }

  fclose(hfile);
  return &rc;
}

t_flcont * download_file_1_svc(t_flname *, struct svc_req *)
{
  static t_flcont ret_flcont;
  return &ret_flcont;
}

t_errmsg * get_error_msg_1_svc(void *, struct svc_req *)
{
  static t_errmsg ret_errmsg;
  return &ret_errmsg;
}

