/*
 * prg_serv.c: implementation of the server functions for file transfers.
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* Created by rpcgen */

// Global system variable that store the error number
extern int errno;

// Reset/init the error state 
void reset_err(int *rc)
{
  // reset the return code: 
  // 1 - success, 0 - failure
  *rc = 1;

  // reset the system error number
  if (errno) errno = 0;
}

/* 
 * The Upload file main function
 */
//errinf * upload_file_1_svc(file *file_upld, struct svc_req *)
int * upload_file_1_svc(file *file_upld, struct svc_req *)
{
  static int rc; /* must be static */
  printf("[upload_file] 0\n");

  // Reset an error state remained from the previous call
  reset_err(&rc);
  printf("[upload_file] 1\n");

  // Open the file
  FILE *hfile = fopen(file_upld->name, "wbx");
  if (hfile == NULL) {
    printf("[upload_file] 2.1\n");
    fprintf(stderr, 
            "!--Error 50: The file '%s' already exists or could not be opened in the write mode.\n"
            "System error #%i message:\n%s\n",
            file_upld->name, errno, strerror(errno)); 
    rc = 0;
    return &rc;
  }
  printf("[upload_file] 3\n");

  // Write the client file data to a new file
  fwrite(file_upld->cont.t_flcont_val, 1, file_upld->cont.t_flcont_len, hfile);
  // TODO: add check for a number of characters written to the file
  if (ferror(hfile)) {
    printf("[upload_file] 3.1\n");
    fprintf(stderr, "!--Error 51: Cannot write to the file: '%s'.\n"
                    "System error #%i message:\n%s", 
                    file_upld->name, errno, strerror(errno));
    fclose(hfile);
    rc = 0;
    return &rc;
  }
  printf("[upload_file] 4\n");

  fclose(hfile);
  printf("[upload_file] 5\n");

  return &rc;
}

/*
 * The Download file main function
 */
t_flcont * download_file_1_svc(t_flname *flname, struct svc_req *)
{
  static t_flcont ret_flcont;
  printf("[download_file] 0\n");

  // Open the file
  FILE *hfile = fopen(*flname, "rb");
  if (hfile == NULL) {
    printf("[download_file] 0.1\n");
    fprintf(stderr, "!--Error 60: Cannot open the file '%s' in the read mode.\n"
                    "System error #%i message:\n%s",
                    *flname, errno, strerror(errno)); 
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
    printf("[download_file] 2.1\n");
    fprintf(stderr, "!--Error 61: Memory allocation error (size=%ld) to store the file content:\n'%s'\n", 
                    ret_flcont.t_flcont_len, *flname);
    fclose(hfile);
    return &ret_flcont;
  }
  printf("[download_file] 3\n");

  // Read the file content into the buffer
  fread(ret_flcont.t_flcont_val, 1, ret_flcont.t_flcont_len, hfile);
  // TODO: add check for a number of characters that are read from the file
  if (ferror(hfile)) {
    printf("[download_file] 3.1\n");
    fprintf(stderr, "!--Error 62: Cannot read the file: '%s'.\n"
                    "System error #%i message:\n%s", 
                    *flname, errno, strerror(errno));
    fclose(hfile);
    return &ret_flcont;
  }
  printf("[download_file] 4\n");

  fclose(hfile);
  printf("[download_file] 5\n");

  return &ret_flcont;
}

