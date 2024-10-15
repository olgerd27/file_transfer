#ifndef _INTERACT_H_
#define _INTERACT_H_

/*
 * Forward definitions for the types defined in the RPC protocol. The actual types
 * will be available in source file after inclusion the RPC protocol header file.
 */
typedef enum pick_ftype pick_ftype;
typedef struct picked_file picked_file;
typedef struct file_err file_err;

// Return a name of the picked (selected) file type
const char *get_pkd_ftype_name(pick_ftype pk_fltype);

// The function pointer type for the file selection functions
typedef file_err * (*T_pf_select)(picked_file *);

/* Get the filename interactively by traversing directories.
 *
 * This function allows a user to interactively select a file by navigating through directories.
 * The user is prompted to enter directory names or file names to traverse the file system starting
 * from a given directory. The selected file's full path is returned.
 *
 * Parameters:
 *  p_flpkd    - a pointer to the `picked_file` structure containing the initial
 *               directory path and file selection type (source/target).
 *  pf_flselect - a function pointer to the file selection function (local or remote).
 *  hostname   - a string representing the hostname where the file selection is taking place.
 *  path_res   - an allocated character array to store the full path of the selected file.
 *
 * Return value:
 *  Returns `path_res` (full path) on successful file selection, and `NULL` on failure.
 *
 * The function performs the following steps:
 * 1. Initializes the traversal starting at p_flpkd->name or '/' in case of an error.
 * 2. Repeatedly prompts the user to select files or directories, updating the current path.
 * 3. If a regular or non-existent file is selected, the full path is copied to `path_res`.
 * 4. In case of an error, the previous valid path is restored, and the user is prompted to retry.
 * 5. Handles errors in file selection.
 */
char *get_filename_inter(const picked_file *p_flpkd, T_pf_select pf_flselect,
                         const char *hostname, char *path_res);

#endif