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
/* Allocate and reset the error information structure.
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

/* Process error info and format an error message.
 *
 * This function processes error information and constructs an error message that
 * includes details about the failed action, the associated file, and optionally,
 * system error details if `errno` is non-zero. It ensures the error info structure
 * is prepared (allocated if needed) and stores the error message in the provided buffer.
 *
 * Parameters:
 *  filename    - the name of the file associated with the error.
 *  errnum      - the custom error number to be stored in the error info structure.
 *  errmsg_act  - a brief message describing the failed action.
 *  p_errinf    - a double pointer to an `err_inf` structure.
 *                -> If `p_errinf` is `NULL`, no error handling is done, and
 *                   the function returns an error code.
 *                -> If `*pp_errinf` is `NULL`, the function attempts to allocate
 *                   memory for `err_inf` and initialize it.
 * Return value:
 *  0 on success,
 * <0 (-1) on failure (error info could not be processed or allocated).
 *
 *  Note: The system error (based on `errno`) is included in the message only if
 * `errno` is non-zero at the time the function is called.
 */
static int process_error(const char *filename, int errnum, 
                         const char *errmsg_act, err_inf **pp_errinf)
{
  // Save the system error value, because it'll be reset by alloc_reset_err_inf()
  int errno_sys = errno;

  // Prepare err_inf - validate and allocate if needed:
  // - no error info is provided
  if (!pp_errinf)
    return -1;

  // - allocate the memory for the error info instance if it's not done previously
  if (!*pp_errinf && alloc_reset_err_inf(pp_errinf) != 0)
    return -1;

  // Construct the error info
  (*pp_errinf)->num = errnum;
  int nch = snprintf((*pp_errinf)->err_inf_u.msg, LEN_ERRMSG_MAX, 
                     "%s:\n%s\n", errmsg_act, filename);

  // add system error information only if errno_sys is non-zero
  if (errno_sys)
    snprintf((*pp_errinf)->err_inf_u.msg + nch, LEN_ERRMSG_MAX, 
              "System error %i: %s\n", errno_sys, strerror(errno_sys));

  return 0;
}

/* Get an error message based on the file open mode.
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

/* Open a file and optionally return error information.
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
 *              If pp_errinf is NULL, no error info is provided.
 *
 * Return value:
 *  A pointer to a FILE object if successful, or NULL on failure.
 */
FILE *open_file(const t_flname flname, const char *mode, err_inf **pp_errinf)
{
  FILE *hfile = fopen(flname, mode);
  if (hfile == NULL)
    (void)process_error(flname, 60, get_error_message(mode), pp_errinf);
  if (DBG_FLOP && hfile) printf("[open_file] DONE\n");
  return hfile;
}

/* Close the file stream.
 *
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
 *              If pp_errinf is NULL, no error info is provided.
 *
 * Return value:
 *  0       - if the file is successfully closed,
 *  <0 (-1) - on failure. In such cases, the error information is prepared,
 *            and the system error message is stored in pp_errinf.
 */
int close_file(const t_flname flname, FILE *hfile, err_inf **pp_errinf)
{
  int rc = fclose(hfile);
  if (rc != 0)
    (void)process_error(flname, 64, "Failed to close the file", pp_errinf);
  if (DBG_FLOP && !rc) printf("[close_file] DONE\n");
  return rc;
}

/* Reads the content of a file into a specified buffer.
 *
 * This function allocates memory for the file content, reads data from the specified
 * file handle, and handles any errors that may occur during the reading process.
 * If any errors occur during the reading process, it allocates and fills
 * an error information with details about the failure and close the file handle.
 *
 * Parameters:
 *  flname     - the name of the file to read from.
 *  p_flcont   - a pointer to a structure where the file content will be stored.
 *  hfile      - a pointer to the opened file handle from which to read.
 *  pp_errinf - a double pointer to an error info structure to store error information.
 *              If an error occurs, this structure is validated and allocated if necessary,
 *              and the error information (number and message) is saved in it.
 *              If pp_errinf is NULL, no error info is provided.
 *
 * Return value:
 *  0 on success,
 *  1 for the file content allocation failure,
 *  2 for read error,
 *  3 for partial read error.
 */
int read_file(const t_flname flname, t_flcont *p_flcont, FILE *hfile, err_inf **pp_errinf)
{
  // Allocate the memory to store the file content
  if ( alloc_file_cont(p_flcont, get_file_size(hfile)) == NULL ) {
    errno = 0; // reset system error to avoid getting a system error message for this error case
    (void)process_error(flname, 61, "Failed to allocate memory for the content of file", pp_errinf);
    fclose(hfile);
    return 1;
  }

  size_t nch = fread(p_flcont->t_flcont_val, 1, p_flcont->t_flcont_len, hfile);

  // Check if an error has occurred during the reading operation
  if (ferror(hfile)) {
    (void)process_error(flname, 62, "Failed to read from the file", pp_errinf);
    fclose(hfile); // decided not to use close_file() so as not to lose this error message
    return 2;
  }

  // Check a number of items read
  if (nch < p_flcont->t_flcont_len) {
    // TODO: check the error number. But also think, maybe it worths to return system error number?
    (void)process_error(flname, 63, "Partial reading of the file", pp_errinf);
    fclose(hfile); // decided not to use close_file() so as not to lose this error message
    return 3;
  }

  if (DBG_FLOP) printf("[read_file] DONE\n");
  return 0;
}

/* Write content to a file.
 *
 * This function writes data from a file content structure to the specified file handle.
 * If any errors occur during the writing process, it allocates and fills an error
 * information with details about the failure and close the file handle.
 *
 * Parameters:
 *  flname    - the name of the file being written to.
 *  p_flcont  - a pointer to the file content structure containing the data to be written.
 *  hfile     - a file handle (FILE*) where the data will be written.
 *  pp_errinf - a double pointer to an error info structure to store error information.
 *              If an error occurs, this structure is validated and allocated if necessary,
 *              and the error information (number and message) is saved in it.
 *              If pp_errinf is NULL, no error info is provided.
 *
 * Return value:
 *  0 on success,
 *  1 if a write error occurs, 
 *  2 if a partial write occurs,
 * -1 if an error occurs while preparing error information.
 */
int write_file(const t_flname flname, const t_flcont *p_flcont, FILE *hfile, err_inf **pp_errinf)
{
  size_t nch = fwrite(p_flcont->t_flcont_val, 1, p_flcont->t_flcont_len, hfile);
  if (DBG_FLOP) printf("[write_file] 1, writing completed\n");

  // Check if an error has occurred during the writing operation
  if (ferror(hfile)) {
    // TODO: check the error number. But also think, maybe it worths to return system error number?
    (void)process_error(flname, 51, "Failed to write to the file", pp_errinf);
    fclose(hfile); // decided not to use close_file() so as not to lose this error message
    return 1;
  }

  // Check a number of written items 
  if (nch < p_flcont->t_flcont_len) {
    // TODO: check the error number. But also think, maybe it worths to return system error number?
    (void)process_error(flname, 52, "Partial writing to the file", pp_errinf);
    fclose(hfile); // decided not to use close_file() so as not to lose this error message
    return 2;
  }

  if (DBG_FLOP) printf("[write_file] DONE\n");
  return 0;
}
