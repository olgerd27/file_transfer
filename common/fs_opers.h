#ifndef _FS_OPERS_H_
#define _FS_OPERS_H_

#include "../rpcgen/fltr.h"

// Return the filetype (defined in fltr.h) for the specified file
enum filetype file_type(const char *file);

// List the directory content.
// RC: 0 - success, >0 - failure
int ls_dir(const char *dirname);

// Get the filename in the interactive mode
char * get_filename_inter(const char *dir_start);

#endif
