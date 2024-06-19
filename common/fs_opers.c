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

/*
 * Convert file permissions from numeric to symbolic form.
 *
 * This function converts the file mode (permissions) from its numeric
 * representation to the symbolic representation used by the `ls -l` command.
 * It includes the file type character at the first position.
 *
 * Parameters:
 *  mode    - The mode of the file.
 *  strmode - A pointer to a character array where the symbolic representation 
 *            will be stored. The array must be at least 11 characters long.
 * 
 * Return value:
 *  A pointer to the `strmode` character array.
 */
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
 * Return the file type for the specified file.
 *
 * This function determines the type of a file given its path.
 * It returns a file type enumeration value defined in `fltr.h`.
 *
 * Parameters:
 *  filepath - A full path to the file.
 * 
 * Return value:
 * A filetype enum value:
 *         - FTYPE_DIR for directory,
 *         - FTYPE_REG for regular file,
 *         - FTYPE_OTH for other file types,
 *         - FTYPE_NEX if the file does not exist,
 *         - FTYPE_INV for an invalid file type or other errors.
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

/*
 * Copy a source path to a target path with a maximum length.
 *
 * This function copies the string `path_src` to `path_trg` ensuring that
 * the number of characters copied does not exceed `LEN_PATH_MAX`.
 * It uses `snprintf` to perform the copy, which guarantees that the resulting
 * string is null-terminated and that it does not write beyond the specified
 * maximum length.
 *
 * Parameters:
 *  path_src - The source path to be copied.
 *  path_trg - The target buffer where the source path will be copied.
 *             The buffer should be at least `LEN_PATH_MAX` characters long.
 * 
 * Return value:
 *  The number of characters written to `path_trg`, not including the null-terminator.
 */
int copy_path(char *path_src, char *path_trg)
{
  return snprintf(path_trg, LEN_PATH_MAX, "%s", path_src);
}

/*
 * NEW VERSIONS, after their acceptance delete old versions to manipulate the memory
 */
/*
 * NEW version of the memory manipulation functions, initialy defined in prg_serv.c and prg_clnt.c.
 */
// TODO: move these versions to the new files like common/memops.c, and use them everywhere where they're required.
// TODO: check if the error number is ok in this function; this function should be used here and in both client and server code.
/*
 * Allocate memory to store the file content.
 *
 * This function allocates memory to store file content if it hasn't already been
 * allocated. If memory has already been allocated, it returns a pointer to the
 * existing memory.
 *
 * Parameters:
 * p_flcont - A pointer to a file content object where the allocated memory will be stored.
 * size     - The size of the memory to allocate.
 * p_err    - A pointer to an error info object where error info will be written in case of an error.
 * 
 * Return value:
 *  A pointer to the allocated memory, or NULL in case of an error.
 */
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

/*
 * Free the file name memory.
 * This function frees the memory allocated for a file name object.
 *
 * Parameters:
 *  p_flname - A pointer to a file name object whose memory is to be freed.
 */
void free_file_name_NEW(t_flname *p_flname)
{
  if (p_flname) {
    free(p_flname);
    p_flname = NULL;
    printf("[free_file_name_NEW] file name freed\n");
  }
}

/*
 * Free the file content memory.
 * This function frees the memory allocated for a file content object.
 *
 * Parameters:
 *  p_flcont - A pointer to a file content object whose memory is to be freed.
 */
void free_file_cont_NEW(t_flcont *p_flcont)
{
  if (p_flcont && p_flcont->t_flcont_val) {
    free(p_flcont->t_flcont_val);
    p_flcont->t_flcont_val = NULL;
    p_flcont->t_flcont_len = 0;
    printf("[free_file_cont_NEW] file cont freed\n");
  }
}

/*
 * Free the file info memory.
 * This function frees the memory allocated for a file info object.
 *
 * Parameters:
 *  p_file - A pointer to a file info object whose memory is to be freed.
 */
void free_file_inf_NEW(file_inf *p_file)
{
  if (p_file) {
    free_file_name_NEW(&p_file->name); // free the file name memory
    free_file_cont_NEW(&p_file->cont); // free the file content memory
    printf("[free_file_inf_NEW] file inf freed\n");
  }
}

/*
 * Free the error info memory.
 * This function frees the memory allocated for an error info object.
 *
 * Parameters:
 *  p_err - A pointer to an error info object whose memory is to be freed.
 */
void free_err_inf_NEW(err_inf *p_err)
{
  if (p_err && p_err->err_inf_u.msg) {
    free(p_err->err_inf_u.msg);
    p_err->err_inf_u.msg = NULL;
    p_err->num = 0;
    printf("[free_err_inf_NEW] err inf freed\n");
  }
}

// TODO: check if the error numbers are ok in this function
/*
 * Reset the file info.
 *
 * This function resets a file info object by reallocating memory for its name
 * and content. The file name's memory is allocated only if it is currently
 * unallocated. The file content's memory is always reallocated to the specified size.
 *
 * Parameters:
 *  p_file      - A pointer to a file info object to reset.
 *  size_fcont  - The size of the memory to allocate for the file content.
 * 
 * Return value:
 *  0 on success, >0 on failure.
 * 
 * Notes:
 * - file name, size is constant (LEN_PATH_MAX):
 *   Memory allocation, if name is NULL, or setting it to 0, if name is NOT NULL.
 *   Memory deallocation must be done separately outside of this function.
 * - file content, size is variable:
 *   Memory deallocation and allocation of new one with the size_fcont size.
 *   Memory deallocation must be done separately outside of this function.
 */
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

// TODO: check if the error numbers are ok in this function
/*
 * Reset the error info.
 *
 * This function resets an error info object by allocating memory for the error
 * message if it is not already allocated and setting the error number and system
 * error number to 0. The error message is reset if the error number is set.
 *
 * Parameters:
 *  p_err - A pointer to an error info object to reset.
 * 
 * Return value:
 *  0 on success, >0 on failure.
 * 
 * Notes:
 * - error message memory is allocated only if it's not allocated (size is LEN_ERRMSG_MAX)
 * - error number is set to 0
 * - error message is set to 0 if the error number is set
 * - system error number is set to 0
 */
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
 *
 * This function converts a relative path to an absolute path. If the conversion
 * fails, it sets an error number and message in the provided error info structure.
 *
 * Parameters:
 *  path_rel - A relative path to be converted.
 *  p_flerr  - A pointer to a file & error info structure where the results will be stored.
 * 
 * Return value:
 *  The full (absolute) path on success (the same as p_flerr->file.name), and NULL on failure.
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

/*
 * Get the file status (info).
 *
 * This function constructs the full path to a specified file and retrieves its
 * status information. If the full path is invalid or if the status retrieval fails,
 * an error message is set.
 *
 * Parameters:
 *  dirname   - The directory name where the file is located.
 *  filename  - The name of the file whose status is to be retrieved.
 *  p_statbuf - A pointer to a stat structure where the file status information will be stored.
 *  errmsg    - A pointer to a string where an error message will be stored in case of failure.
 *
 * Return value:
 *  0 on success, >0 on failure.
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

// Directory listing settings
struct lsdir_setts
{
  int numb_files;   // number of files
  int lenmax_usr;   // max length of the user (owner) name
  int lenmax_grp;   // max length of the group name
  int lenmax_size;  // max length of the file size
};

/*
 * Calculate the number of digits in a number.
 *
 * This function calculates the number of digits in a given long integer
 * by repeatedly dividing the number by 10 until it becomes zero.
 *
 * Parameters:
 *  val - The number whose digits are to be counted.
 * 
 * Return value:
 *  The number of digits in the input number.
 */
int numb_digits(long val)
{
  int numb = 1;
  while ((val /= 10) > 0) ++numb;
  return numb;
}

/*
 * Update directory listing settings based on file statistics.
 *
 * This function updates various settings related to directory listing,
 * such as the number of files, the maximum length of user (owner) names,
 * group names, and file sizes. It uses the file statistics provided by
 * the `struct stat` and updates the settings in the `struct lsdir_setts`.
 *
 * Parameters:
 *  p_statbuf - Pointer to a `struct stat` containing file statistics.
 *  p_lsd_set - Pointer to a `struct lsdir_setts` to be updated.
 */
void update_lsdir_setts(struct stat *p_statbuf, struct lsdir_setts *p_lsd_set)
{
  struct passwd *pwd;
  struct group *grp;
  int len = 0;

  // Update the number of files
  p_lsd_set->numb_files++;

  // Update the max length of the user (owner) name  
  if ((pwd = getpwuid(p_statbuf->st_uid)) != NULL) {
    len = strlen(pwd->pw_name);
    if (len > p_lsd_set->lenmax_usr)
      p_lsd_set->lenmax_usr = len;
  }

  // Update the max length of the group name  
  if ((grp = getgrgid(p_statbuf->st_gid)) != NULL) {
    len = strlen(grp->gr_name);
    if (len > p_lsd_set->lenmax_grp)
      p_lsd_set->lenmax_grp = len;
  }

  // Update the max file size
  len = numb_digits(p_statbuf->st_size);
  if (len > p_lsd_set->lenmax_size)
    p_lsd_set->lenmax_size = len;
}

/*
 * Get file information.
 *
 * This function gathers and formats information about a specified file or directory.
 * It populates the provided buffer with details about the file's permissions, owner,
 * group, size, modification time, and name.
 *
 * The function performs the following steps:
 * 1. Retrieves the file status information from the passed 'stat' struct.
 * 2. Formats and appends to the provided buffer the file's type and permissions,
 *    owner, group, size, modification time, and name.
 *
 * Parameters:
 *   p_statbuf  - Pointer to a `struct stat` that contains the file status information.
 *   filename   - The name of the file for which information is to be gathered.
 *   p_lsd_set  - Pointer to a `struct lsdir_setts` that contains the directory listing settings.
 *   p_buff     - Pointer to the buffer where the formatted file information will be stored.
 *
 * Return value:
 *   This function does not return a value. The formatted file information is stored in the
 *   buffer pointed to by `p_buff`.
 *
 * Notes:
 * - The `p_buff` buffer is expected to be pre-allocated and large enough to hold the formatted
 *   file information.
 * - The `str_perm` function is assumed to convert the file mode to a string representing the
 *   file's type and permissions.
 * - This function prints the owner and group names if they can be retrieved using `getpwuid()`
 *   and `getgrgid()` respectively; otherwise, it prints the numeric UID and GID.
 * - The modification time is printed in the format "%b %d %R %Y", which includes the month, day,
 *   time, and year.
 */
void get_file_info(struct stat *p_statbuf, const char *filename, 
                   struct lsdir_setts *p_lsd_set, char *p_buff)
{
  char           strperm[11];
  struct passwd *pwd;
  struct group  *grp;
  struct tm     *tm;
  char           datestring[32];

  // TODO: maybe reallocate the file content here?

  // Print the file type and permissions
  p_buff += sprintf(p_buff, "%s", str_perm(p_statbuf->st_mode, strperm));

  // Print the file owner's name if it's found using getpwuid()
  if ((pwd = getpwuid(p_statbuf->st_uid)) != NULL)
    p_buff += sprintf(p_buff, "  %-*s", p_lsd_set->lenmax_usr, pwd->pw_name);
  else
    p_buff += sprintf(p_buff, "  %-*d", p_lsd_set->lenmax_usr, p_statbuf->st_uid);

  // Print the file group name if it's found using getgrgid()
  if ((grp = getgrgid(p_statbuf->st_gid)) != NULL)
    p_buff += sprintf(p_buff, " %-*s", p_lsd_set->lenmax_grp, grp->gr_name);
  else
    p_buff += sprintf(p_buff, " %-*d", p_lsd_set->lenmax_grp, p_statbuf->st_gid);

  // Print the file size
  p_buff += sprintf(p_buff, " %*jd", p_lsd_set->lenmax_size, (intmax_t)p_statbuf->st_size);

  // Get localized date string and print it
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
  DIR                 *hdir; // directory handler
  struct dirent       *de; // directory entry
  struct stat         statbuf; // struct contains the file status information
  struct lsdir_setts  lsdir_set = {0, 0, 0, 0}; // the directory listing settings
  char                *errmsg = NULL;

  // Open the passed directory
  if ( (hdir = opendir(p_flerr->file.name)) == NULL ) {
    p_flerr->err.num = 85;
    sprintf(p_flerr->err.err_inf_u.msg, "Error %i: Cannot open directory:\n'%s'\n%s\n",
            p_flerr->err.num, p_flerr->file.name, strerror(errno));
    return p_flerr->err.num;
  }

  // Preliminary loop through directory entries to get settings values 
  // for subsequent flexible printing of the directory content
  while ((de = readdir(hdir)) != NULL) {
    // Get the file status (info)
    if ( get_file_stat(p_flerr->file.name, de->d_name, &statbuf, &errmsg) == 0 )
      update_lsdir_setts(&statbuf, &lsdir_set);
    else {
      // printf("%s", errmsg); // print the error message - decided to not do it, maybe for debug only
      free(errmsg); // free allocated memory
    }
  }

  // Reset the position of directory stream to the beginning
  rewinddir(hdir);

  // Loop through directory entries to get entry info with flexible fields width
  char *p_curr; // pointer to the current position in the dir content buffer
  while ((de = readdir(hdir)) != NULL) {
    // Calc the current position in buffer where to place the next (in this iteration) portion of data
    p_curr = p_flerr->file.cont.t_flcont_val + strlen(p_flerr->file.cont.t_flcont_val);
    
    // Get the file status (info)
    if ( get_file_stat(p_flerr->file.name, de->d_name, &statbuf, &errmsg) == 0 )
      get_file_info(&statbuf, de->d_name, &lsdir_set, p_curr); // get entry info and add it to the buffer
    else {
      strcpy(p_curr, errmsg); // copy the error message to the buffer
      free(errmsg); // free allocated memory
    }
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

