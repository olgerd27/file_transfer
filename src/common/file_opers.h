#ifndef _FILE_OPERS_H_
#define _FILE_OPERS_H_

#include <stdio.h>
#include "../rpcgen/fltr.h"

/* Open a file and optionally return error information.
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
FILE *open_file(const t_flname flname, const char *mode, err_inf **pp_errinf);

/* Close the file stream.
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
int close_file(const t_flname flname, FILE *hfile, err_inf **pp_errinf);

/* Write content to a file.
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
 *  0 on success.
 *  1 if a write error occurs,
 *  2 if a partial write occurs,
 * -1 if an error occurs while preparing error information.
 */
int write_file(const t_flname flname, const t_flcont *p_flcont, FILE *hfile, err_inf **pp_errinf);

/* Reads the content of a file into a specified buffer.
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
int read_file(const t_flname flname, t_flcont *p_flcont, FILE *hfile, err_inf **pp_errinf);

#endif