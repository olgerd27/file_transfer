#ifndef _FS_OPERS_H_
#define _FS_OPERS_H_

#include "../rpcgen/fltr.h"

/*
 * Return a letter representing the file type used in Unix-like OS.
 *
 * This function takes a `mode_t` value, which represents the file mode,
 * and returns a character corresponding to the file type.
 *
 * Parameters:
 *  mode - The mode of the file.
 *
 * Return value:
 *  A character representing the file type:
 *         'd' for directory,
 *         'b' for block device,
 *         'c' for character device,
 *         'p' for FIFO (named pipe),
 *         'l' for symbolic link,
 *         '-' for regular file,
 *         's' for socket,
 *         '?' for unknown type.
 */
enum filetype get_file_type(const char *filepath);

// The types of selected files
enum select_ftype {
  sel_ftype_source, // the 'source' selection file type (regular file should be selected)
  sel_ftype_target  // the 'target' selection file type (non existent file should be selected)
};

// A special error number if an error occurred while resetting the error info.
// Used as workaround when the error info intstance cannot be created but
// you need to return smth pointed out on this issue.
enum { ERRNUM_ERRINF_ERR = -1 };

/*
 * Get the filename interactively by traversing directories.
 *
 * This function allows a user to interactively select a file by navigating through directories.
 * The user is prompted to enter directory names or file names to traverse the file system starting
 * from a given directory.
 *
 * Parameters:
 *  dir_start - a starting directory for the traversal.
 *  path_res  - an allocated char array to store the resulting file path.
 *  sel_ftype - an enum value of type select_ftype indicating whether the file to be selected
 *              is a source or target file.
 *
 * Return value:
 *  Returns path_res on success, and NULL on failure.
 */
char *get_filename_inter(const char *dir_start, char *file_res, enum select_ftype sel_ftype);

#endif