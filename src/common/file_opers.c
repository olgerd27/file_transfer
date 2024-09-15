#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "file_opers.h"
#include "mem_opers.h"

#define DBG_FLOP 1 // debug the file operations

extern int errno; // global system error number

// TODO: document all the file openning facilities below 
// Helper function to get error message based on mode, with dynamic message for invalid modes
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

// Open the file.
FILE *open_file(const char *const flname, const char *const mode, err_inf *p_errinf)
{
  FILE *hfile = fopen(flname, mode);
  if (hfile == NULL) {
    // reset the error info object and put the error message
    if (reset_err_inf(p_errinf) != 0) exit(1);
    p_errinf->num = 60; // TODO: check the error number
    
    // Print the main error message
    sprintf(p_errinf->err_inf_u.msg,
            "%s:\n%s\n", get_error_message(mode), flname);
    
    // Print the additional system error message if it was occurred
    printf("errno = %d\n", errno);
    if (errno)
      sprintf(p_errinf->err_inf_u.msg + strlen(p_errinf->err_inf_u.msg) - 1,
        "System error %i: %s\n", errno, strerror(errno));
    if (DBG_FLOP) printf("[open_file] failed\n");
  }
  if (DBG_FLOP && hfile) printf("[open_file] DONE\n");
  return hfile;
}
