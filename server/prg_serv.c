/*
 * prg_serv.c: implementation of the server functions for file transfers.
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* Created by rpcgen */

extern int errno; // is global, defined in the system's standard C library

/*
 * Reset/init the error information
 */
void reset_err(errinf *err)
{
  // allocate the memory for error messages
  if (!err->errinf_u.msg)
    err->errinf_u.msg = (char*)malloc(SIZE_ERRMSG);

  // reset the error information that is remained from the previous case of an error
  if (err->num) {
    err->num = 0;
    if (err->errinf_u.msg)
      memset(err->errinf_u.msg, 0, SIZE_ERRMSG);
  }

  // reset the system error number
  if (errno) errno = 0;
}

errinf * upload_file_1_svc(file *file_upld, struct svc_req *)
{
  static errinf res_err; /* must be static */
  printf("[upload_file] 0\n");

  // Reset the error information remained from the previous call
  reset_err(&res_err);
  printf("[upload_file] 1\n");

  // Check the file existence
  // TODO: if file exists provide a choice to the client what to do next: 
  // Specify different name, Overwrite, Cancel
  if (access(file_upld->name, F_OK) == 0) {
    res_err.num = 50;
    sprintf(res_err.errinf_u.msg, 
            "The specified file '%s' already exists.\n"
            "Please choose a different name to save a file.", file_upld->name); 
    printf("[upload_file_1_svc 1.1] ERROR #%i\n", res_err.num);
    return &res_err;
  }
  printf("[upload_file] 2\n");

  // Open the file
  FILE *hfile = fopen(file_upld->name, "wb");
  if (hfile == NULL) {
    res_err.num = 51;
    sprintf(res_err.errinf_u.msg, 
            "Cannot open the file '%s' in the write mode.\n"
            "System error #%i message:\n%s",
            file_upld->name, errno, strerror(errno)); 
    printf("[upload_file] 2.1 ERROR #%i\n", res_err.num);
    return &res_err;
  }
  printf("[upload_file] 3\n");

  // Write to the file
  fwrite(file_upld->cont.t_flcont_val, 1, file_upld->cont.t_flcont_len, hfile);
  if (ferror(hfile)) {
    res_err.num = 52;
    sprintf(res_err.errinf_u.msg, 
            "Cannot write to the file: '%s'.\n"
            "System error #%i message:\n%s", 
            file_upld->name, errno, strerror(errno));
    printf("[upload_file] 3.1 ERROR #%i\n", res_err.num);
    fclose(hfile);
    return &res_err;
  }
  printf("[upload_file] 4\n");

  fclose(hfile);
  printf("[upload_file] 5\n");

  return &res_err;
}

t_flcont * download_file_1_svc(t_flname *flname, struct svc_req *)
{
  static t_flcont ret_flcont;
  printf("[download_file] 0\n");

  // Open the file
  FILE *hfile = fopen(*flname, "rb");
  if (hfile == NULL) {
    fprintf(stderr, 
            "Cannot open the file '%s' in the read mode.\n"
            "System error #%i message:\n%s",
            *flname, errno, strerror(errno)); 
    printf("[download_file] 0.1 File openning error\n");
    return &ret_flcont;
  }
  printf("[download_file] 1\n");

  // Get the file size
  fseek(hfile, 0, SEEK_END);
  ret_flcont.t_flcont_len = ftell(hfile);
  rewind(hfile);
  printf("[download_file] 2\n");

  // Allocate the memory to store the file content
  ret_flcont.t_flcont_val = (char*)malloc(ret_flcont.t_flcont_len);
  if (ret_flcont.t_flcont_val == NULL) {
    fprintf(stderr, "Memory allocation error (size=%ld) to store the file content:\n'%s'\n", 
            ret_flcont.t_flcont_len, *flname);
    printf("[download_file] 2.1 Memory allocation error\n");
    fclose(hfile);
    return &ret_flcont;
  }
  printf("[download_file] 3\n");

  // Read the file
  fread(ret_flcont.t_flcont_val, 1, ret_flcont.t_flcont_len, hfile);
  if (ferror(hfile)) {
    fprintf(stderr, 
            "Cannot read the file: '%s'.\n"
            "System error #%i message:\n%s", 
            *flname, errno, strerror(errno));
    printf("[download_file] 3.1 File reading error\n");
    fclose(hfile);
    return &ret_flcont;
  }
  printf("[download_file] 4\n");

  fclose(hfile);
  printf("[download_file] 5\n");

  return &ret_flcont;
}

