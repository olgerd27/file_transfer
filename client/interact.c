#include <stdio.h>
#include <string.h>

#include "interact.h"
#include "../common/mem_opers.h"
#include "../common/fs_opers.h"

#define DBG_INTR 1

/*
 * Get user input for a filename.
 *
 * This function prompts the user to enter a filename. If the user inputs nothing
 * (just presses ENTER), an error is returned. It uses `fgets` to read the input
 * and ensures the resulting string is properly null-terminated by removing any
 * trailing newline character.
 *
 * Parameters:
 *  filename - A pointer to an allocated string array where the input will be stored.
 *             The array should be at least `NAME_MAX` characters long.
 * 
 * Return value:
 *  An integer indicating the result of the operation:
 *    0 on success,
 *    1 if a read error occurs,
 *    2 if the input is empty (user just pressed ENTER).
 */
static int input_filename(char *filename)
{
  printf(">>> ");
  if (fgets(filename, NAME_MAX, stdin) == NULL) {
    fprintf(stderr, "Read error occurred. Please make the input again\n");
    return 1;
  }

  // Check if input is empty (user just pressed ENTER)
  if (*filename == '\n') return 2;

  // Remove a trailing newline character remained from fgets() input
  filename[strcspn(filename, "\n")] = '\0';

  return 0;
}

/*
 * Constructs a full path by appending a new path segment to the path delimeter '/'.
 *
 * This function appends the provided new path segment (path_new) to the path delimeter '/',
 * storing the resulting full path in path_full.
 * 
 * Parameters:
 *  path_new  - a pointer to a character array containing the new path segment.
 *  lenmax    - the maximum length of the resulting path (`path_full`), including the null terminator.
 *  path_full - a pointer to a character array where the resulting full path will be stored.
 * 
 * Return value:
 *  The number of characters written to path_full on success (excluding the null terminator).
 *  >0 on success, <=0 on failure.
 */
int construct_full_path(char *path_new, size_t lenmax, char *path_full)
{
  int nwrt = snprintf(path_full, lenmax, "/%s", path_new);

  // Check if nothing was written to a full path
  if (nwrt <= 0) {
    fprintf(stderr, "Invalid filename: '%s'\n", path_new);
    return -1;
  }

  // Check if the whole path_new was added to path_full.
  // '+1' was added, as '/' was also written to path_full
  if (nwrt != strlen(path_new) + 1) {
    fprintf(stderr, "Cannot append the inputted filename to the result filename\n");
    return -2;
  }

  return nwrt;
}

// TODO: update the documentation
/*
 * Get the filename interactively by traversing directories.
 *
 * This function allows a user to interactively select a file by navigating through directories.
 * The user is prompted to enter directory names or file names to traverse the file system starting
 * from a given directory.
 *
 * Parameters:
 *  dir_start - a starting directory for the traversal.
 *  pftype    - an enum value of type pick_ftype indicating whether the file to be selected
 *              is a source or target file.
 *  path_res  - an allocated char array to store the resulting file path.
 *
 * Return value:
 *  Returns path_res on success, and NULL on failure.
 *
 * The function performs the following steps:
 * 1. Initializes the traversal starting at p_flpkd->name or, in case of error, on '/'.
 * 2. Repeatedly prompts the user to select files or directories, updating the current path.
 * 3. If a regular file is selected, it copies the full path to path_res and returns it.
 * 4. Handles errors in file selection and reverts to the previous valid path if necessary.
 *
 * // TODO: update this procedure to reflect the current way of working the interactive operation
 * General procedure:
 * 1. list of the passed dir (initially "." or "/")
 * 2. get user input
 * 3. add user input to the current path
 * 3. check the input:
 *    3.1 if it's a dir, go to p.1.
 *    3.2 if it's not a dir, do the following:
 *        >> for source file:
 *           - if it's a regular file - OK and return its full path
 *           - if it's NOT a regular file - error and message like:
 *             Please select a regular file.
 *           - if it's NOT exist - error and message like:
 *             File ... doesn't exist. Please select an existing source file.
 *        >> for target file:
 *           - if it's NOT exist - OK and return its full path
 *           - if it exists (type doesn't matter) - error and message like:
 *             File ... exists. Please specify non-existing target file.
 */
char *get_filename_inter(picked_file *p_flpkd, T_pf_select pf_flselect, 
                         const char *hostname, char *path_res)
{
  char path_curr[LEN_PATH_MAX]; // current path used to walk through the directories and construct path_res
  char path_prev[LEN_PATH_MAX]; // a copy of the previous path to restore it if necessary
  char fname_inp[LEN_PATH_MAX]; // inputted filename (LEN_PATH_MAX is used, as filename can be an absolute path)
  char *pfname_inp; // pointer to the inputted filename; used to differentiate between relative and absolute paths
  int offset; // offset from the beginning of the current path for the added subdir/file
  int nwrt_fname; // number of characters written to the current path in each iteration
  file_err *p_flerr; // pointer to the struct that stores the file & error info

  // Initialization
  // Init the previous path with a root dir as a guaranteed valid path on Unix-like OS
  copy_path("/", path_prev);

  // Init the current path (path_curr) in a way of copying p_flpkd->name
  // TODO: delete this method of init if the presented below method will be eventially choosed
  // offset = copy_path(p_flpkd->name, path_curr); // init the current path with the passed start dir for traversal

  // Init the current path (path_curr) in a way of converting the p_flpkd->name
  // to the full (absolute) path
  // TODO: it looks it's not correct to do it here, since a conversion to the abs path
  // should occur in select_file() only that can be executed either on client or server side.
  char *errmsg = NULL;
  if (!rel_to_full_path(p_flpkd->name, path_curr, &errmsg)) {
    fprintf(stderr, "%s\nChange the current directory to the default one: '%s'\n", 
            errmsg, path_prev);
    free(errmsg); // free allocated memory
    // The 2nd attempt: set the default previous path for the current path
    if (!rel_to_full_path(path_prev, path_curr, &errmsg)) {
      fprintf(stderr, "Fatal error: %s\n", errmsg);
      free(errmsg); // free allocated memory
      return NULL;
    }
  }

  // Init the offset as a length of the current path
  offset = strlen(path_curr);

  if (DBG_INTR)
    printf("[get_filename_inter] 1, offset: %d, path_curr:\n  '%s'\n", offset, path_curr);

  // Main loop
  while (1) {
    // Call the file selection function via its pointer for either local or remote file selection
    p_flpkd->name = path_curr; // set the current path to the file name that should be picked
    p_flerr = (*pf_flselect)(p_flpkd); // function pointer call
    if (p_flerr->err.num == 0) {
      if (p_flerr->file.type == FTYPE_REG || p_flerr->file.type == FTYPE_NEX) {
        // Successful file selection, it's a regular or non-existent file type
        copy_path(p_flerr->file.name, path_res);
        return path_res;
      }
    }
    else {
      // An error occurred while selecting a file 
      fprintf(stderr, "%s", 
              p_flerr->err.num != ERRNUM_ERRINF_ERR 
              ? p_flerr->err.err_inf_u.msg /* normal error occurred */
              : "Failed to reset the error information"); /* error occurred while resetting the error info (workaround) */
      offset = copy_path(path_prev, path_curr); // restore the previous valid path
      continue; // start loop from the beginning to select the previous valid path
    }

    /* At this point, we're sure that a valid directory has been selected. */

    // Update the current path with the absolute path from p_flerr->file.name to make 
    // a clearer path without "." or ".."
    offset = copy_path(p_flerr->file.name, path_curr);

    // Print the full (absolute) path and content of the current directory
    printf("\n%s:\n%s\n", p_flerr->file.name, p_flerr->file.cont.t_flcont_val);

    // Print the prompt for user input
    printf("Select the %s file on %s:\n",
           (p_flpkd->pftype == pk_ftype_source ? "Source" : "Target"),
           hostname);
    
    // Get user input of filename
    if (input_filename(fname_inp) != 0)
      continue;

    // Before changing the current path, save it as the previous valid path
    copy_path(path_curr, path_prev);

    // Settings for an absolute (full) and relative path
    if (*fname_inp == '/') {
      // User inputted a full (absolute) path
      memset(path_curr, 0, offset); // reset the current path
      offset = 0; // inputted path should be set at the beginning of the current path
      pfname_inp = fname_inp + 1; // skip the 1st '/', as it will be added to the full path 
                                  // later in construct_full_path()
    }
    else {
      // User inputted a relative path (subpath)
      pfname_inp = fname_inp; // points at the beginning of the inputted filename
    }

    if (DBG_INTR)
      printf("[get_filename_inter] 2, path_curr + offset(%i): '%s'\n  fname_inp: '%s', pfname_inp: '%s'\n",
             offset, path_curr + offset - 2, fname_inp, pfname_inp);

    // Construct the full path of the selected file
    nwrt_fname = construct_full_path(pfname_inp, LEN_PATH_MAX - offset, path_curr + offset);
    if (nwrt_fname <= 0) return NULL;
    
    // Increment the offset by the number of chars written to path_curr (after completed checks)
    offset += nwrt_fname;

    if (DBG_INTR)
      printf("[get_filename_inter] 3, nwrt_fname: %d, offset: %d, path_curr:\n  '%s'\n",
             nwrt_fname, offset, path_curr);
  }

  // TODO: maybe need to free the file name & content memory - free_file_inf(), at least
  // if file selection performs on client. The file_err object is created in a static 
  // memory by the select_file() function, so it's incorrect to free it for the client. 
  // TODO: think what to do if file selection performs on server.
  return NULL;
}
