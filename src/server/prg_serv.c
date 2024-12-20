/*
 * prg_serv.c: the Server program to process the remote requests.
 * Errors range: 1-5 (reserve 6-10)
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../common/mem_opers.h" /* for the memory manipulations */
#include "../common/fs_opers.h" /* for working with the File System */
#include "../common/file_opers.h" /* for the files manipulations */
#include "../common/logging.h" /* for logging */

extern int errno; // global system error number

// NOTE: it was made the same approach for all the error messages: the error messages with
// the short info should be provided to the client through error info object, and the error
// messages with the extended info should be printed to STDERR on the server side only.

// Print the error message in special format to STDERR
static void print_error(const char *oper_type, const struct err_inf *p_errinf)
{
  fprintf(stderr, "%s Failed - error %i\n%s\n",
          oper_type, p_errinf->num, p_errinf->err_inf_u.msg);
}

// The main RPC function to Upload a file.
// Note: file_inf and err_inf objects will be auto-freed by xdr_free() at function end.
err_inf * upload_file_1_svc(file_inf *file_upld, struct svc_req *)
{
  LOG(LOG_TYPE_SERV, LOG_LEVEL_DEBUG, "Begin");
  static err_inf ret_err; // returned variable, must be static
  static err_inf *p_ret_err = &ret_err; // pointer to a returned static variable
  FILE *hfile;            // the file handler
  LOG(LOG_TYPE_SERV, LOG_LEVEL_INFO,
      "process the Upload file request, save file as: %s", file_upld->name);

  // Reset an error state remained after a previous call of the 'upload' function
  if ( reset_err_inf(p_ret_err) != 0 ) {
    // Return a special value if an error has occurred while initializing the error info
    ret_err.num = ERRNUM_ERRINF_ERR;
    ret_err.err_inf_u.msg = "Failed to init the error info\n";
    print_error("Upload", p_ret_err); // print the error message to STDERR
    LOG(LOG_TYPE_SERV, LOG_LEVEL_ERROR, "%s", ret_err.err_inf_u.msg);
    return p_ret_err;
  }

  LOG(LOG_TYPE_SERV, LOG_LEVEL_DEBUG, "error info was init'ed");

  // Save the passed file content to a new local file
  if ( save_file_cont(file_upld->name, &file_upld->cont, &p_ret_err) != 0 ) {
    print_error("Upload", p_ret_err);
    LOG(LOG_TYPE_SERV, LOG_LEVEL_ERROR, "Failed to save file contents");
    return p_ret_err;
  }
  LOG(LOG_TYPE_SERV, LOG_LEVEL_INFO, "file was saved successfully");
  LOG(LOG_TYPE_SERV, LOG_LEVEL_DEBUG, "Done.\n");
  return p_ret_err;
}

// The main RPC function to Download a file.
// Note: file_err object will be auto-freed by xdr_free() at function end.
file_err * download_file_1_svc(t_flname *p_flname, struct svc_req *)
{
  LOG(LOG_TYPE_SERV, LOG_LEVEL_DEBUG, "Begin");
  static file_err ret_flerr; // returned variable, must be static
  static file_inf *p_fileinf = &ret_flerr.file; // a pointer to a file info
  static err_inf *p_errinf = &ret_flerr.err; // a pointer to an error info
  FILE *hfile;  // the file handler
  LOG(LOG_TYPE_SERV, LOG_LEVEL_INFO, 
      "process the Download file request, read file: %s", *p_flname);

  // Reset an error info remained from the previous call of 'download' function
  if ( reset_err_inf(p_errinf) != 0 ) {
    // Return a special value if an error has occurred while initializing the error info
    p_errinf->num = ERRNUM_ERRINF_ERR;
    p_errinf->err_inf_u.msg = "Failed to init the error info\n";
    print_error("Download", p_errinf);
    LOG(LOG_TYPE_SERV, LOG_LEVEL_ERROR, "%s", p_errinf->err_inf_u.msg);
    return &ret_flerr;
  }

  LOG(LOG_TYPE_SERV, LOG_LEVEL_DEBUG, "error info was init'ed");

  // Init the file name & type info remained from the previous call of 'download' function
  if ( reset_file_name_type(p_fileinf) != 0 ) {
    p_errinf->num = 1;
    sprintf(p_errinf->err_inf_u.msg, "Failed to init the file name & type\n");
    print_error("Download", p_errinf);
    LOG(LOG_TYPE_SERV, LOG_LEVEL_ERROR, "%s", p_errinf->err_inf_u.msg);
    return &ret_flerr;
  }

  LOG(LOG_TYPE_SERV, LOG_LEVEL_DEBUG, "file name & type was init'ed");

  // Set the file name to be read
  p_fileinf->name = *p_flname;

  // Read the file content into the buffer
  if ( read_file_cont(p_fileinf->name, &p_fileinf->cont, &p_errinf) != 0 ) {
    print_error("Download", p_errinf);
    LOG(LOG_TYPE_SERV, LOG_LEVEL_ERROR, "Failed to read file contents");
    return &ret_flerr;
  }
  LOG(LOG_TYPE_SERV, LOG_LEVEL_INFO, "file was read successfully");
  LOG(LOG_TYPE_SERV, LOG_LEVEL_DEBUG, "Done.\n");
  return &ret_flerr;
}

// The main RPC function for Interactive Selection (Picking) a file on the server.
// Note: file_err object will be auto-freed by xdr_free() at function end.
file_err * pick_file_1_svc(picked_file *p_flpkd, struct svc_req *)
{
  LOG(LOG_TYPE_SERV, LOG_LEVEL_DEBUG, "Begin");
  static file_err *p_flerr_ret; // returned pointer, must be static
  LOG(LOG_TYPE_SERV, LOG_LEVEL_INFO, "process the Pick file request: %s", p_flpkd->name);
  
  p_flerr_ret = select_file(p_flpkd); // select_file() returns pointer to a static file_err object
  if (p_flerr_ret->err.num != 0)
    LOG(LOG_TYPE_SERV, LOG_LEVEL_ERROR, 
        "Failed selection: %s\n", p_flerr_ret->err.num, p_flerr_ret->err.err_inf_u.msg);
  
  LOG(LOG_TYPE_SERV, LOG_LEVEL_DEBUG, "Done.\n");
  return p_flerr_ret;
}