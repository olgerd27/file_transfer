#ifndef _INTERACT_H_
#define _INTERACT_H_

/*
 * Forward definitions for the types defined in the RPC protocol. The actual types
 * will be available in source file after inclusion the RPC protocol header file.
 */
typedef enum pick_ftype pick_ftype;
typedef struct picked_file picked_file;
typedef struct file_err file_err;

// The function pointer type for the file selection functions
typedef file_err * (*T_pf_select)(picked_file *);

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
char *get_filename_inter(picked_file *p_flpkd, T_pf_select pf_flselect,
                         const char *hostname, char *path_res);

#endif