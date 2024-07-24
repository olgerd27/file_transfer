#ifndef _INTERACT_H_
#define _INTERACT_H_

// The types of selected files
enum select_ftype {
  sel_ftype_source, // the 'source' selection file type (regular file should be selected)
  sel_ftype_target  // the 'target' selection file type (non existent file should be selected)
};

typedef struct file_err file_err;

/*
 * Select a file: determine its type and get its full (absolute) path.
 *
 * This function determines the type of the specified file and converts its
 * relative path to an absolute path. It resets the error info before performing
 * these operations. The results, including any errors, are stored in the provided
 * file_err structure.
 *
 * Parameters:
 *  path      - a path of the file that needs to be selected.
 *  p_flerr   - a pointer to the file_err RPC struct instance to store file & error info.
 *              This struct is used to set and return the result through the function argument.
 *  sel_ftype - an enum value of type select_ftype indicating whether the file to be selected
 *              is a source or target file.
 *
 * Return value:
 *  RC: 0 on success, >0 on failure.
 */
int select_file(const char *path, file_err *p_flerr, enum select_ftype sel_ftype);

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