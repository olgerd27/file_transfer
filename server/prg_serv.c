/*
 * prg_serv.c: implementation of the server functions for file transfers.
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* RPC protocol definitions - created by rpcgen */
#include "../common/fs_opers.h" /* functions for working with the File System */
#include "../common/mem_opers.h" /* function for the memory manipulations */

#define DBG_SERV 1

extern int errno; // global system error number

/* 
 * The Upload file Section
 */
// Open the file in a write exclusive ('x') mode 
FILE * open_file_write_x(const t_flname fname, err_inf *p_err)
{
  if (DBG_SERV) printf("[open_file_write] 1\n");
  FILE *hfile = fopen(fname, "wbx");
  if (hfile == NULL) {
    p_err->num = 50;
    sprintf(p_err->err_inf_u.msg,
            "The file '%s' already exists or could not be opened in the write mode.\n"
            "System server error %i: %s\n",
            fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
    fprintf(stderr, "File Upload Failed - error %i\n", p_err->num);
  }
  if (DBG_SERV) printf("[open_file_write] DONE\n");
  return hfile;
}

// Write a content of the client file to a new file.
// RC: 0 on success; >0 on failure
int write_file(FILE *hfile, file_inf *p_file, err_inf *p_err)
{
  if (DBG_SERV) printf("[write_file] 1\n");
  size_t nch = fwrite(p_file->cont.t_flcont_val, 1, p_file->cont.t_flcont_len, hfile);
  
  if (DBG_SERV) printf("[write_file] 2 writing completed\n");

  // Check a number of written items 
  if (nch < p_file->cont.t_flcont_len) {
    p_err->num = 51;
    sprintf(p_err->err_inf_u.msg,
            "Partial writing to the file: '%s'.\n"
            "System server error %i: %s",
            p_file->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
    fprintf(stderr, "File Upload Failed - error %i\n", p_err->num);
    fclose(hfile);
    return 1;
  }

  // Check if an error has occurred during the writing operation
  if (ferror(hfile)) {
    p_err->num = 52;
    sprintf(p_err->err_inf_u.msg,
            "Failed to read from the file: '%s'\n"
            "System server error %i: %s",
            p_file->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
    fprintf(stderr, "File Upload Failed - error %i\n", p_err->num);
    fclose(hfile);
    return 2;
  }
  if (DBG_SERV) printf("[write_file] DONE\n");
  return 0;
}

// TODO: move this function before any functions that call fclose() and 
// call it instead of fclose()
// Close file stream.
// This function doesn't print an error message to stderr and doesn't reset the 
// file buffer if necessary. Customers must do this by themselves.
int close_file(FILE *hfile, const t_flname fname, err_inf *p_err)
{
  if (DBG_SERV) printf("[close file] 1\n");
  int rc = fclose(hfile);
  if (rc != 0) {
    p_err->num = 64;
    sprintf(p_err->err_inf_u.msg,
            "Failed to close the file: '%s'\n"
            "System server error %i: %s",
            fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
  }
  if (DBG_SERV) printf("[close file] DONE\n");
  return rc;
}

// The main function to Upload a file
err_inf * upload_file_1_svc(file_inf *file_upld, struct svc_req *)
{
  if (DBG_SERV) printf("[upload_file] 1\n");
  static err_inf ret_err; /* returned variable, must be static */

  // Reset an error state remained after a previous call of the 'upload' function
  if ( reset_err_inf(&ret_err) != 0 ) {
    // NOTE: just a workaround - return a special value if an error has occurred
    // while resetting the error info
    // TODO: client should process the case if ERRNUM_ERRINF_ERR has returned by server
    ret_err.num = ERRNUM_ERRINF_ERR;
    fprintf(stderr, "File Upload Failed - error %i\nFailed to reset the error info\n",
            ret_err.num);
    return &ret_err;
  }

  if (DBG_SERV) printf("[upload_file] 2 error info reset\n");

  // Open the file
  FILE *hfile = open_file_write_x(file_upld->name, &ret_err);
  if (hfile == NULL)
    return &ret_err;

  if (DBG_SERV) printf("[upload_file] 3 file openned\n");

  // Write a content of the client file to a new file
  if ( write_file(hfile, file_upld, &ret_err) != 0 )
    return &ret_err;

  if (DBG_SERV) printf("[upload_file] 4 file has written\n");

  // Close the file stream
  if ( close_file(hfile, file_upld->name, &ret_err) != 0 ) {
    fprintf(stderr, "File Upload Failed - error %i\n\n", ret_err.num);
    return &ret_err;
  }

  if (DBG_SERV) printf("[upload_file] DONE\n\n");
  return &ret_err;
}

/*
 * The Download file Section
 */
// Open the file
FILE * open_file_read(const t_flname fname, err_inf *p_err)
{
  if (DBG_SERV) printf("[open_file_read] 1\n");
  FILE *hfile = fopen(fname, "rb");
  if (hfile == NULL) {
    p_err->num = 60;
    sprintf(p_err->err_inf_u.msg,
            "Cannot open the file '%s' in the read mode.\n"
            "System server error %i: %s",
            fname, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err.err_inf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
  }
  if (DBG_SERV) printf("[open_file_read] DONE\n");
  return hfile;
}

// Get the file size in bytes
// TODO: move this function to some common file in common/ directory and 
// call it from this file, because it's used in the client code as well.
size_t get_file_size(FILE *hfile)
{
  fseek(hfile, 0, SEEK_END);
  unsigned size = ftell(hfile);
  rewind(hfile);
  return size;
}

// Read the file content into the buffer.
// RC: 0 on success; >0 on failure
int read_file(FILE *hfile, file_inf *p_file, err_inf *p_err)
{
  if (DBG_SERV) printf("[read_file] 1\n");
  size_t nch = fread(p_file->cont.t_flcont_val, 1, p_file->cont.t_flcont_len, hfile);
  
  if (DBG_SERV) printf("[read_file] 2 reading completed\n");

  // Check a number of items read
  if (nch < p_file->cont.t_flcont_len) {
    p_err->num = 62;
    sprintf(p_err->err_inf_u.msg,
            "Partial reading of the file: '%s'\n"
            "System server error %i: %s",
            p_file->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    fclose(hfile);
    free_file_inf(p_file); // deallocate the file content
    return 1;
  }

  // Check if an error has occurred during the reading operation
  if (ferror(hfile)) {
    p_err->num = 63;
    sprintf(p_err->err_inf_u.msg,
            "Failed to read from the file: '%s'\n"
            "System server error %i: %s",
            p_file->name, errno, strerror(errno));
    // TODO: implement logging and put there the full error info taken from p_err->err_inf_u.msg
    fprintf(stderr, "File Download Failed - error %i\n", p_err->num);
    fclose(hfile);
    free_file_inf(p_file); // deallocate the file content
    return 2;
  }
  if (DBG_SERV) printf("[read_file] DONE\n");
  return 0;
}

// The main function to Download a file
// TODO: delete reduntent debug messages or substitute some of them with info messages
// TODO: make the same approach for all the error messages printed by this functions and
// the functions called by this function. I suppose the following approach:
// the error messages with the short info should be provided to the client through error info object,
// and the error messages with the extended info should be printed in stderr on the server side.
// TODO: maybe create the function to print the error message in stderr like:
// void print_err(const char *oper_type, const struct err_inf *p_errinf)
// {
//    fprintf(stderr, "File %s Failed - error %i\n%s", oper_type, p_errinf->num, p_errinf->err_inf_u.msg)
// }
// Call:
// print_err("Download", p_errinf);
file_err * download_file_1_svc(t_flname *flname, struct svc_req *)
{
  printf("[download_file] 1\n");
  static file_err ret_flerr; /* returned variable, must be static */
  static file_inf *p_fileinf = &ret_flerr.file; // a pointer to a file info
  static err_inf *p_errinf = &ret_flerr.err; // a pointer to an error info

  // Reset an error info remained from the previous call of 'download' function
  if ( reset_err_inf(p_errinf) != 0 ) {
    // NOTE: just a workaround - return a special value if an error has occurred
    // while resetting the error info
    // TODO: client should process the case if ERRNUM_ERRINF_ERR has returned by server
    p_errinf->num = ERRNUM_ERRINF_ERR;
    fprintf(stderr, "File Download Failed - error %i\nFailed to reset the error info\n", 
            p_errinf->num);
    return &ret_flerr;
  }

  if (DBG_SERV) printf("[download_file] 2 error info reset\n");

  // Reset the file name & type info remained from the previous call of 'download' function
  if ( reset_file_name_type(p_fileinf) != 0 ) {
    p_errinf->num = 64;
    sprintf(p_errinf->err_inf_u.msg, "Failed to reset the file name & type\n");
    fprintf(stderr, "File Download Failed - error %i\n%s", 
            p_errinf->num, p_errinf->err_inf_u.msg);
    return &ret_flerr;
  }

  if (DBG_SERV) printf("[download_file] 3 file name & type reset\n");

  // Assign the file name
  p_fileinf->name = *flname;

  // Open the file
  // TODO: think, maybe create one file openning function to process both reading and writing
  FILE *hfile = open_file_read(p_fileinf->name, p_errinf);
  if ( hfile == NULL )
    return &ret_flerr;
  if (DBG_SERV) printf("[download_file] 4 file openned\n");

  // Allocate the memory to store the file content  
  if ( alloc_file_cont(&p_fileinf->cont, get_file_size(hfile)) == NULL ) {
    p_errinf->num = 61;
    sprintf(p_errinf->err_inf_u.msg,
            "Failed to allocate memory for the content of file:\n'%s'\n", p_fileinf->name);
    fprintf(stderr, "File Download Failed - error %i\n%s", 
            p_errinf->num, p_errinf->err_inf_u.msg);
    return &ret_flerr;
  }

  if (DBG_SERV) printf("[download_file] 5 memory for file content allocated\n");

  // Read the file content into the buffer
  if ( read_file(hfile, p_fileinf, p_errinf) != 0 )
    return &ret_flerr;

  if (DBG_SERV) printf("[download_file] 6 file has read\n");

  // Close the file stream
  if ( close_file(hfile, p_fileinf->name, p_errinf) != 0 ) {
    fprintf(stderr, "File Download Failed - error %i\n\n", p_errinf->num);
    free_file_inf(p_fileinf); // deallocate the file content
    return &ret_flerr;
  }

  if (DBG_SERV) printf("[download_file] DONE\n\n");
  return &ret_flerr;
}
