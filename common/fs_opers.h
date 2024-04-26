#ifndef _FS_OPERS_H_
#define _FS_OPERS_H_

#include "../rpcgen/fltr.h"

// Return the filetype (defined in fltr.h) for the specified file
enum filetype file_type(const char *file);

/*
 * Get the filename in the interactive mode.
 * - dir_start -> a starting directory for the traversal
 * - file_res  -> a result file path+name to put the result, memory should be allocated outside
 * RC: returns file_res on success, and NULL on failure.
 */
char * get_filename_inter(const char *dir_start, char *file_res);

#endif
