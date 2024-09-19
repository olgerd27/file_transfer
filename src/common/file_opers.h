#ifndef _FILE_OPERS_H_
#define _FILE_OPERS_H_

#include <stdio.h>

// Forward definitions for the types defined in the RPC protocol.
typedef struct err_inf err_inf;

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
 *              If NULL, no error info is provided. If uninitialized, memory is allocated.
 *
 * Return value:
 *  A pointer to a FILE object if successful, or NULL on failure.
 */
FILE *open_file(const char *const flname, const char *const mode, err_inf **pp_errinf);

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
int close_file(const char *const flname, FILE *hfile, err_inf **pp_errinf);

#endif