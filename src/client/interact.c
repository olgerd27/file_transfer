#include <stdio.h>
#include <string.h>

#include "interact.h"
#include "../common/mem_opers.h"
#include "../common/fs_opers.h"
#include "../common/logging.h"

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
static int construct_full_path(char *path_new, size_t lenmax, char *path_full)
{
  int nwrt = snprintf(path_full, lenmax, "/%s", path_new);

  // Check if nothing was written to a full path
  if (nwrt <= 0) {
    LOG(LOG_TYPE_INTR, LOG_LEVEL_ERROR, "Invalid filename: '%s'", path_new);
    fprintf(stderr, "Invalid filename: '%s'\n", path_new);
    return -1;
  }

  // Check if the whole path_new was added to path_full.
  // '+1' was added, as '/' was also written to path_full
  if (nwrt != strlen(path_new) + 1) {
    LOG(LOG_TYPE_INTR, LOG_LEVEL_ERROR, "Cannot append the inputted filename to the result filename");
    fprintf(stderr, "Cannot append the inputted filename to the result filename\n");
    return -2;
  }

  return nwrt;
}

// Return a name of the picked (selected) file type
const char *get_pkd_ftype_name(pick_ftype pk_fltype)
{
  switch (pk_fltype) {
    case pk_ftype_source: return "Source";
    case pk_ftype_target: return "Target";
    default:              return "Invalid pick file type";
  }
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
 * 1 List of the passed dir (initially "." or "/")
 * 2 Get user input
 * 3 Add user input to the current path
 * 4 Check the input:
 *    4.1 If it's a dir, go to p.1.
 *    4.2 If it's not a dir, do the following:
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
 *
 * Chain of calls to run the select_file() on the CLIENT:
 * prg_clnt.c:main()
 * prg_clnt.c:do_RPC_action()
 * prg_clnt.c:interact() # p_func points to fs_opers.c:select_file(); pass p_func to get_filename_inter()
 * interact.c:get_filename_inter() # it calls p_func in a loop
 * fs_opers.c:select_file()
 * 
 * Chain of calls to run the select_file() on the SERVER:
 * prg_clnt.c:main()
 * prg_clnt.c:do_RPC_action()
 * prg_clnt.c:interact() # p_func points to prg_clnt.c:file_pick_rmt(); pass p_func to get_filename_inter()
 * interact.c:get_filename_inter() # it calls p_func in a loop
 * prg_clnt.c:file_pick_rmt() # a wrapper for prg_clnt.c:pick_file_1()
 * prg_clnt.c:pick_file_1() # RPC function on client
 * prg_serv.c:pick_file_1_svc() # RPC function on server
 * fs_opers.c:select_file()
 */
char *get_filename_inter(const picked_file *p_flpkd, T_pf_select pf_flselect, 
                         const char *hostname, char *path_res)
{
  LOG(LOG_TYPE_INTR, LOG_LEVEL_DEBUG, "Begin. Request to get %s filename on %s", 
      get_pkd_ftype_name(p_flpkd->pftype), hostname);
  char path_curr[LEN_PATH_MAX]; // current path used to walk through the directories and construct path_res
  char path_prev[LEN_PATH_MAX]; // a copy of the previous path to restore it if necessary
  char fname_inp[LEN_PATH_MAX]; // inputted filename (LEN_PATH_MAX is used, as filename can be an absolute path)
  char *pfname_inp; // pointer to the inputted filename; used to differentiate between relative and absolute paths
  int offset; // offset from the beginning of the current path for the added subdir/file
  int nwrt_fname; // number of characters written to the current path in each iteration
  picked_file flpkd_curr = *p_flpkd; // current picked file object that will be sent to a selection function
  file_err *p_flerr; // pointer to the file & error info struct

  // Initialization
  // Init the previous path with a root dir as a guaranteed valid path on Unix-like OS
  copy_path("/", path_prev);

  // Init the current path (path_curr) in a way of copying the passed start dir for traversal
  offset = copy_path(flpkd_curr.name, path_curr);
  LOG(LOG_TYPE_INTR, LOG_LEVEL_DEBUG, "offset: %d, path_curr: %s", offset, path_curr);

  // Main loop
  while (1) {
    // Call the file selection function via its pointer for either local or remote file selection
    flpkd_curr.name = path_curr; // set the current path to the file name that should be picked
    p_flerr = (*pf_flselect)(&flpkd_curr); // make a function pointer call
    LOG(LOG_TYPE_INTR, LOG_LEVEL_DEBUG, "file selection has done, p_flerr: %p", (void *)p_flerr);
    if (p_flerr->err.num == 0) {
      if (p_flerr->file.type == FTYPE_REG || p_flerr->file.type == FTYPE_NEX) {
        // Successful file selection, it's a regular or non-existent file type.
        // Copy the result path before freeing the memory of file_err object
        copy_path(p_flerr->file.name, path_res);
        xdr_free((xdrproc_t)xdr_file_err, p_flerr); // free the file & error info
        LOG(LOG_TYPE_INTR, LOG_LEVEL_INFO, "Successful selection of file:\n  %s", path_res);
        LOG(LOG_TYPE_INTR, LOG_LEVEL_DEBUG, "Done.");
        return path_res;
      }
    }
    else {
      // An error occurred while selecting a file.
      // Check if a real file type was defined. If no - that means it's a fatal 
      // error case, and this error is not related to the failed file selection error, 
      // so we just need to leave this function.
      if (p_flerr->file.type == FTYPE_DFL) {
        fprintf(stderr, "Fatal error: file has not been selected. "
                        "More info should be provided if loggin level set to LOG_LEVEL_DEBUG\n");
        break;
      }
      // If running reaches this point, it does mean it's indeed a file selection error 
      // and we can provide the user with a new attempt to select a file
      LOG(LOG_TYPE_INTR, LOG_LEVEL_ERROR, "%s", p_flerr->err.err_inf_u.msg);
      offset = copy_path(path_prev, path_curr); // restore the previous valid path
      continue; // start loop from the beginning to select the previous valid path
    }

    /* At this point, we're sure that a valid directory has been selected. */

    // Update the current path with the absolute path from p_flerr->file.name to make 
    // a clear path without "." or ".."
    offset = copy_path(p_flerr->file.name, path_curr);

    // Print the full (absolute) path and content of the current directory
    printf("\n%s:\n%s\n", p_flerr->file.name, p_flerr->file.cont.t_flcont_val);

    // Print the prompt for user input
    printf("Select the %s file on %s:\n", get_pkd_ftype_name(flpkd_curr.pftype), hostname);

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

    LOG(LOG_TYPE_INTR, LOG_LEVEL_DEBUG,
        "path_curr + offset(%i): '%s'\n  fname_inp: '%s', pfname_inp: '%s'",
        offset, path_curr + offset - 2, fname_inp, pfname_inp);

    // Construct the full path of the selected file
    nwrt_fname = construct_full_path(pfname_inp, LEN_PATH_MAX - offset, path_curr + offset);
    if (nwrt_fname <= 0) return NULL;
    
    // Increment the offset by the number of chars written to path_curr (after completed checks)
    offset += nwrt_fname;

    LOG(LOG_TYPE_INTR, LOG_LEVEL_DEBUG,
        "nwrt_fname: %d, offset: %d, path_curr:\n  %s", nwrt_fname, offset, path_curr);
    xdr_free((xdrproc_t)xdr_file_err, p_flerr); // free the file & error info
    LOG(LOG_TYPE_INTR, LOG_LEVEL_DEBUG, "file_error object freed, ptr=%p", (void*)p_flerr);
  }
  LOG(LOG_TYPE_INTR, LOG_LEVEL_ERROR, "Unsuccessful end.\n");
  return NULL;
}
