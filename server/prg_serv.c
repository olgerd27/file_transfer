/*
 * prg_serv.c: implementation of the server functions for file transfers.
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* Created by rpcgen */

// Global system variable that store the error number
extern int errno;

// Reset the error info (an error state)
void reset_err(errinf *p_err)
{
  if (!p_err) {
    fprintf(stderr, "Cannot reset error information, p_err=%p\n", (void*)p_err);
    exit(1);
  }

  // initial allocation of the memory for error messages
  if (!p_err->errinf_u.msg)
    p_err->errinf_u.msg = (char*)malloc(SIZE_ERRMSG);

  // reset the error information that is remained from the previous case when error has occurred
  if (p_err->num) {
    p_err->num = 0;
    if (p_err->errinf_u.msg)
      memset(p_err->errinf_u.msg, 0, SIZE_ERRMSG);
  }

  // reset the system error number
  if (errno) errno = 0;
}

/* 
 * The Upload file Section
 */
// The main function to Upload a file
errinf * upload_file_1_svc(file *file_upld, struct svc_req *)
{
  static errinf res_err; /* must be static */
  printf("[upload_file] 0\n");

  // Reset an error state remained after a previous call of this function
  reset_err(&res_err);
  printf("[upload_file] 1\n");

  // Open the file
  FILE *hfile = fopen(file_upld->name, "wbx");
  printf("[upload_file] 2\n");

  if (hfile == NULL) {
    printf("[upload_file] 2.1\n");
    res_err.num = 50;
    sprintf(res_err.errinf_u.msg, 
            "The file '%s' already exists or could not be opened in the write mode.\n"
            "System server error %i: %s\n", file_upld->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from res_err.errinf_u.msg
    fprintf(stderr, "Error %i: File Upload Failed\n", res_err.num);
    return &res_err;
  }

  // Write the client file data to a new file
  size_t nch = fwrite(file_upld->cont.t_flcont_val, 1, file_upld->cont.t_flcont_len, hfile);
  printf("[upload_file] 3\n");

  // Check a number of written items 
  if (nch < file_upld->cont.t_flcont_len) {
    printf("[upload_file] 3.1\n");
    res_err.num = 51;
    sprintf(res_err.errinf_u.msg, 
            "Partial writing to the file: '%s'.\n"
            "System server error %i: %s", file_upld->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from res_err.errinf_u.msg
    fprintf(stderr, "Error %i: File Upload Failed\n", res_err.num);
    fclose(hfile);
    return &res_err;
  }
  printf("[upload_file] 4\n");

  // Check if an error has been occurred during the writing operation
  if (ferror(hfile)) {
    printf("[upload_file] 4.1\n");
    res_err.num = 52;
    sprintf(res_err.errinf_u.msg, 
            "Error occurred while writing to the file: '%s'.\n"
            "System server error %i: %s", file_upld->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from res_err.errinf_u.msg
    fprintf(stderr, "Error %i: File Upload Failed\n", res_err.num);
    fclose(hfile);
    return &res_err;
  }
  printf("[upload_file] 5\n");

  fclose(hfile);
  printf("[upload_file] 6\n\n");

  return &res_err;
}

/*
 * The Download file Section
 */
// Reset (deallocate) the file content
void reset_file_cont(t_flcont *file_cont)
{
  // freeing memory for the downloaded file content that was allocated during 
  // the previous call of the 'download' function
  if (file_cont && file_cont->t_flcont_val) {
    free(file_cont->t_flcont_val);
    file_cont->t_flcont_val = NULL;
    file_cont->t_flcont_len = 0;
  }
}

// The main function to Download a file
flcont_errinf * download_file_1_svc(t_flname *flname, struct svc_req *)
{
  static flcont_errinf ret_flerr; /* must be static */
  static t_flcont *p_flcnt = &ret_flerr.file_cont;
  static errinf *p_errinf = &ret_flerr.err_inf;
  printf("[download_file] 0\n");

  // Reset (deallocate) the file content
  reset_file_cont(p_flcnt);
  printf("[download_file] 1\n");
  // Reset an error state remained after a previous call of this function
  reset_err(p_errinf);
  printf("[download_file] 2\n");

  // Open the file
  FILE *hfile = fopen(*flname, "rb");
  printf("[download_file] 3\n");

  if (hfile == NULL) {
    printf("[download_file] 3.1\n");
    p_errinf->num = 60;
    sprintf(p_errinf->errinf_u.msg,
            "Cannot open the file '%s' in the read mode.\n"
            "System server error %i: %s", *flname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_errinf.errinf_u.msg
    fprintf(stderr, "Error %i: File Download Failed\n", p_errinf->num);
    return &ret_flerr;
  }

  // Get the file size
  fseek(hfile, 0, SEEK_END);
  p_flcnt->t_flcont_len = ftell(hfile);
  rewind(hfile);
  printf("[download_file] 4\n");

  // Allocate the memory to store the file content
  p_flcnt->t_flcont_val = (char*)malloc(p_flcnt->t_flcont_len);
  printf("[download_file] 5\n");

  if (p_flcnt->t_flcont_val == NULL) {
    printf("[download_file] 5.1\n");
    p_errinf->num = 61;
    sprintf(p_errinf->errinf_u.msg,
            "Memory allocation error (size=%ld) to store the file content:\n'%s'\n", 
            p_flcnt->t_flcont_len, *flname);
    // TODO: implement logging and put there the full error info taken from p_errinf.errinf_u.msg
    fprintf(stderr, "Error %i: File Download Failed\n", p_errinf->num);
    fclose(hfile);
    printf("[download_file] 5.2\n");
    return &ret_flerr;
  }

  // Read the file content into the buffer
  size_t nch = fread(p_flcnt->t_flcont_val, 1, p_flcnt->t_flcont_len, hfile);
  printf("[download_file] 6\n");

  // Check a number of items read
  if (nch < p_flcnt->t_flcont_len) {
    printf("[download_file] 6.1\n");
    p_errinf->num = 62;
    sprintf(p_errinf->errinf_u.msg,
            "Partial reading of the file: '%s'.\n"
            "System server error %i: %s", *flname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_errinf.errinf_u.msg
    fprintf(stderr, "Error %i: File Download Failed\n", p_errinf->num);
    fclose(hfile);
    printf("[download_file] 6.2\n");
    reset_file_cont(p_flcnt); // deallocate the file content
    printf("[download_file] 6.3\n");
    return &ret_flerr;
  }
  printf("[download_file] 7\n");

  // Check if an error has been occurred during the reading operation
  if (ferror(hfile)) {
    printf("[download_file] 7.1\n");
    p_errinf->num = 63;
    sprintf(p_errinf->errinf_u.msg,
            "Error occurred while reading the file: '%s'.\n"
            "System server error %i: %s", *flname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_errinf.errinf_u.msg
    fprintf(stderr, "Error %i: File Download Failed\n", p_errinf->num);
    fclose(hfile);
    printf("[download_file] 7.2\n");
    reset_file_cont(p_flcnt); // deallocate the file content
    printf("[download_file] 7.3\n");
    return &ret_flerr;
  }
  printf("[download_file] 8\n");

  fclose(hfile);
  printf("[download_file] 9\n\n");

  return &ret_flerr;
}

