#ifndef _FS_OPERS_H_
#define _FS_OPERS_H_

#include "../rpcgen/fltr.h"

/*
 * Return the filetype (defined in fltr.h) for the specified file.
 * - filepath - a full path to the file.
 * RC: returns a filetype enum value defined in the RPC fltr.x file.
 */
enum filetype file_type(const char *filepath);

/*
 * Get the filename in the interactive mode.
 * - dir_start -> a starting directory for the traversal
 * - path_res  -> an allocated char array to store the resulting file path
 * RC: returns file_res on success, and NULL on failure.
 */
char * get_filename_inter(const char *dir_start, char *file_res);

#endif
