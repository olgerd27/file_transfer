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
void reset_err_inf(err_inf *p_err)
{
  printf("[reset_err_inf] 1\n");
  if (!p_err) {
    fprintf(stderr, "Error: Cannot reset error information, p_err=%p\n", (void*)p_err);
    return;
  }

  // initial allocation of the memory for error messages
  if (!p_err->err_inf_u.msg) {
    printf("[reset_err_inf] 2 init memory allocation for error info\n");
    p_err->err_inf_u.msg = (char*)malloc(LEN_ERRMSG_MAX);
  }
  printf("[reset_err_inf] 3\n");

  // reset the error information that is remained from the previous case when error has occurred
  if (p_err->num) {
    printf("[reset_err_inf] 4 reset error info\n");
    p_err->num = 0;
    if (p_err->err_inf_u.msg)
      memset(p_err->err_inf_u.msg, 0, LEN_ERRMSG_MAX);
  }

  // reset the system error number
  if (errno) errno = 0;
  printf("[reset_err_inf] 5$\n");
}

/* 
 * The Upload file Section
 */
// Open the file in a write exclusive ('x') mode 
FILE * open_file_write_x(const t_flname fname, err_inf *p_err)
{
  printf("[open_file_write] 1\n");
  FILE *hfile = fopen(fname, "wbx");
  if (hfile == NULL) {
    printf("[open_file_write] 1.1 error fopen()\n");
    p_err->num = 50;
    sprintf(p_err->err_inf_u.msg, 
            "The file '%s' already exists or could not be opened in the write mode.\n"
            "System server error %i: %s\n", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
    fprintf(stderr, "File Upload Failed - error %i\n", p_err->num);
  }
  printf("[open_file_write] 2$\n");
  return hfile;
}

// Write a content of the client file to a new file.
// RC: 0 on success; >0 on failure
int write_file(FILE *hfile, file_inf *p_file, err_inf *p_err)
{
  printf("[write_file] 1\n");
  size_t nch = fwrite(p_file->cont.t_flcont_val, 1, p_file->cont.t_flcont_len, hfile);

  // Check a number of written items 
  if (nch < p_file->cont.t_flcont_len) {
    printf("[write_file] 1.1 error partial writing to a file\n");
    p_err->num = 51;
    sprintf(p_err->err_inf_u.msg, 
            "Partial writing to the file: '%s'.\n"
            "System server error %i: %s", p_file->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
    fprintf(stderr, "File Upload Failed - error %i\n", p_err->num);
    fclose(hfile);
    return 1;
  }
  printf("[write_file] 2\n");

  // Check if an error has occurred during the writing operation
  if (ferror(hfile)) {
    printf("[write_file] 2.1 error ferror()\n");
    p_err->num = 52;
    sprintf(p_err->err_inf_u.msg, 
            "Failed to read from the file: '%s'\n"
            "System server error %i: %s", p_file->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
    fprintf(stderr, "File Upload Failed - error %i\n", p_err->num);
    fclose(hfile);
    return 2;
  }
  printf("[write_file] 5$\n");
  return 0;
}

// Close file stream.
// This function doesn't print an error message to stderr and doesn't reset the file buffer if necessary.
// Customers must do this by themselves.
int close_file(FILE *hfile, const t_flname fname, err_inf *p_err)
{
  printf("[close file] 1\n");
  int rc = fclose(hfile);
  if (rc != 0) {
    printf("[close file] 1.1 error fclose()\n");
    p_err->num = 64;
    sprintf(p_err->err_inf_u.msg,
            "Failed to close the file: '%s'\n"
            "System server error %i: %s", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
  }
  printf("[close file] 2$\n");
  return rc;
}

// The main function to Upload a file
err_inf * upload_file_1_svc(file_inf *file_upld, struct svc_req *)
{
  printf("[upload_file] 1\n");
  static err_inf ret_err; /* returned variable, must be static */

  // Reset an error state remained after a previous call of the 'upload' function
  reset_err_inf(&ret_err);

  // Open the file
  FILE *hfile = open_file_write_x(file_upld->name, &ret_err);
  if (hfile == NULL) {
    printf("[upload_file] 1.1 error open a file\n\n");
    return &ret_err;
  }

  // Write a content of the client file to a new file
  if ( write_file(hfile, file_upld, &ret_err) != 0 ) {
    printf("[upload_file] 1.2 error write to a file\n\n");
    return &ret_err;
  }

  // Close the file stream
  if ( close_file(hfile, file_upld->name, &ret_err) != 0 ) {
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
// Reset (deallocate) the file info: set file name to NULL and free the memory for the 
// downloaded file content that was allocated during the previous call of the download function
void reset_file_inf(file_inf *p_file)
{
  printf("[reset_file_inf] 1\n");
  if (!p_file) {
    fprintf(stderr, "Error: Cannot reset the file info. p_file=%p\n", (void*)p_file);
    return;
  }

  // reset the file information
  if (p_file->cont.t_flcont_val) {
    printf("[reset_file_inf] 1.1\n");
    free(p_file->cont.t_flcont_val);
    p_file->cont.t_flcont_val = NULL;
    p_file->cont.t_flcont_len = 0;
    p_file->name = NULL;
  }
  printf("[reset_file_inf] 2$\n");
}

// Open the file
FILE * open_file_read(const t_flname fname, err_inf *p_err)
{
  printf("[open_file_read] 1\n");
  FILE *hfile = fopen(fname, "rb");
  if (hfile == NULL) {
    printf("[open_file_read] 1.1\n");
    p_err->num = 60;
    sprintf(p_err->err_inf_u.msg,
            "Cannot open the file '%s' in the read mode.\n"
            "System server error %i: %s", fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err.err_inf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
  }
  printf("[open_file_read] 2$\n");
  return hfile;
}

// Get the file size in bytes
// TODO: move this function to some common file in common/ directory and 
// call it from this file, because it's used in the client code as well.
unsigned get_file_size(FILE *hfile)
{
  fseek(hfile, 0, SEEK_END);
  unsigned size = ftell(hfile);
  rewind(hfile);
  return size;
}

// Allocate the memory to store the file content.
// The allocated memory is set to p_file argument.
char * alloc_mem_file_cont(FILE *hfile, file_inf *p_file, err_inf *p_err)
{
  printf("[alloc_mem_file_cont] 1\n");
  // Get the file size
  p_file->cont.t_flcont_len = get_file_size(hfile);

  p_file->cont.t_flcont_val = (char*)malloc(p_file->cont.t_flcont_len);
  if (p_file->cont.t_flcont_val == NULL) {
    printf("[alloc_mem_file_cont] 1.1\n");
    p_err->num = 61;
    sprintf(p_err->err_inf_u.msg,
            "Memory allocation error to store the content of file '%s'\n", p_file->name);
    // TODO: implement logging and put there the full error info taken from p_err.err_inf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    fclose(hfile);
  }
  printf("[alloc_mem_file_cont] 2$\n");
  return p_file->cont.t_flcont_val;
}

// Read the file content into the buffer.
// RC: 0 on success; >0 on failure
int read_file(FILE *hfile, file_inf *p_file, err_inf *p_err)
{
  printf("[read_file] 1\n");
  size_t nch = fread(p_file->cont.t_flcont_val, 1, p_file->cont.t_flcont_len, hfile);

  // Check a number of items read
  if (nch < p_file->cont.t_flcont_len) {
    printf("[read_file] 1.1\n");
    p_err->num = 62;
    sprintf(p_err->err_inf_u.msg,
	    "Partial reading of the file: '%s'\n"
            "System server error %i: %s", p_file->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    fclose(hfile);
    reset_file_inf(p_file); // deallocate the file content
    return 1;
  }
  printf("[read_file] 2\n");

  // Check if an error has occurred during the reading operation
  if (ferror(hfile)) {
    printf("[read_file] 2.1\n");
    p_err->num = 63;
    sprintf(p_err->err_inf_u.msg,
            "Failed to read from the file: '%s'\n"
            "System server error %i: %s", p_file->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    fclose(hfile);
    reset_file_inf(p_file); // deallocate the file content
    return 2;
  }
  printf("[read_file] 3$\n");
  return 0;
}

// The main function to Download a file
file_err * download_file_1_svc(t_flname *flname, struct svc_req *)
{
  printf("[download_file] 1\n");
  static file_err ret_flerr; /* returned variable, must be static */
  static file_inf *p_fileinf = &ret_flerr.file; // a pointer to a file info
  static err_inf *p_errinf = &ret_flerr.err; // a pointer to an error info

  // Reset the data remained from the previous call of 'download' function
  reset_file_inf(p_fileinf); // reset a file info
  reset_err_inf(p_errinf); // reset an error info

  // Assign the file name
  p_fileinf->name = *flname;

  // Open the file
  FILE *hfile = open_file_read(p_fileinf->name, p_errinf);
  if ( hfile == NULL ) {
    printf("[download_file] 1.1\n\n");
    return &ret_flerr;
  }

  // Allocate the memory to store the file content
  if ( alloc_mem_file_cont(hfile, p_fileinf, p_errinf) == NULL ) {
    printf("[download_file] 1.2\n\n");
    fprintf(stderr, "File Download Failed - error %i\n", p_errinf->num);
    return &ret_flerr;
  }

  // Read the file content into the buffer
  if ( read_file(hfile, p_fileinf, p_errinf) != 0 ) {
    printf("[download_file] 1.3\n\n");
    return &ret_flerr;
  }

  // Close the file stream
  if ( close_file(hfile, p_fileinf->name, p_errinf) != 0 ) {
    printf("[download_file] 1.4\n\n");
    fprintf(stderr, "File Download Failed - error %i\n", p_errinf->num);
    reset_file_inf(p_fileinf); // deallocate the file content
    return &ret_flerr;
  }

  printf("[download_file] 2$\n\n");
  return &ret_flerr;
}

