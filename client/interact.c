#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "interact.h"
#include "../common/mem_opers.h"
#include "../common/fs_opers.h"

#define DBG_INTR 1

extern int errno; // global system error number

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
int select_file(const char *path, file_err *p_flerr, enum select_ftype sel_ftype)
{
  // Reset the error info before file selection
  if (reset_err_inf(&p_flerr->err) != 0) {
    // NOTE: just a workaround - return a special value if an error has occurred 
    // while resetting the error info; but maybe another solution should be used here
    p_flerr->err.num = ERRNUM_ERRINF_ERR;
    return p_flerr->err.num;
  }

  // Reset the file name & type 
  if (reset_file_name_type(&p_flerr->file) != 0) {
    p_flerr->err.num = 81;
    sprintf(p_flerr->err.err_inf_u.msg,
            "Error %i: Failed to reset the file name & type\n", p_flerr->err.num);
    return p_flerr->err.num;
  }

  // Determine the file type
  p_flerr->file.type = get_file_type(path);

  // Process the case of non-existent file required for the target file.
  // Should be processing before conversion path to the absolute one.
  if (p_flerr->file.type == FTYPE_NEX) {
    // Copy the selected file name into the file info instance
    // TODO: delete a line with strncpy() after final acception of the line with copy_path()
    // strncpy(p_flerr->file.name, path, strlen(path) + 1);
    copy_path(path, p_flerr->file.name);

    // Check the correctness of the current file selection based on the selection file type
    if (sel_ftype == sel_ftype_target)
      ; // OK: the non-existent file type was selected as expected for a 'target' selection file type
    else if (sel_ftype == sel_ftype_source) {
      // Fail: the source file selection (a regular file) was expected, but the target file selection
      // (a non-existent file) was actually attempted -> produce the error
      p_flerr->err.num = 82;
      sprintf(p_flerr->err.err_inf_u.msg,
              "Error %i: The selected file does not exist:\n'%s'\n"
              "Only the regular file can be selected as the source file.\n",
              p_flerr->err.num, p_flerr->file.name);
    }
    return p_flerr->err.num;
  }

  // Convert the passed path into the full (absolute) path - needed to any type of existent file
  char *errmsg = NULL;
  if (!rel_to_full_path(path, p_flerr->file.name, &errmsg)) {
    p_flerr->err.num = 80;
    sprintf(p_flerr->err.err_inf_u.msg, "Error %i: %s\n", p_flerr->err.num, errmsg);
    free(errmsg); // free allocated memory
    return p_flerr->err.num;
  }

  // Process the cases of existent file
  switch (p_flerr->file.type) {
    case FTYPE_DIR: /* directory */
      // Get the directory content and save it to file_err instance
      // If error has occurred it sets to p_flerr, no need to check the RC
      ls_dir_str(p_flerr);
      break;

    case FTYPE_REG: /* regular file */
      // Check the correctness of the current file selection based on the selection file type
      if (sel_ftype == sel_ftype_source)
        ; // OK: the regular file type was selected as expected for a 'source' selection file type
      else if (sel_ftype == sel_ftype_target) {
        // Fail: the target file selection (a non-existent file) was expected, but the source file selection
        // (a regular file) was actually attempted -> produce the error
        p_flerr->err.num = 83;
        sprintf(p_flerr->err.err_inf_u.msg,
                "Error %i: The wrong file type was selected - regular file:\n'%s'\n"
                "Only the non-existent file can be selected as the target file.\n",
                p_flerr->err.num, p_flerr->file.name);
      }
      break;

    case FTYPE_OTH: /* any other file type like link, socket, etc. */
      p_flerr->err.num = 84;
      sprintf(p_flerr->err.err_inf_u.msg,
              "Error %i: Unsupported file type was selected (other):\n'%s'\n",
              p_flerr->err.num, p_flerr->file.name);
      break;

    case FTYPE_INV: /* invalid file */
      // NOTE: If the rel_to_full_path() call above fails, this case will never be reached.
      p_flerr->err.num = 85;
      sprintf(p_flerr->err.err_inf_u.msg,
              "Error %i: Invalid file was selected:\n'%s'\n%s\n",
              p_flerr->err.num, p_flerr->file.name, strerror(errno));
      break;
  }
  return p_flerr->err.num;
}

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
 *
 * The function performs the following steps:
 * 1. Initializes the traversal starting at dir_start or, in case of error, on '/'.
 * 2. Repeatedly prompts the user to select files or directories, updating the current path.
 * 3. If a regular file is selected, it copies the full path to path_res and returns it.
 * 4. Handles errors in file selection and reverts to the previous valid path if necessary.
 *
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
char *get_filename_inter(const char *dir_start, char *path_res, enum select_ftype sel_ftype)
{
  char path_curr[LEN_PATH_MAX]; // current path used to walk through the directories and construct path_res
  char path_prev[LEN_PATH_MAX]; // a copy of the previous path to restore it if necessary
  char fname_inp[LEN_PATH_MAX]; // inputted filename (LEN_PATH_MAX is used, as filename can be an absolute path)
  char *pfname_inp; // pointer to the inputted filename; used to differentiate between relative and absolute paths
  int offset; // offset from the beginning of the current path for the added subdir/file
  int nwrt_fname; // number of characters written to the current path in each iteration
  file_err flerr; // a local struct instance to store & pass the information about a file & error

  // Initialization
  // Init the previous path with a root dir as a guaranteed valid path
  copy_path("/", path_prev);

  // Init the current path (path_curr) in a way of copying dir_start
  // offset = copy_path(dir_start, path_curr); // init the current path with the passed start dir for traversal

  // Init the current path (path_curr) in a way of converting the dir_start to the full (absolute) path
  char *errmsg = NULL;
  if (!rel_to_full_path(dir_start, path_curr, &errmsg)) {
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
    printf("[get_filename_inter] 1, path_curr: '%s'\noffset: %d\n", path_curr, offset);

  // Main loop
  while (1) {
    // TODO: replace select_file() with a function pointer that can be either
    // select_file() or pick_entity() RPC function
    if (select_file(path_curr, &flerr, sel_ftype) == 0) {
      // Successful file selection (regular or non-existent file type)
      if (flerr.file.type == FTYPE_REG || flerr.file.type == FTYPE_NEX) {
        copy_path(flerr.file.name, path_res);
        return path_res;
      }
    }
    else {
      // An error occurred while selecting a file 
      fprintf(stderr, "%s", 
              flerr.err.num != ERRNUM_ERRINF_ERR 
              ? flerr.err.err_inf_u.msg /* normal error occurred */
              : "Failed to reset the error information"); /* error occurred while resetting the error info (workaround) */
      offset = copy_path(path_prev, path_curr); // restore the previous valid path
      continue; // start loop from the beginning to select the previous valid path
    }
    
    /* At this point we're sure that it's selected a valid directory */

    // Update the current path with the absolute path from flerr.file.name to make 
    // a clearer path without "." or ".."
    offset = copy_path(flerr.file.name, path_curr);

    // Print the full (absolute) path and content of the current directory
    printf("\n%s:\n%s\n", flerr.file.name, flerr.file.cont.t_flcont_val);

    // Print the prompt for user input
    printf("Select the %s file on %s:\n" 
           , (sel_ftype == sel_ftype_source ? "Source" : "Target")
           , "localhost"); // TODO: specify the actual hostname here
    
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
      printf("[get_filename_inter] 3, path_curr + offset(%i): '%s'\nfname_inp: '%s', pfname_inp: '%s'\n",
             offset, path_curr + offset - 2, fname_inp, pfname_inp);

    // Construct the full path of the selected file
    nwrt_fname = construct_full_path(pfname_inp, LEN_PATH_MAX - offset, path_curr + offset);
    if (nwrt_fname <= 0) return NULL;
    
    // Increment the offset by the number of chars written to path_curr (after completed checks)
    offset += nwrt_fname;

    if (DBG_INTR)
      printf("[get_filename_inter] 4, path_curr: '%s'\nnwrt_fname: %d,  offset: %d\n", 
             path_curr, nwrt_fname, offset);
  }

  // TODO: maybe need to free the file name & content memory - free_file_inf(), at least
  // if file selection performs on client.
  // TODO: think what to do if file selection performs on server.
  return NULL;
}
