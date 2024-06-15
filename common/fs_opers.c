#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <stdint.h>
#include <errno.h>

#include "fs_opers.h"

// Global system variable that store the error number
extern int errno;
static char filename_src[LEN_PATH_MAX]; // DELETE: it's just for testing

// Return a letter of a file type used in Unix-like OS
static char get_file_type_unix(mode_t mode)
{
  if (S_ISDIR(mode)) return 'd';
  if (S_ISBLK(mode)) return 'b';
  if (S_ISCHR(mode)) return 'c';
  if (S_ISFIFO(mode)) return 'p';
  if (S_ISLNK(mode)) return 'l';
  if (S_ISREG(mode)) return '-';
  if (S_ISSOCK(mode)) return 's';
  return '?';
}

// Convert permissions from the digit to the symbol form.
// The function adds a file type at the 1st place, like same as 'ls -l' command does.
static char * str_perm(mode_t mode, char *strmode)
{
  strmode[0] = get_file_type_unix(mode);
  strmode[1] = (mode & S_IRUSR) ? 'r' : '-';
  strmode[2] = (mode & S_IWUSR) ? 'w' : '-';
  strmode[3] = (mode & S_IXUSR) ? 'x' : '-';
  strmode[4] = (mode & S_IRGRP) ? 'r' : '-';
  strmode[5] = (mode & S_IWGRP) ? 'w' : '-';
  strmode[6] = (mode & S_IXGRP) ? 'x' : '-';
  strmode[7] = (mode & S_IROTH) ? 'r' : '-';
  strmode[8] = (mode & S_IWOTH) ? 'w' : '-';
  strmode[9] = (mode & S_IXOTH) ? 'x' : '-';
  strmode[10] = '\0';
  return strmode;
}

/*
 * Return the filetype (defined in fltr.h) for the specified file.
 * - filepath - a full path to the file.
 * RC: returns a filetype enum value defined in the RPC fltr.x file.
 */
enum filetype file_type(const char *filepath)
{
//  printf("[file_type] 0, filepath: '%s'\n", filepath);
  enum filetype ftype;
  struct stat statbuf;

  // Check if stat() returns an error due to file non-existence, return FTYPE_NEX; 
  // otherwise, return an invalid file type - FTYPE_INV.
  if (stat(filepath, &statbuf) == -1)
    return errno == ENOENT ? FTYPE_NEX : FTYPE_INV;

  // Determine the file type
  switch( get_file_type_unix(statbuf.st_mode) ) {
    case 'd':
      ftype = FTYPE_DIR;
      break;
    case '-':
      ftype = FTYPE_REG;
      break;
    default:
      ftype = FTYPE_OTH;
      break;
  }
//  printf("[file_type] $, filetype: %d\n", (int)ftype);
  return ftype;
}

/*
 * Get user input of filename. 
 * If nothing was inputted (user just pressed ENTER), an error is returned.
 * - filename -> the allocated string array to put the input
 * RC: 0 on success, >0 on failure.
 */
static int input_filename(char *filename)
{
  printf("\n>>> ");
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

// Copy the path_src to path_trg with the length of LEN_PATH_MAX.
// RC: returns the number of the written characters.
int copy_path(char *path_src, char *path_trg)
{
  return snprintf(path_trg, LEN_PATH_MAX, "%s", path_src);
}

/*
 * NEW VERSIONS, after their acceptance delete old version: ls_dir()
 */
/*
 * NEW version of the memory manipulation functions, initialy defined in prg_serv.c and prg_clnt.c.
 */
// TODO: move these versions to the new files like common/memops.c, and use them everywhere where they're required.
/*
 * Allocate the memory to store the file content.
 * Allocate only if it has not already done, else - return a pointer to the existing memory.
 * - p_flcont: pointer to a file content object to store the allocated memory
 * - size: the size of the memory to allocate
 * - p_err: pointer to an error info object, where the error info will be written in case of error.
 * RC: returns a pointer to the allocated memory or NULL in case of error.
 */
// TODO: check if the error number is ok in this function; this function should be used here and in both client and server code.
char * alloc_file_cont_NEW(t_flcont *p_flcont, unsigned size)
{
  // Check if the file content object is allocated
  if (!p_flcont) {
    fprintf(stderr, "Error 6: Failed to allocate memory for the file content. p_flcont=%p\n", (void*)p_flcont);
    return NULL;
  }

  // Memory allocation 
  if (!p_flcont->t_flcont_val) {
    p_flcont->t_flcont_len = size;
    p_flcont->t_flcont_val = (char*)malloc(size);
    if (!p_flcont->t_flcont_val) {
      fprintf(stderr, "Error 7: Failed to allocate memory for the file content, ptr to file cont val: %p\n", 
              (void*)p_flcont->t_flcont_val);
      return NULL;
    }
    p_flcont->t_flcont_val[0] = '\0'; // required to ensure strlen() works correctly on this memory
    printf("[alloc_file_cont_NEW] file cont allocated, size=%d\n", size);
  }
  return p_flcont->t_flcont_val;
}

// Free the file name memory.
// - p_flname: pointer to a file name object to free the memory
void free_file_name_NEW(t_flname *p_flname)
{
  if (p_flname) {
    free(p_flname);
    p_flname = NULL;
    printf("[free_file_name_NEW] file name freed\n");
  }
}

// Free the file content memory.
// - p_flcont: pointer to a file content object to free the memory
void free_file_cont_NEW(t_flcont *p_flcont)
{
  if (p_flcont && p_flcont->t_flcont_val) {
    free(p_flcont->t_flcont_val);
    p_flcont->t_flcont_val = NULL;
    p_flcont->t_flcont_len = 0;
    printf("[free_file_cont_NEW] file cont freed\n");
  }
}

// Free the file info memory.
// - p_file: pointer to a file info object to free the memory
void free_file_inf_NEW(file_inf *p_file)
{
  if (p_file) {
    free_file_name_NEW(&p_file->name); // free the file name memory
    free_file_cont_NEW(&p_file->cont); // free the file content memory
    printf("[free_file_inf_NEW] file inf freed\n");
  }
}

// Free the error info memory.
// - p_err: pointer to an error info object to free the memory
void free_err_inf_NEW(err_inf *p_err)
{
  if (p_err && p_err->err_inf_u.msg) {
    free(p_err->err_inf_u.msg);
    p_err->err_inf_u.msg = NULL;
    p_err->num = 0;
    printf("[free_err_inf_NEW] err inf freed\n");
  }
}

/*
 * Reset the file info.
 * - p_file: pointer to a file info object to reset the memory
 * - size_fcont: the size of the memory to allocate for the file content
 * RC: 0 on success, >0 on failure.
 * Features:
 * - file name, size is constant (LEN_PATH_MAX):
 * Memory allocation (if name is NULL) or setting it to 0 (if name is NOT NULL).
 * Memory deallocation must be done separately outside of this function.
 * - file content, size is variable:
 * Memory deallocation and allocation new one with the size_fcont size.
 * Memory deallocation must be done separately outside of this function.
 */
// TODO: check if the error numbers are ok in this function
int reset_file_inf_NEW(file_inf *p_file, unsigned size_fcont)
{
  // Check if the file info object is allocated
  if (!p_file) {
    fprintf(stderr, "Error 8: Failed to reset the file info. p_file=%p\n", (void*)p_file);
    return 8;
  }

  // Reset the file name
  if (!p_file->name) {
    // Initial dynamic memory allocation.
    // It performs for the first call of this function or after memory freeing.
    // Deallocation is required later
    p_file->name = (char*)malloc(LEN_PATH_MAX);
    if (!p_file->name) {
      fprintf(stderr, "Error 9: Failed to allocate a memory for the file name, name ptr=%p\n", 
              (void*)p_file->name);
      return 9;
    }
    printf("[reset_file_inf_NEW] file name allocated\n");
  }
  else {
    // Reset the previous file name.
    // Due to its constant size, the file name is reset by setting the memory to 0.
    // Since the file name changes often, a memory reset should occur in each call of this function.
    memset(p_file->name, 0, strlen(p_file->name));
    printf("[reset_file_inf_NEW] file name set to 0\n");
  }

  // Reset the file content
  // Due to its variable size, the file contents are reset by reallocating memory rather than setting it to 0.
  free_file_cont_NEW(&p_file->cont);
  if ( !alloc_file_cont_NEW(&p_file->cont, size_fcont) ) 
    return 10;

  printf("[reset_file_inf_NEW] DONE\n");
  return 0;
}

/*
 * Reset the error info.
 * - p_err: pointer to an error info object to reset the memory
 * RC: 0 on success, >0 on failure.
 * Features:
 * - error message memory is allocated only if it's not allocated (size is LEN_ERRMSG_MAX)
 * - error number is set to 0
 * - error message is set to 0 if the error number is set
 * - system error number is set to 0
 */
// TODO: check if the error numbers are ok in this function
int reset_err_inf_NEW(err_inf *p_err)
{
  // Check if the error info object is allocated
  if (!p_err) {
    fprintf(stderr, "Error 13: Failed to reset the error information, p_err=%p\n", (void*)p_err);
    return 11;
  }

  // Reset the error message
  if (!p_err->err_inf_u.msg) {
  // Initial dynamic memory allocation.
  // It performs for the first call of this function or after memory freeing.
  // Deallocation is required later
    p_err->err_inf_u.msg = (char*)malloc(LEN_ERRMSG_MAX);
    if (!p_err->err_inf_u.msg) {
      fprintf(stderr, "Error 14: Failed to allocate a memory for the error info, msg ptr=%p\n", 
              (void*)p_err->err_inf_u.msg);
      return 12;
    }
    p_err->num = 0;
    printf("[reset_err_inf_NEW] error info allocated\n");
  }
  else {
    // Reset the previous error info.
    // Due to its constant size, the error's message is reset by setting the memory to 0.
    // Since the error message does not change very often, a memory reset will occur only
    // if the error number is set.
    if (p_err->num) {
      p_err->num = 0;
      memset(p_err->err_inf_u.msg, 0, strlen(p_err->err_inf_u.msg));
      printf("[reset_err_inf_NEW] error info set to 0\n");
    }
  }

  // Reset the system error number
  if (errno) errno = 0;

  printf("[reset_err_inf_NEW] DONE\n");
  return 0;
}

/*
 * Convert the passed relative path to the full (absolute) one.
 * - path_rel: a relative path
 * - p_flerr: pointer to the file & error info to put the results.
 * RC: returns full (absolute) path (the same as p_flerr->file.name) on success, and NULL on failure.
 */
char * rel_to_full_path(const char *path_rel, file_err *p_flerr)
{
  if (!realpath(path_rel, p_flerr->file.name)) {
    p_flerr->err.num = 80;
    sprintf(p_flerr->err.err_inf_u.msg,
            "Error %i: Failed to resolve the specified path:\n'%s'\n%s\n",
            p_flerr->err.num, path_rel, strerror(errno));
    return NULL;
  }
  return p_flerr->file.name;
}

// TODO: document this function
/*
 * TODO: document this function
 * RC: 0 on success, >0 on failure.
 */
int get_file_stat(const char *dirname, const char *filename, struct stat *p_statbuf, char **errmsg)
{
  char fullpath[LEN_PATH_MAX];

  // Construct the full path to the needed file
  // A NULL-character appends to fullpath automatically by snprintf()
  if ( snprintf(fullpath, LEN_PATH_MAX, "%s/%s", dirname, filename) < 0 ) {
    *errmsg = malloc(LEN_PATH_MAX + 100); // allocate an approximate memory size to store the entire error message
    if (*errmsg) sprintf(*errmsg, "get_file_stat(): Invalid path to filename:\n'%s/%s'\n", 
                         dirname, filename);
    return 1;
  }

  // Get the file status (info)
  if (lstat(fullpath, p_statbuf) == -1) {
    *errmsg = malloc(LEN_PATH_MAX + 100); // allocate an approximate memory size to store the entire error message
    if (*errmsg) sprintf(*errmsg, "get_file_stat(): Cannot get the file status for:\n'%s/%s'\n%s\n",
                         dirname, filename, strerror(errno));
    return 2;
  }

  return 0;
}

// TODO: update this documentation as the function has changed
/*
 * Get file information.
 *
 * This function gathers and formats information about a specified file or directory.
 * It populates the file content data (in string format) in the provided `file_inf` structure
 * with details about the file's permissions, owner, group, size, modification time, and name.
 *
 * The function performs the following steps:
 * 1. Constructs the full path to the specified file.
 * 2. Retrieves the file status information using `lstat()`.
 * 3. Formats and appends the file's type and permissions, owner, group, size,
 *    modification time, and name to the `t_flcont_val` buffer in the `file_inf` structure.
 *
 * The `t_flcont_val` buffer is updated to contain the formatted file information.
 *
 * Parameters:
 *   p_dir_base - Pointer to a `file_inf` structure that contains information about
 *                the directory or file. This structure will be updated with the
 *                gathered file information.
 *   filename   - The name of the file for which information is to be gathered.
 *
 * RC: 0 on success, >0 on failure.
 */
void get_file_info(struct stat *p_statbuf, const char *filename, char *p_buff)
{
  char           strperm[11];
  struct passwd *pwd;
  struct group  *grp;
  struct tm     *tm;
  char           datestring[32];

  // TODO: maybe reallocate the file content here?

  // Print out type and permissions
  p_buff += sprintf(p_buff, "%s", str_perm(p_statbuf->st_mode, strperm));

  // Print out owner's name if it is found using getpwuid()
  // TODO: determine the length of the field as a max length among all the entries
  // And then the following format can be used: "  %-Ns"
  if ((pwd = getpwuid(p_statbuf->st_uid)) != NULL)
    p_buff += sprintf(p_buff, "  %-8.8s", pwd->pw_name);
  else
    p_buff += sprintf(p_buff, "  %-8d", p_statbuf->st_uid);

  // Print out group name if it is found using getgrgid()
  if ((grp = getgrgid(p_statbuf->st_gid)) != NULL)
    p_buff += sprintf(p_buff, " %-8.8s", grp->gr_name);
  else
    p_buff += sprintf(p_buff, " %-8d", p_statbuf->st_gid);

  // Print size of file
  p_buff += sprintf(p_buff, " %9jd", (intmax_t)p_statbuf->st_size);

  // Get localized date string
  // NOTE: the 'ls' command has 2 different formats for this date&time string:
  // - if a mod.time is less than 1 year then it's used the format with time and without year,
  // - if a mod.time is more than 1 year then it's used the format without time but with year.
  // difftime() function may be used to determine the right format similar to 'ls'.
  // Potentially a similar approach can be used in this program, but I have decided that
  // printing month, day, time and year is more informative and sufficient for now.
  tm = localtime(&p_statbuf->st_mtime);
  strftime(datestring, sizeof(datestring), "%b %d %R %Y", tm);
  p_buff += sprintf(p_buff, " %s %s\n", datestring, filename);
}

/*
 * List the directory content into the char array stored in the file_err struct.
 * - p_flerr: pointer to an allocated and nulled RPC struct object to store file & error info.
 * RC: 0 on success, >0 on failure.
 * The function should always receive a directory as the file type.
 * The file name in the passed struct specifies the directory to be read.
 * The file contents in the passed struct are populated with the directory contents, 
 * which are then returned to the calling function.
 */
int ls_dir_str(file_err *p_flerr)
{
  DIR           *hdir; // directory handler
  struct dirent *de; // directory entry
  struct stat    statbuf;
  char          *errmsg = NULL;

  // Open the passed directory
  if ( (hdir = opendir(p_flerr->file.name)) == NULL ) {
    p_flerr->err.num = 85;
    sprintf(p_flerr->err.err_inf_u.msg, "Error %i: Cannot open directory:\n'%s'\n%s\n",
            p_flerr->err.num, p_flerr->file.name, strerror(errno));
    return p_flerr->err.num;
  }

  // // Preliminary loop through directory entries to get settings values 
  // // for subsequent printing of the directory content
  // while ((de = readdir(hdir)) != NULL) {
  //   // Get the file status (info)
  //   if ( get_file_stat(p_flerr->file.name, de->d_name, &statbuf, &errmsg) > 0 ) {
  //     printf("%s", errmsg); // print the error message
  //     free(errmsg); // free allocated memory
  //   }
  //   // TODO: continue...
  // }

  // Reset position of directory stream to the beginning of a directory
  // rewinddir(hdir);

  // Loop through directory entries to get entry info
  char *p_curr; // pointer to the current position in the dir content buffer
  while ((de = readdir(hdir)) != NULL) {
    // Calc the current position in buffer where to place the next (in this iteration) portion of data
    p_curr = p_flerr->file.cont.t_flcont_val + strlen(p_flerr->file.cont.t_flcont_val);
    
    // Get the file status (info)
    if ( get_file_stat(p_flerr->file.name, de->d_name, &statbuf, &errmsg) != 0 ) {
      strcpy(p_curr, errmsg); // copy the error message to buffer
      free(errmsg); // free allocated memory
    }
    else
      get_file_info(&statbuf, de->d_name, p_curr); // get entry info and add it to buffer
  }
  closedir(hdir);
  return 0;
}

/*
 * Select a file: determine a file type and get the full (absolute) path to the file.
 * - path: file path that needs to be selected.
 * - p_flerr: pointer to an allocated and nulled RPC struct to store file & error info;
 *   is used here to set and return of the result through the function argument.
 * RC: 0 on success, >0 on failure.
 */
int select_file(const char *path, file_err *p_flerr)
{
  // Determine the file type
  p_flerr->file.type = file_type(path); 
  
  // Convert the passed path to the full (absolute) path
  if ( !rel_to_full_path(path, p_flerr) )
    return p_flerr->err.num;

  // Process the file type
  switch (p_flerr->file.type) {
    case FTYPE_DIR:
      // Get the directory content and save it to file_err object
      if (p_flerr->file.type == FTYPE_DIR)
        ls_dir_str(p_flerr); // if error has occurred it was set to p_flerr, no need to check the RC
    case FTYPE_REG:
      break; // just exit from switch for both dir and file
    case FTYPE_OTH:
      p_flerr->err.num = 81;
      sprintf(p_flerr->err.err_inf_u.msg,
              "Error %i: Unsupported file type:\n'%s'\n"
              "Only regular file can be chosen\n",
              p_flerr->err.num, p_flerr->file.name);
      break;
    case FTYPE_NEX:
    case FTYPE_INV:
      // NOTE: If a call of rel_to_full_path() failed due to missing file, 
      // these 2 cases (FTYPE_NEX & FTYPE_INV) will never be reached.
      p_flerr->err.num = 82;
      printf("[select_file] 6\n");
      sprintf(p_flerr->err.err_inf_u.msg,
              "Error %i: Invalid file:\n'%s'\n%s\n",
              p_flerr->err.num, p_flerr->file.name, strerror(errno));
      break;
  }
  return p_flerr->err.num;
}

/*
 * Constructs a full path by appending a new path segment to the path delimeter '/'.
 * This function appends the provided new path segment (path_new) to the path delimeter '/',
 * storing the resulting full path in path_full.
 * - path_new: a pointer to a character array containing the new path segment.
 * - lenmax: the maximum length of the resulting path (`path_full`), including the null terminator.
 * - path_full: a pointer to a character array where the resulting full path will be stored.
 * Returns the number of characters written to path_full on success (excluding the null terminator).
 * RC: >0 on success, <=0 on failure.
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
 * Get the filename in the interactive mode.
 * - dir_start: a starting directory for the traversal
 * - path_res: an allocated char array to store the resulting file path
 * RC: returns path_res on success, and NULL on failure.
 *
 * Procedure:
 * 1. ls dir
 * 2. get user input
 * 3. add user input to the current path
 * 3. check the input:
 *    3.1 if it's a dir, go to p.1.
 *    3.2 if it's not a dir, do the following:
 *        >> for source file:
 *        - if it's a regular file - OK and return its full path
 *        - if it does NOT a regular file - error and message like please choose a regular file
 *        - if it does NOT exist - error and message like the file ... doesn't exist
 *        >> for target file:
 *        - if it does NOT exist - OK and return its full path
 *        - if it exists (type doesn't matter) - error and message like file exists, please specify another one
 */
char * get_filename_inter(const char *dir_start, char *path_res)
{
  char path_curr[LEN_PATH_MAX]; // current path used to walk through the directories and construct path_res
  char path_prev[LEN_PATH_MAX]; // a copy of the previous path to restore it if necessary
  char fname_inp[LEN_PATH_MAX]; // inputted filename (LEN_PATH_MAX is used, as filename can be an absolute path)
  char *pfname_inp; // pointer to the inputted filename; used to differentiate between relative and absolute paths
  int offset; // offset from the beginning of the current path for the added subdir/file
  int nwrt_fname; // number of characters written to the current path in each iteration
  file_err flerr; // a local struct object to store&pass the information about a file&error

  // Init the full path and offset
  offset = strlen(dir_start); // define offset as the start dir length that will be copied into path_curr
  strncpy(path_curr, dir_start, offset + 1); // strncpy doesn't add/copy '\0', so add "+1" to copy it
  strcpy(path_prev, "/"); // init the previous path with a root dir as a guaranteed valid path
  // TODO: copy a home directory to the previous path instead of the root dir, if it's possible

  while (1) {
    // Reset the file info before each call of select_file()
    // TODO: determine, maybe move a call of this function to select_file()?
    // TODO: implement the dynamic memory reallocation for file_inf object 
    if (reset_file_inf_NEW(&flerr.file, 10000) != 0) {
      fprintf(stderr, "Failed to reset the file information\n");
      return NULL;
    }

    // Reset the error info before each call of select_file()
    // TODO: determine, maybe move a call of this function to select_file()?
    if (reset_err_inf_NEW(&flerr.err) != 0) {
      fprintf(stderr, "Failed to reset the error information\n");
      return NULL;
    }

    // TODO: replace select_file() with a function pointer that can be either 
    // select_file() or pick_entity() RPC function
    if (select_file(path_curr, &flerr) == 0) {
      // There was a successful file selection (regular or directory)
      if (flerr.file.type == FTYPE_REG) {
        copy_path(flerr.file.name, path_res);
        return path_res;
      }
    }
    else {
      // An error occurred while selecting a file 
      fprintf(stderr, "%s\n", flerr.err.err_inf_u.msg); // print the error message
      printf("[get_filename_inter] 1, filetype=%i\n", (int)flerr.file.type);
      offset = copy_path(path_prev, path_curr); // restore the previous valid path
      continue; // start loop from the beginning to select the previous valid path
    }
    
    /* At this point we're sure that it's selected a valid directory */

    // Print the full path and content of the current directory
    printf("\n%s:\n%s\n", flerr.file.name, flerr.file.cont.t_flcont_val);

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

    // Construct the full path of the choosed file
    nwrt_fname = construct_full_path(pfname_inp, LEN_PATH_MAX - offset, path_curr + offset);
    if (nwrt_fname <= 0) return NULL;
    
    // Increment the offset by the number of chars written to path_curr (after completed checks)
    offset += nwrt_fname;

    //  printf("[get_filename_inter] 2, path_curr: '%s'\n", path_curr);
  }
  return NULL;
}

// DELETE: it's just for testing
// Compile full:  gcc -I/usr/include/tirpc -o fs_opers fs_opers.c -ltirpc
// Compile short: gcc -I/usr/include/tirpc fs_opers.c
int main()
{
  if (!get_filename_inter(".", filename_src))
    return 1;
  printf("!!! File has been choosed:\n'%s'\n", filename_src);
  return 0;
}

