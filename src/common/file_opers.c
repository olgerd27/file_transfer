#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <errno.h>

#include "file_opers.h"
#include "mem_opers.h"

#define DBG_FLOP 1 // debug the file operations

extern int errno; // global system error number

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
 * Allocate and reset the error information structure.
 *
 * This function allocates memory for an error information structure and resets its
 * contents to prepare it for storing error data. If memory allocation fails, it
 * prints an error message and returns a failure code.
 *
 * Parameters:
 *  p_errinf - A double pointer to an error info structure to allocate and reset.
 *
 * Return value:
 *  0 on success, >0 on failure. (e.g., memory allocation error).
 */
static int alloc_reset_err_inf(err_inf **p_errinf) {
    // Allocate memory if needed
    *p_errinf = (err_inf*)malloc(sizeof(err_inf));
    if (!*p_errinf) {
      fprintf(stderr, "Memory allocation failed for the error info instance\n");
      return 1;
    }
    
    // Allocate or reset the error message buffer
    (*p_errinf)->err_inf_u.msg = NULL; // needed for a correct work of reset_err_inf()
    if (reset_err_inf(*p_errinf) != 0) { // allocate memory for error message buffer
      fprintf(stderr, "Cannot reset the memory for the error info instance\n");
      return 2;
    }

    return 0;
}

/*
 * Open a file and optionally return error information.
 *
 * This function attempts to open a file in the specified mode. If the file cannot
 * be opened, and the error info pointer is provided, it allocates an error structure
 * and stores detailed error information. If the error info pointer is NULL, no error
 * details are returned. If a valid pointer is passed but uninitialized, the function
 * allocates and initializes it.
 *
 * Note: The caller is responsible for freeing the error information structure if
 * the value passed was NULL and memory was allocated by this function. If a pointer
 * to a static or automatic instance of err_inf is passed, only the contents are modified, 
 * not the pointer.
 *
 * Parameters:
 *  flname   - The name of the file to open.
 *  mode     - The mode in which to open the file (e.g., "r", "w").
 *  p_errinf - A double pointer to an error info structure to store error information.
 *             If NULL, no error info is provided. If uninitialized, memory is allocated.
 *
 * Return value:
 *  A pointer to a FILE object if successful, or NULL on failure.
 */
FILE *open_file(const char *const flname, const char *const mode, err_inf **p_errinf)
{
  FILE *hfile = fopen(flname, mode);
  if (hfile == NULL) {
    int errno_save = errno; // save errno before error info reset that can reset it

    // no error info is provided
    if (p_errinf == NULL) return NULL;

    // allocate the memory for the error info instance
    if ( *p_errinf == NULL && alloc_reset_err_inf(p_errinf) != 0 )
      return NULL;

    // Save the error info
    (*p_errinf)->num = 60; // TODO: check the error number, or maybe use system error number?
    
    // print the error message + system error
    snprintf((*p_errinf)->err_inf_u.msg, LEN_ERRMSG_MAX,
            "%s:\n%s\nSystem error %i: %s\n",
            get_error_message(mode), flname, errno_save, strerror(errno_save));
  }
  if (DBG_FLOP && hfile) printf("[open_file] DONE\n");
  return hfile;
}
