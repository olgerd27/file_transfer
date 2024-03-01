/*
 * prg_serv.c: implementation of the server functions for file transfers.
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* Created by rpcgen */

// Global system variable that store the error number
extern int errno;

/* 
 * A Section to Upload file
 */
// Reset the variables (state) for the 'upload' function
void reset_upld(int *rc)
{
  // reset the return code: 
  // 1 - success, 0 - failure
  *rc = 1;

  // reset the system error number
  if (errno) errno = 0;
}

// The main function to Upload a file
int * upload_file_1_svc(file *file_upld, struct svc_req *)
{
  static int rc; /* must be static */
  printf("[upload_file] 0\n");

  // Reset a state remained after a previous call of this function
  reset_upld(&rc);
  printf("[upload_file] 1\n");

  // Open the file
  FILE *hfile = fopen(file_upld->name, "wbx");
  printf("[upload_file] 2\n");

  if (hfile == NULL) {
    printf("[upload_file] 2.1\n");
    fprintf(stderr, 
            "!--Error 50: The file '%s' already exists or could not be opened in the write mode.\n"
            "System error #%i: %s\n",
            file_upld->name, errno, strerror(errno)); 
    rc = 0;
    return &rc;
  }

  // Write the client file data to a new file
  size_t nwrt = fwrite(file_upld->cont.t_flcont_val, 1, file_upld->cont.t_flcont_len, hfile);
  printf("[upload_file] 3\n");

  // Check a number of written items 
  if (nwrt < file_upld->cont.t_flcont_len) {
    printf("[upload_file] 3.1\n");
    fprintf(stderr, "!--Error 51: partial writing to the file: '%s'.\n"
                    "System error #%i: %s", 
                    file_upld->name, errno, strerror(errno));
    fclose(hfile);
    rc = 0;
    return &rc;
  }

  // Check if an error has been occurred during the writing operation
  if (ferror(hfile)) {
    printf("[upload_file] 3.2\n");
    fprintf(stderr, "!--Error 52: error occurred while writing to the file: '%s'.\n"
                    "System error #%i: %s", 
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
 * A Section to Download file
 */
// Reset the variables (state) for the 'download' function
void reset_dwld(t_flcont *file_cont)
{
  // freeing memory for the contents of the downloaded file that was allocated during 
  // the previous call of the server 'download' function
  if (file_cont != NULL && file_cont->t_flcont_val != NULL) {
    free(file_cont->t_flcont_val);
    file_cont->t_flcont_val = NULL;
    file_cont->t_flcont_len = 0;
  }

  // reset the system error number
  if (errno) errno = 0;
}

// The main function to Download a file
t_flcont * download_file_1_svc(t_flname *flname, struct svc_req *)
{
  static t_flcont ret_flcont;
  printf("[download_file] 1\n");

  // Reset a state remained after a previous call of this function
  reset_dwld(&ret_flcont);
  printf("[download_file] 2\n");

  // Open the file
  FILE *hfile = fopen(*flname, "rb");
  printf("[download_file] 3\n");

  if (hfile == NULL) {
    printf("[download_file] 3.1\n");
    fprintf(stderr, "!--Error 60: Cannot open the file '%s' in the read mode.\n"
                    "System error #%i: %s",
                    *flname, errno, strerror(errno)); 
    return (t_flcont *)NULL;
  }

  // Get the file size
  fseek(hfile, 0, SEEK_END);
  ret_flcont.t_flcont_len = ftell(hfile);
  rewind(hfile);
  printf("[download_file] 4\n");

  // Allocate the memory to store the file content
  ret_flcont.t_flcont_val = (char*)malloc(ret_flcont.t_flcont_len);
  printf("[download_file] 5\n");

  if (ret_flcont.t_flcont_val == NULL) {
    printf("[download_file] 5.1\n");
    fprintf(stderr, "!--Error 61: Memory allocation error (size=%ld) to store the file content:\n'%s'\n", 
                    ret_flcont.t_flcont_len, *flname);
    fclose(hfile);
    return (t_flcont *)NULL;
  }

  // Read the file content into the buffer
  size_t nrd = fread(ret_flcont.t_flcont_val, 1, ret_flcont.t_flcont_len, hfile);
  printf("[download_file] 6\n");

  // Check a number of items read
  if (nrd < ret_flcont.t_flcont_len) {
    printf("[download_file] 6.1\n");
    fprintf(stderr, "!--Error 62: partial reading of the file: '%s'.\n"
                    "System error #%i: %s", 
                    *flname, errno, strerror(errno));
    fclose(hfile);
    reset_dwld(&ret_flcont); // free the file content memory in case of error
    return (t_flcont *)NULL;
  }

  // Check if an error has been occurred during the reading operation
  if (ferror(hfile)) {
    printf("[download_file] 6.2\n");
    fprintf(stderr, "!--Error 63: error occurred while reading of the file: '%s'.\n"
                    "System error #%i: %s", 
                    *flname, errno, strerror(errno));
    fclose(hfile);
    reset_dwld(&ret_flcont); // free the file content memory in case of error
    return (t_flcont *)NULL;
  }
  printf("[download_file] 7\n");

  fclose(hfile);
  printf("[download_file] 8\n");

  return &ret_flcont;
}

