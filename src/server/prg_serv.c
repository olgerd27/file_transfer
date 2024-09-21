/*
 * prg_serv.c: implementation of the server functions for file transfers.
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../common/mem_opers.h" /* for the memory manipulations */
#include "../common/fs_opers.h" /* for working with the File System */
#include "../common/file_opers.h" /* for the files manipulations */

#define DBG_SERV 1

extern int errno; // global system error number

// NOTE: it was made the same approach for all the error messages: the error messages with
// the short info should be provided to the client through error info object, and the error
// messages with the extended info should be printed to STDERR on the server side only.

// Print the error message in special format to STDERR
void print_error(const char *oper_type, const struct err_inf *p_errinf)
{
  fprintf(stderr, "%s Failed - error %i\n%s\n",
          oper_type, p_errinf->num, p_errinf->err_inf_u.msg);
}

/* 
 * The Upload file Section
 */
// The main RPC function to Upload a file
// TODO: check if the file content, passed as an argument, should be freed at the end of this function 
//       and in case of errors?
err_inf * upload_file_1_svc(file_inf *file_upld, struct svc_req *)
{
  if (DBG_SERV)
    printf("[upload_file] 0, request to Upload file and save it on server as:\n  %s\n", 
            file_upld->name);
  static err_inf ret_err; // returned variable, must be static
  static err_inf *p_ret_err = &ret_err; // pointer to a returned static variable
  FILE *hfile;            // the file handler

  // Reset an error state remained after a previous call of the 'upload' function
  if ( reset_err_inf(p_ret_err) != 0 ) {
    // Return a special value if an error has occurred while initializing the error info
    ret_err.num = ERRNUM_ERRINF_ERR;
    ret_err.err_inf_u.msg = "Failed to init the error info\n";
    print_error("Upload", p_ret_err); // print the error message to STDERR
    return p_ret_err;
  }

  if (DBG_SERV) printf("[upload_file] 1, error info was init'ed\n");

  // Open the file
  if ( (hfile = open_file(file_upld->name, "wbx", &p_ret_err)) == NULL ) {
    print_error("Upload", p_ret_err); // print the error message to STDERR
    return p_ret_err;
  }

  if (DBG_SERV) printf("[upload_file] 2 file openned\n");

  // Write a content of the client file to a new file
  if ( write_file(file_upld->name, &file_upld->cont, hfile, &p_ret_err) != 0 ) {
    print_error("Upload", p_ret_err); // print the error message to STDERR
    return p_ret_err;
  }

  if (DBG_SERV) printf("[upload_file] 3 file has written\n");

  // Close the file stream
  if ( close_file(file_upld->name, hfile, &p_ret_err) != 0 ) {
    print_error("Upload", p_ret_err);
    return p_ret_err;
  }

  if (DBG_SERV) printf("[upload_file] DONE\n\n");
  return p_ret_err;
}

/*
 * The Download file Section
 */
// The main RPC function to Download a file
file_err * download_file_1_svc(t_flname *p_flname, struct svc_req *)
{
  if (DBG_SERV)
    printf("[download_file] 0, request to Download server's file:\n  %s\n", *p_flname);
  static file_err ret_flerr; // returned variable, must be static
  static file_inf *p_fileinf = &ret_flerr.file; // a pointer to a file info
  static err_inf *p_errinf = &ret_flerr.err; // a pointer to an error info
  FILE *hfile;  // the file handler

  // Reset an error info remained from the previous call of 'download' function
  if ( reset_err_inf(p_errinf) != 0 ) {
    // Return a special value if an error has occurred while initializing the error info
    p_errinf->num = ERRNUM_ERRINF_ERR;
    p_errinf->err_inf_u.msg = "Failed to init the error info\n";
    print_error("Download", p_errinf);
    return &ret_flerr;
  }

  if (DBG_SERV) printf("[download_file] 1, error info was init'ed\n");

  // Init the file name & type info remained from the previous call of 'download' function
  if ( reset_file_name_type(p_fileinf) != 0 ) {
    p_errinf->num = 64;
    sprintf(p_errinf->err_inf_u.msg, "Failed to init the file name & type\n");
    print_error("Download", p_errinf);
    return &ret_flerr;
  }

  if (DBG_SERV) printf("[download_file] 2, file name & type was init'ed\n");

  // Set the file name to be read
  p_fileinf->name = *p_flname;

  // Open the file
  if ( (hfile = open_file(p_fileinf->name, "rb", &p_errinf)) == NULL ) {
    print_error("Download", p_errinf);
    return &ret_flerr;
  }

  if (DBG_SERV) printf("[download_file] 3 file openned\n");

  // Read the file content into the buffer
  if ( read_file(p_fileinf->name, &p_fileinf->cont, hfile, &p_errinf) != 0 ) {
    print_error("Download", p_errinf);
    xdr_free((xdrproc_t)xdr_file_inf, p_fileinf); // free the file info
    return &ret_flerr;
  }

  if (DBG_SERV) printf("[download_file] 5 file has read\n");

  // Close the file stream
  if ( close_file(p_fileinf->name, hfile, &p_errinf) != 0 ) {
    print_error("Download", p_errinf);
    xdr_free((xdrproc_t)xdr_file_inf, p_fileinf); // free the file info
    return &ret_flerr;
  }

  if (DBG_SERV) printf("[download_file] DONE\n\n");
  return &ret_flerr;
}

// The main RPC function for interactive pick a file on the server
file_err * pick_file_1_svc(picked_file *p_flpkd, struct svc_req *)
{
  if (DBG_SERV) printf("[pick_file] 0\n");
  static file_err *p_flerr_ret; // returned pointer, must be static
  p_flerr_ret = select_file(p_flpkd); // select_file() returns pointer to a static file_err object
  if (DBG_SERV) printf("[pick_file] DONE\n\n");
  return p_flerr_ret;
}