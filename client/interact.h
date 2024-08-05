#ifndef _INTERACT_H_
#define _INTERACT_H_

// Forward definition for the type defined in the RPC protocol. The actual type
// will be available in source file after inclusion the RPC protocol header file
typedef enum pick_ftype pick_ftype;

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
 *  pftype    - an enum value of type pick_ftype indicating whether the file to be selected
 *              is a source or target file.
 *
 * Return value:
 *  Returns path_res on success, and NULL on failure.
 */
char *get_filename_inter(char *dir_start, char *file_res, enum pick_ftype pftype);

#endif