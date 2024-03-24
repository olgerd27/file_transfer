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
  if (!p_err->errinf_u.msg) {
    printf("[reset_err] 2 allocate memory for error message\n");
    p_err->errinf_u.msg = (char*)malloc(SIZE_ERRMSG);
  }
  printf("[reset_err] 3\n");

  // reset the error information that is remained from the previous case when error has occurred
  if (p_err->num) {
    printf("[reset_err] 4 reset error info\n");
    p_err->num = 0;
    if (p_err->errinf_u.msg)
      memset(p_err->errinf_u.msg, 0, SIZE_ERRMSG);
  }

  // reset the system error number
  if (errno) errno = 0;
  printf("[reset_err] 5$\n");
}

/* 
 * The Upload file Section
 */
// Open the file in a write exclusive ('x') mode 
FILE * open_file_write_x(const t_flname fname, errinf *p_err)
{
  printf("[open_file_write] 1\n");
  FILE *hfile = fopen(fname, "wbx");
  if (hfile == NULL) {
    printf("[open_file_write] 1.1 error fopen()\n");
    p_err->num = 50;
    sprintf(p_err->errinf_u.msg, 
            "The file '%s' already exists or could not be opened in the write mode.\n"
            "System server error %i: %s\n", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->errinf_u.msg
    fprintf(stderr, "File Upload Failed - error %i\n", p_err->num);
  }
  printf("[open_file_write] 2$\n");
  return hfile;
}

// Write a content of the client file to a new file
// RC: 0 on success; >0 on failure
int write_file(FILE *hfile, const t_flname fname, t_flcont *p_fcont, errinf *p_err)
{
  printf("[write_file] 1\n");
  size_t nch = fwrite(p_fcont->t_flcont_val, 1, p_fcont->t_flcont_len, hfile);

  // Check a number of written items 
  if (nch < p_fcont->t_flcont_len) {
    printf("[write_file] 1.1 error partial writing to a file\n");
    p_err->num = 51;
    sprintf(p_err->errinf_u.msg, 
            "Partial writing to the file: '%s'.\n"
            "System server error %i: %s", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->errinf_u.msg
    fprintf(stderr, "File Upload Failed - error %i\n", p_err->num);
    fclose(hfile);
    return 1;
  }
  printf("[write_file] 2\n");

  // Check if an error has occurred during the writing operation
  if (ferror(hfile)) {
    printf("[write_file] 2.1 error ferror()\n");
    p_err->num = 52;
    sprintf(p_err->errinf_u.msg, 
            "Failed to read from the file: '%s'\n"
            "System server error %i: %s", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->errinf_u.msg
    fprintf(stderr, "File Upload Failed - error %i\n", p_err->num);
    fclose(hfile);
    return 2;
  }
  printf("[write_file] 5$\n");
  return 0;
}

// Close a file stream
// This function doesn't print an error message to stderr and doesn't reset the file buffer if necessary.
// Customers must do this by themselves.
int close_file(FILE *hfile, const t_flname fname, t_flcont *p_fcont, errinf *p_err)
{
  printf("[close file] 1\n");
  int rc = fclose(hfile);
  if (rc != 0) {
    printf("[close file] 1.1 error fclose()\n");
    p_err->num = 64;
    sprintf(p_err->errinf_u.msg,
            "Failed to close the file: '%s'\n"
            "System server error %i: %s", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->errinf_u.msg
  }
  printf("[close file] 2$\n");
  return rc;
}

// The main function to Upload a file
errinf * upload_file_1_svc(file *file_upld, struct svc_req *)
{
  printf("[upload_file] 1\n");
  static errinf ret_err; /* must be static */

  // Reset an error state remained after a previous call of the 'upload' function
  reset_err(&ret_err);

  // Open the file
  FILE *hfile = open_file_write_x(file_upld->name, &ret_err);
  if (hfile == NULL) {
    printf("[upload_file] 1.1 error open a file\n\n");
    return &ret_err;
  }

  // Write a content of the client file to a new file
  if ( write_file(hfile, file_upld->name, &file_upld->cont, &ret_err) != 0 ) {
    printf("[upload_file] 1.2 error write to a file\n\n");
    return &ret_err;
  }

  // Close the file stream
  if ( close_file(hfile, file_upld->name, &file_upld->cont, &ret_err) != 0 ) {
    printf("[download_file] 1.3 error file close\n\n");
    fprintf(stderr, "File Upload Failed - error %i\n", ret_err.num);
    return &ret_err;
  }

  printf("[upload_file] 2$\n\n");
  return &ret_err;
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
  printf("[reset_file_cont] 2$\n");
}

// Open the file
FILE * open_file_read(const t_flname fname, errinf *p_err)
{
  printf("[open_file_read] 1\n");
  FILE *hfile = fopen(fname, "rb");
  if (hfile == NULL) {
    printf("[open_file_read] 1.1\n");
    p_err->num = 60;
    sprintf(p_err->errinf_u.msg,
            "Cannot open the file '%s' in the read mode.\n"
            "System server error %i: %s", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_errinf.errinf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
  }
  printf("[open_file_read] 2$\n");
  return hfile;
}

// Get the file size
unsigned get_file_size(FILE *hfile)
{
  printf("[get_file_size] 1\n");
  fseek(hfile, 0, SEEK_END);
  unsigned size = ftell(hfile);
  rewind(hfile);
  printf("[get_file_size] 2$\n");
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
    printf("[alloc_mem_file_cont] 1.1\n");
    p_err->num = 61;
    sprintf(p_err->errinf_u.msg,
            "Memory allocation error (size=%d) to store the file content:\n",
            p_fcont->t_flcont_len);
    // TODO: implement logging and put there the full error info taken from p_errinf.errinf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    fclose(hfile);
  }
  printf("[alloc_mem_file_cont] 2$\n");
  return p_fcont->t_flcont_val;
}

// Read the file content into the buffer
// RC: 0 on success; >0 on failure
int read_file(FILE *hfile, const t_flname fname, t_flcont *p_fcont, errinf *p_err)
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
    // TODO: implement logging and put there the full error info taken from p_err->errinf_u.msg
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
    // TODO: implement logging and put there the full error info taken from p_err->errinf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    fclose(hfile);
    reset_file_cont(p_fcont); // deallocate the file content
    return 2;
  }
  printf("[read_file] 3$\n");
  return 0;
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

  // Reset an error state remained after a previous call of the 'download' function
  reset_err(p_errinf);

  // Open the file
  FILE *hfile = open_file_read(*flname, p_errinf);
  if ( hfile == NULL ) {
    printf("[download_file] 1.1\n\n");
    return &ret_flerr;
  }

  // Allocate the memory to store the file content
  if ( alloc_mem_file_cont(hfile, p_flcont, p_errinf) == NULL ) {
    printf("[download_file] 1.2\n\n");
    return &ret_flerr;
  }

  // Read the file content into the buffer
  if ( read_file(hfile, *flname, p_flcont, p_errinf) != 0 ) {
    printf("[download_file] 1.3\n\n");
    return &ret_flerr;
  }

  // Close the file stream
  if ( close_file(hfile, *flname, p_flcont, p_errinf) != 0 ) {
    printf("[download_file] 1.4\n\n");
    fprintf(stderr, "File Download Failed - error %i\n", p_errinf->num);
    reset_file_cont(p_flcont); // deallocate the file content
    return &ret_flerr;
  }

  printf("[download_file] 2$\n\n");
  return &ret_flerr;
}

