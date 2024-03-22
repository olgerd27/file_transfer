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
  printf("[reset_err] 1\n");
  if (!p_err) {
    fprintf(stderr, "Cannot reset error information, p_err=%p\n", (void*)p_err);
    exit(1);
  }

  // initial allocation of the memory for error messages
  if (!p_err->errinf_u.msg)
    p_err->errinf_u.msg = (char*)malloc(SIZE_ERRMSG);
  printf("[reset_err] 2\n");

  // reset the error information that is remained from the previous case when error has occurred
  if (p_err->num) {
    printf("[reset_err] 3\n");
    p_err->num = 0;
    if (p_err->errinf_u.msg)
      memset(p_err->errinf_u.msg, 0, SIZE_ERRMSG);
  }

  // reset the system error number
  if (errno) errno = 0;
  printf("[reset_err] 4 end\n");
}

/* 
 * The Upload file Section
 */
// TODO: break down the main upload function and move the functionality to the following functions:
// Open the file
//FILE * open_file_read(t_flname fname, errinf *p_err)
//{
//}

// Read the file content into the buffer
// RC: 0 on success; >0 on failure
//int write_file(t_flname fname, FILE *hfile, t_flcont *p_fcont, errinf *p_err)
//{
//}

// TODO: maybe use one universal close_file() for both upload and download functions?
// Close a file stream
//int close_file(FILE *hfile, t_flname fname, errinf *p_err)
//{
//}

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
    fprintf(stderr, "File Upload Failed - error %i\n", res_err.num);
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
    fprintf(stderr, "File Upload Failed - error %i\n", res_err.num);
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
    fprintf(stderr, "File Upload Failed - error %i\n", res_err.num);
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
// Reset (deallocate) the file content - freeing memory for the downloaded file content 
// that was allocated during the previous call of the download function
void reset_file_cont(t_flcont *file_cont)
{
  printf("[reset_file_cont] 1\n");
  if (file_cont && file_cont->t_flcont_val) {
    printf("[reset_file_cont] 1.1\n");
    free(file_cont->t_flcont_val);
    file_cont->t_flcont_val = NULL;
    file_cont->t_flcont_len = 0;
  }
  printf("[reset_file_cont] 2 end\n");
}

// Open the file
FILE * open_file_read(t_flname fname, errinf *p_err)
{
  printf("[open_file_read] 1\n");
  FILE *hfile = fopen(fname, "rb");

  if (hfile == NULL) {
    printf("[open_file_read] 2.1\n");
    p_err->num = 60;
    sprintf(p_err->errinf_u.msg,
            "Cannot open the file '%s' in the read mode.\n"
            "System server error %i: %s", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_errinf.errinf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
  }
  printf("[open_file_read] 3 end\n");
  return hfile;
}

// Get the file size
unsigned get_file_size(FILE *hfile)
{
  printf("[get_file_size] 1\n");
  fseek(hfile, 0, SEEK_END);
  unsigned size = ftell(hfile);
  rewind(hfile);
  printf("[get_file_size] 2 end\n");
  return size;
}

// Allocate the memory to store the file content
// A pointer to the allocated memory set to p_fcont and return as a result.
char * alloc_mem_file_cont(FILE *hfile, t_flcont *p_fcont, errinf *p_err)
{
  printf("[alloc_mem_file_cont] 1\n");
  // Get the file size
  p_fcont->t_flcont_len = get_file_size(hfile);

  p_fcont->t_flcont_val = (char*)malloc(p_fcont->t_flcont_len);
  if (p_fcont->t_flcont_val == NULL) {
    printf("[alloc_mem_file_cont] 2.1\n");
    p_err->num = 61;
    sprintf(p_err->errinf_u.msg,
            "Memory allocation error (size=%d) to store the file content:\n",
            p_fcont->t_flcont_len);
    // TODO: implement logging and put there the full error info taken from p_errinf.errinf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    fclose(hfile);
  }
  printf("[alloc_mem_file_cont] 3 end\n");
  return p_fcont->t_flcont_val;
}

// Read the file content into the buffer
// RC: 0 on success; >0 on failure
int read_file(FILE *hfile, t_flname fname, t_flcont *p_fcont, errinf *p_err)
{
  printf("[read_file] 1\n");
  size_t nch = fread(p_fcont->t_flcont_val, 1, p_fcont->t_flcont_len, hfile);

  // Check a number of items read
  if (nch < p_fcont->t_flcont_len) {
    printf("[read_file] 1.1\n");
    p_err->num = 62;
    sprintf(p_err->errinf_u.msg,
	    "Partial reading of the file: '%s'\n"
            "System server error %i: %s", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err.errinf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    fclose(hfile);
    reset_file_cont(p_fcont); // deallocate the file content
    return 1;
  }
  printf("[read_file] 2\n");

  // Check if an error has occurred during the reading operation
  if (ferror(hfile)) {
    printf("[read_file] 2.1\n");
    p_err->num = 63;
    sprintf(p_err->errinf_u.msg,
            "Failed to read from the file: '%s'\n"
            "System server error %i: %s", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err.errinf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    fclose(hfile);
    reset_file_cont(p_fcont); // deallocate the file content
    return 2;
  }
  printf("[read_file] 3 end\n");
  return 0;
}

// Close a file stream
int close_file(FILE *hfile, t_flname fname, t_flcont *p_fcont, errinf *p_err)
{
  printf("[close file] 1\n");
  int rc = fclose(hfile);
  if (rc != 0) {
    printf("[close file] 1.1\n");
    p_err->num = 64;
    sprintf(p_err->errinf_u.msg,
            "Failed to close the file: '%s'\n"
            "System server error %i: %s", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_errinf.errinf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    reset_file_cont(p_fcont); // deallocate the file content
  }
  printf("[close file] 2 end\n");
  return rc;
}

// The main function to Download a file
flcont_errinf * download_file_1_svc(t_flname *flname, struct svc_req *)
{
  printf("[download_file] 1\n");
  static flcont_errinf ret_flerr; /* must be static */
  static t_flcont *p_flcont = &ret_flerr.file_cont; // a pointer to a file content
  static errinf *p_errinf = &ret_flerr.err_inf; // a pointer to an error info

  // Reset (deallocate) the file content
  reset_file_cont(p_flcont);

  // Reset an error state remained after a previous call of this function
  reset_err(p_errinf);

  // Open the file
  FILE *hfile = open_file_read(*flname, p_errinf);
  if ( hfile == NULL ) {
    printf("[download_file] 2.1\n");
    return &ret_flerr;
  }

  // Allocate the memory to store the file content
  if ( alloc_mem_file_cont(hfile, p_flcont, p_errinf) == NULL ) {
    printf("[download_file] 2.2\n");
    return &ret_flerr;
  }

  // Read the file content into the buffer
  if ( read_file(hfile, *flname, p_flcont, p_errinf) != 0 ) {
    printf("[download_file] 2.3\n");
    return &ret_flerr;
  }

  // Close the file stream
  if ( close_file(hfile, *flname, p_flcont, p_errinf) != 0 ) {
    printf("[download_file] 2.4\n");
    return &ret_flerr;
  }

  printf("[download_file] 3 end\n\n");
  return &ret_flerr;
}

