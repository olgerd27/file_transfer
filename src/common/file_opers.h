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
FILE *open_file(const char *const flname, const char *const mode, err_inf **p_errinf);

#endif