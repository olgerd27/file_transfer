#ifndef _FILE_OPERS_H_
#define _FILE_OPERS_H_

#include <stdio.h>
#include "../rpcgen/fltr.h"

/* Read the file content into a buffer.
 *
 * This function opens the specified file, reads its content into the provided buffer,
 * and then closes the file. It handles any errors that may occur during these steps
 * by setting the appropriate error information in the `err_inf` structure.
 *
 * Parameters:
 *  flname    - the name of the file to be read.
 *  p_flcont  - a pointer to a `t_flcont` structure where the file content will be stored.
 *  pp_errinf - a double pointer to an `err_inf` structure to store error information.
 *              If an error occurs, this structure is validated and allocated if necessary,
 *              and the error information (number and message) is saved in it.
 *              If pp_errinf is NULL, no error info is provided.
 *
 * Return value:
 *  0 on success,
 *  >0 on failure (error code is stored in `(*pp_errinf)->num`).
 */
int read_file_cont(const t_flname flname, t_flcont *p_flcont, 
                   err_inf **pp_errinf);

/* Save file content to a new local file.
 *
 * This function creates a new file with the specified name, writes the provided
 * content into it, and then closes the file. Any errors encountered during these
 * operations are recorded in the `err_inf` structure.
 *
 * Parameters:
 *  flname    - the name of the new file to be created and written to.
 *  p_flcont  - a pointer to a `t_flcont` structure containing the content to be saved.
 *  pp_errinf - a double pointer to an `err_inf` structure for storing error information.
 *              If an error occurs, this structure is validated and allocated if necessary,
 *              and the error information (number and message) is saved in it.
 *              If pp_errinf is NULL, no error info is provided.
 *
 * Return value:
 *  0 on success,
 *  >0 on failure (error code is stored in `(*pp_errinf)->num`).
 */
int save_file_cont(const t_flname flname, const t_flcont *p_flcont, 
                   err_inf **pp_errinf);

#endif