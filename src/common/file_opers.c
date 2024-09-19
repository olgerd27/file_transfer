#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <errno.h>

#include "file_opers.h"
#include "mem_opers.h"
#include "fs_opers.h"

#define DBG_FLOP 1 // debug the file operations

extern int errno; // global system error number

/* Common functions for file operations */
/*
 * Allocate and reset the error information structure.
 *
 * This function allocates memory for an error information structure and resets its
 * contents to prepare it for storing error data. If memory allocation fails, it
 * prints an error message and returns a failure code.
 *
 * Parameters:
 *  pp_errinf - A double pointer to an error info structure to allocate and reset.
 *
 * Return value:
 *  0 on success, >0 on failure. (e.g., memory allocation error).
 */
static int alloc_reset_err_inf(err_inf **pp_errinf) {
    // Allocate memory if needed
    *pp_errinf = (err_inf*)malloc(sizeof(err_inf));
    if (!*pp_errinf) {
      fprintf(stderr, "Memory allocation failed for the error info instance\n");
      return 1;
    }
    
    // Allocate or reset the error message buffer
    (*pp_errinf)->err_inf_u.msg = NULL; // needed for a correct work of reset_err_inf()
    if (reset_err_inf(*pp_errinf) != 0) { // allocate memory for error message buffer
      fprintf(stderr, "Cannot reset the memory for the error info instance\n");
      return 2;
    }

    return 0;
}

/*
 * Prepare the err_inf for use, including allocation if necessary.

 * This function checks whether the provided double pointer to an error info
 * (`err_inf`) is `NULL` and, if needed, allocates memory for it.
 * The function ensures that the caller has a valid and initialized
 * `err_inf` structure to store error information.
 *
 * The function is typically used in file operation functions, where error
 * handling is required. It handles both cases where the caller provides
 * a pre-allocated error info structure or when `NULL` is passed and
 * a new structure needs to be allocated.
 *
 * NOTE. A call of this function resets the system errno value.
 *       It must be saved in advance.
 *
 * Parameters:
 *  p_errinf - Double pointer to an `err_inf` structure.
 *             - If `p_errinf` is `NULL`, no error handling is done, and
 *               the function returns an error.
 *             - If `*p_errinf` is `NULL`, the function attempts to allocate
 *               memory for `err_inf`.
 * Return value:
 *  0 on success, or <0 (-1) on failure.
 */
static int prepare_err_info(err_inf **pp_errinf)
{
  // no error info is provided
  if (!pp_errinf)
    return -1;

  // allocate the memory for the error info instance if it's not done previously
  if (!*pp_errinf && alloc_reset_err_inf(pp_errinf) != 0)
    return -1;

  return 0;
}

/* Open a file */
/*
 * Get an error message based on the file open mode.
 *
 * This helper function returns an appropriate error message based on the file
 * opening mode (e.g., "r", "w", etc.). If the mode is invalid or not found in
 * the predefined set, it returns a custom error message indicating an invalid mode.
 *
 * Parameters:
 *  mode - The mode in which the file was attempted to be opened (e.g., "r", "w").
 *
 * Return value:
 *  A string containing the error message for the corresponding mode or an error
 *  message indicating the mode is invalid.
 */
static const char *get_error_message(const char *mode) {
  // Error message struct to map file modes to error messages (for valid modes only)
  static const struct {
    const char *mode;
    const char *message;
  } error_messages[] = {
    { "r",    "Cannot open the file for reading" },
    { "rb",   "Cannot open the file for binary reading" },
    { "wx",   "The file already exists or could not be opened in write mode" },
    { "wbx",  "The file already exists or could not be opened in write binary mode" },
    { "w",    "Cannot open the file for writing" },
    { "wx",   "Cannot open the file for writing as it may already exist" },
    { "a",    "Cannot open the file for appending" },
    // Add more modes as needed...
  };

  // Search for the mode in the predefined error messages
  size_t num_modes = sizeof(error_messages) / sizeof(error_messages[0]);
  size_t i;
  for (i = 0; i < num_modes; i++)
      if (strcmp(error_messages[i].mode, mode) == 0)
          return error_messages[i].message;

  // If mode is not found it's invalid - create a custom message with the mode value
  static char msg_inv_mode[128];
  snprintf(msg_inv_mode, sizeof(msg_inv_mode), 
           "Cannot open the file in the requested invalid mode '%s'", mode);

  return msg_inv_mode; // return a default message if the mode is not found
}

/*
 * Open a file and optionally return error information.
 *
 * This function attempts to open a file in the specified mode. If the file cannot
 * be opened, and the error info pointer is provided, it allocates an error structure
 * and stores detailed error information.
 * If the passed error info double pointer is NULL, no error details are returned.
 * If a valid pointer is passed but uninitialized, the function allocates and initializes it.
 * If no error occurs, the error info pointer (pp_errinf) remains unchanged,
 * and the memory it points to is not modified. That means it can be safely reused.
 *
 * Note: The caller is responsible for freeing the error information structure if
 * the value passed was NULL and memory was allocated by this function. If a pointer
 * to a static or automatic instance of err_inf is passed, only the contents are modified,
 * not the pointer.
 *
 * Parameters:
 *  flname    - the name of the file to open.
 *  mode      - the mode in which to open the file (e.g., "r", "w").
 *  pp_errinf - a double pointer to an error info structure to store error information.
 *              If an error occurs, this structure is validated and allocated if necessary,
 *              and the error information (number and message) is saved in it.
 *              If NULL, no error info is provided. If uninitialized, memory is allocated.
 *
 * Return value:
 *  A pointer to a FILE object if successful, or NULL on failure.
 */
FILE *open_file(const char *const flname, const char *const mode, err_inf **pp_errinf)
{
  FILE *hfile = fopen(flname, mode);
  if (hfile == NULL) {
    // save errno before error info preparation that can reset errno
    int errno_save = errno;

    // prepare err_inf - validate and allocate if needed
    if ( prepare_err_info(pp_errinf) != 0 )
      return NULL;

    // Save the error info
    (*pp_errinf)->num = 60; // TODO: check the error number, or maybe use system error number?
    
    // construct the own error message + system error
    snprintf((*pp_errinf)->err_inf_u.msg, LEN_ERRMSG_MAX,
             "%s:\n%s\nSystem error %i: %s\n",
             get_error_message(mode), flname, errno_save, strerror(errno_save));
  }
  if (DBG_FLOP && hfile) printf("[open_file] DONE\n");
  return hfile;
}

/* Close a file */
/*
 * Close the file stream.
 * Closes the file stream and handles any errors that may occur during the process.
 * If an error occurs, this function prepares the error information using the provided
 * error info pointer (pp_errinf). The system error (errno) is captured and used
 * to populate the error message.
 *
 * Parameters:
 *  flname    - the name of the file to close.
 *  hfile     - the file stream to be closed.
 *  pp_errinf - a double pointer to an error info structure to store error information.
 *              If an error occurs, this structure is validated and allocated if necessary,
 *              and the error information (number and message) is saved in it.
 *              If NULL, no error info is provided. If uninitialized, memory is allocated.
 *
 * Return value:
 *  0       - if the file is successfully closed,
 *  <0 (-1) - on failure. In such cases, the error information is prepared, 
 *            and the system error message is stored in pp_errinf.
 */
int close_file(const char *const flname, FILE *hfile, err_inf **pp_errinf)
{
  int rc = fclose(hfile);
  if (rc != 0) {
    // save errno before error info preparation that can reset errno
    int errno_save = errno;

    // prepare err_inf - validate and allocate if needed
    if ( prepare_err_info(pp_errinf) != 0 )
      return -1;

    // save the error info
    (*pp_errinf)->num = 64;
    
    // construct the own error message + system error
    snprintf((*pp_errinf)->err_inf_u.msg, LEN_ERRMSG_MAX,
             "Failed to close the file:\n%s\n"
             "System error %i: %s\n",
             flname, errno_save, strerror(errno_save));
  }
  if (DBG_FLOP && !rc) printf("[close file] DONE\n");
  return rc;
}

/* Read a file */
// TODO: complete this function
// Allocate the memory to store the file content
static int allocate_file_content(const t_flname flname, t_flcont *p_flcont,
                                 FILE *hfile, err_inf **pp_errinf)
{
  if ( alloc_file_cont(p_flcont, get_file_size(hfile)) == NULL ) {
    // prepare err_inf - validate and allocate if needed
    if ( prepare_err_info(pp_errinf) != 0 )
      return -1;
    
    // construct the error message
    (*pp_errinf)->num = 61;
    snprintf((*pp_errinf)->err_inf_u.msg, LEN_ERRMSG_MAX,
             "Failed to allocate memory for the content of file:\n'%s'\n", flname);
    
    // close the file
    fclose(hfile);

    return (*pp_errinf)->num;
  }
  return 0;
}
