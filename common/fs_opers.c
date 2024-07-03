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

#define DBG_MAIN 0  // debug this file adding main()
#define DBG_FT 0    // debug the file type info
#define DBG_MEM 0   // debug the memory manipulation
#define DBG_PATH 0  // debug the path's manipulation

// Global system variable that store the error number
extern int errno;
static char filename_src[LEN_PATH_MAX]; // DELETE: it's just for testing
static char filename_trg[LEN_PATH_MAX]; // DELETE: it's just for testing

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
enum filetype get_file_type(const char *filepath)
{
  if (DBG_FT) printf("[get_file_type] 0, filepath: '%s'\n", filepath);
  enum filetype ftype;
  struct stat statbuf;

  // Check if stat() returns an error due to file non-existance, return FTYPE_NEX; 
  // otherwise, return an invalid file type - FTYPE_INV.
  if (stat(filepath, &statbuf) == -1) {
    if (DBG_FT) printf("[get_file_type] 1, stat() error - File %s\n", 
                       errno == ENOENT ? "not exist" : "invalid");
    return errno == ENOENT ? FTYPE_NEX : FTYPE_INV;
  }

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
  if (DBG_FT) printf("[get_file_type] $, filetype: %d\n", (int)ftype);
  return ftype;
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
char * alloc_file_cont_NEW(t_flcont *p_flcont, size_t size)
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
      // NOTE: print this error in stderr only on the side of the caller of this function.
      // Because these are the details that another side should not be interested in.
      // A caller of this function can produce a more general error message and send it 
      // to another side through the error info object, which is not accessible here.
      fprintf(stderr, "Error 7: Failed to allocate memory for the file content, ptr to file cont val: %p\n", 
              (void*)p_flcont->t_flcont_val);
      return NULL;
    }
    p_flcont->t_flcont_val[0] = '\0'; // required to ensure strlen() works correctly on this memory
    if (DBG_MEM) printf("[alloc_file_cont_NEW] file cont allocated, size=%d\n", size);
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
    if (DBG_MEM) printf("[free_file_name_NEW] DONE\n");
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
    if (DBG_MEM) printf("[free_file_cont_NEW] DONE\n");
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
    if (DBG_MEM) printf("[free_file_inf_NEW] DONE\n");
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
    if (DBG_MEM) printf("[free_err_inf_NEW] DONE\n");
  }
}

// TODO: check if the error numbers are ok in this function
/*
 * Reset the file name & type.
 *
 * The file name's memory size is constant (LEN_PATH_MAX) and it's allocated just once
 * if it is currently unallocated.
 * If name is NULL -> memory allocation occurs, if name is NOT NULL -> set memory to 0.
 * Memory deallocation must be done separately outside of this function.
 *
 * Any errors encountered in this function are printed to stderr on the same side where they occur,
 * as these details should not be of interest to the other side. The caller of this function
 * can generate a more general error message and communicate it to the other side
 * through the error info object.
 *
 * Note: This function does not populate the error info object to convey errors
 * to the other side.
 *
 * Parameters:
 *  p_file      - A pointer to a file info object to reset.
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int reset_file_name_type_NEW(file_inf *p_file)
{
  // Check if the file info object is allocated
  if (!p_file) {
    fprintf(stderr, "Error 8: Failed to reset the file name & type. p_file=%p\n", (void*)p_file);
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
    p_file->name[0] = '\0'; // required to ensure strlen() works correctly on this memory
    if (DBG_MEM) printf("[reset_file_name_type_NEW] file name allocated\n");
  }
  else {
    // Reset the previous file name.
    // Due to its constant size, the file name is reset by setting the memory to 0.
    // Since the file name changes often, a memory reset should occur in each call of this function.
    memset(p_file->name, 0, strlen(p_file->name));
    if (DBG_MEM) printf("[reset_file_name_type_NEW] file name set to 0\n");
  }

  // Reset the file type
  p_file->type = FTYPE_DFL;

  if (DBG_MEM) printf("[reset_file_name_type_NEW] DONE\n");
  return 0;
}

/*
 * Reset the file content.

 * Memory deallocation and allocation of new one with the size_fcont size.
 * Due to its variable size, the file contents are reset by reallocating memory
 * rather than setting it to 0.
 * Memory deallocation must be done separately outside of this function.
 *
 * Parameters:
 *  p_flcont    - A pointer to a file content object to reset.
 *  size_fcont  - The size of the memory to allocate for the file content.
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int reset_file_cont_NEW(t_flcont *p_flcont, size_t size_fcont)
{
  free_file_cont_NEW(p_flcont);
  if (!alloc_file_cont_NEW(p_flcont, size_fcont))
    return 10;
  if (DBG_MEM) printf("[reset_file_cont_NEW] DONE\n");
  return 0;
}

/*
 * Reset the file info.
 *
 * This function resets the file information, including the file name, type, and content.
 * It allocates new memory for the file content based on the specified size.
 *
 * Parameters:
 *  p_file      - A pointer to a file info object to reset.
 *  size_fcont  - The size of the memory to allocate for the file content.
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int reset_file_inf_NEW(file_inf *p_file, size_t size_fcont)
{
  // check if the file info object is allocated
  if (!p_file) {
    fprintf(stderr, "Error 8: Failed to reset the file info. p_file=%p\n", (void*)p_file);
    return 8;
  }

  // reset the file name & type
  int rc;
  if ( (rc = reset_file_name_type_NEW(p_file)) != 0 )
    return rc;
  
  // reset the file content
  if ( (rc = reset_file_cont_NEW(&p_file->cont, size_fcont)) != 0 )
    return rc;
  
  if (DBG_MEM) printf("[reset_file_inf_NEW] DONE\n");
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
    p_err->err_inf_u.msg[0] = '\0'; // required to ensure strlen() works correctly on this memory
    p_err->num = 0;
    if (DBG_MEM) printf("[reset_err_inf_NEW] error info allocated\n");
  }
  else {
    // Reset the previous error info.
    // Due to its constant size, the error's message is reset by setting the memory to 0.
    // Since the error message does not change very often, a memory reset will occur only
    // if the error number is set.
    if (p_err->num) {
      p_err->num = 0;
      memset(p_err->err_inf_u.msg, 0, strlen(p_err->err_inf_u.msg));
      if (DBG_MEM) printf("[reset_err_inf_NEW] error info set to 0\n");
    }
  }

  // Reset the system error number
  if (errno) errno = 0;

  if (DBG_MEM) printf("[reset_err_inf_NEW] DONE\n");
  return 0;
}

/*
 * Convert the passed relative path to the full (absolute) one.
 *
 * This function converts a relative path to an absolute path. If the conversion
 * fails, it allocates memory for an error message and sets it in the provided errmsg pointer.
 *
 * Parameters:
 *  path_rel  - A relative path to be converted.
 *  path_full - A buffer to store the resulting absolute path.
 *  errmsg    - A pointer to a char pointer where the error message will be stored
 *              if the conversion fails.
 *
 * Return value:
 *  The full (absolute) path on success (same as path_full), or NULL on failure with 
 *  the error messages in errmsg which should be freed by a caller of this function.
 */
char *rel_to_full_path(const char *path_rel, char *path_full, char **errmsg)
{
  if (!realpath(path_rel, path_full)) {
    // allocate an approximate memory size to store the entire error message
    *errmsg = malloc(LEN_ERRMSG_MAX + LEN_PATH_MAX);
    if (*errmsg)
      sprintf(*errmsg, "Failed to resolve the specified path:\n'%s'\n%s",
              path_rel, strerror(errno));
    return NULL;
  }
  return path_full;
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
int copy_path(const char *path_src, char *path_trg)
{
  return snprintf(path_trg, LEN_PATH_MAX, "%s", path_src);
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
    // allocate an approximate memory size to store the entire error message
    *errmsg = malloc(LEN_ERRMSG_MAX + LEN_PATH_MAX);
    if (*errmsg)
      sprintf(*errmsg, "get_file_stat(): Invalid path to filename:\n'%s/%s'\n", 
              dirname, filename);
    return 1;
  }

  // Get the file status (info)
  if (lstat(fullpath, p_statbuf) == -1) {
    // allocate an approximate memory size to store the entire error message
    *errmsg = malloc(LEN_ERRMSG_MAX + LEN_PATH_MAX);
    if (*errmsg)
      sprintf(*errmsg, "get_file_stat(): Cannot get the file status for:\n'%s/%s'\n%s\n",
              dirname, filename, strerror(errno));
    return 2;
  }

  return 0;
}

// Directory listing settings
struct lsdir_setts
{
  int numb_files;       // number of files
  int lenmax_usr;       // max length of the user (owner) name
  int lenmax_grp;       // max length of the group name
  int lenmax_size;      // max length of the file size
  size_t lensum_names;  // total filenames length
} lsdir_setts_dflt = {0, 0, 0, 0, 0}; // creation the default instance with default values

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
 * such as the number of files, the maximum length of: user (owner) names,
 * group names, file sizes, and also the total filenames length.
 * It uses the file statistics provided by the `struct stat` and name of the file,
 * and updates the settings in the `struct lsdir_setts`.
 * 
 * Parameters:
 *  p_statbuf - Pointer to a `struct stat` containing file statistics.
 *  filename  - The name of the file for which information is to be gathered.
 *  p_lsd_set - Pointer to a `struct lsdir_setts` to be updated.
 */
void update_lsdir_setts(struct stat *p_statbuf, const char *filename, struct lsdir_setts *p_lsd_set)
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

  // Update the total filenames length
  p_lsd_set->lensum_names += strlen(filename);
}

/*
 * Calculate the total size needed to list the directory content.
 *
 * This function computes the amount of memory required to store the directory content
 * based on the specified settings. The calculation includes space for file permissions,
 * owner, group, size, date, new line characters, and all filenames.
 *
 * NOTE. This function should be updated each time the way of listing the directory is updated
 *       in the get_file_info() function.
 *
 * Parameters:
 *  p_lsd_set - A pointer to the lsdir_setts structure containing settings for listing the directory.
 * 
 * Return value:
 *  The total size needed to store the directory content.
 */
size_t calc_dir_cont_size(const struct lsdir_setts *p_lsd_set)
{
  return (10                          // space for file permissions
        + 2 + p_lsd_set->lenmax_usr   // space for file owner
        + 1 + p_lsd_set->lenmax_grp   // space for file group
        + 1 + p_lsd_set->lenmax_size  // space for file size
        + 1 + 17                      // space for file date
        + 1 + 1)                      // space for new line character after the filename
        * p_lsd_set->numb_files       // multiply on the files number
        + p_lsd_set->lensum_names     // space for all filenames
        + 1;                          // space for trailing NULL character
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
 * - The calc_dir_cont_size() function should be updated each time the way of listing 
 *   the directory is updated in this function.
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
                   const struct lsdir_setts *p_lsd_set, char *p_buff)
{
  char           strperm[11];
  struct passwd *pwd;
  struct group  *grp;
  struct tm     *tm;
  char           datestring[32];

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
  // Potentially a similar approach can be used in this program, but I have decided 
  // that printing month, day, time and year is more informative and sufficient.
  tm = localtime(&p_statbuf->st_mtime);
  strftime(datestring, sizeof(datestring), "%b %d %R %Y", tm);
  p_buff += sprintf(p_buff, " %s %s\n", datestring, filename);
}

/*
 * List the directory content into the file content (char array) stored in the file_err struct.
 *
 * This function reads the contents of a specified directory and stores the directory entries
 * in the file content field of the passed file_err structure. The function assumes that the
 * file name in the passed struct specifies the directory to be read and that the file type 
 * is a directory.
 *
 * Errors encountered during these operations are stored in the error info field of 
 * the file_err structure.
 *
 * The function performs the following steps:
 * 1. Opens the specified directory.
 * 2. Iterates through the directory entries to gather settings for flexible listing.
 * 3. Resets the file content buffer based on the calculated size of the directory content.
 * 4. Iterates through the directory entries again to populate the file content buffer 
 *    with the directory listing.
 *
 * Parameters:
 *  p_flerr - Pointer to an allocated and nulled RPC struct object to store file & error info.
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int ls_dir_str(file_err *p_flerr)
{
  DIR                *hdir; // directory handler
  struct dirent      *de; // directory entry
  struct stat         statbuf; // struct contains the file status information
  struct lsdir_setts  lsdir_set = lsdir_setts_dflt; // the directory listing settings
  char               *errmsg = NULL;

  // Open the passed directory
  if ( (hdir = opendir(p_flerr->file.name)) == NULL ) {
    p_flerr->err.num = 85;
    sprintf(p_flerr->err.err_inf_u.msg, "Error %i: Cannot open directory:\n'%s'\n%s\n",
            p_flerr->err.num, p_flerr->file.name, strerror(errno));
    return p_flerr->err.num;
  }

  // Preliminary loop through directory entries to get settings values 
  // for subsequent flexible listing of the directory content
  while ((de = readdir(hdir)) != NULL) {
    // Get the file status (info)
    if ( get_file_stat(p_flerr->file.name, de->d_name, &statbuf, &errmsg) == 0 )
      update_lsdir_setts(&statbuf, de->d_name, &lsdir_set);
    else {
      // NOTE: Decided not to print the error message here, as such error messages will be retrieved
      // again and placed in the p_flerr->err object during the next loop through the directories.
      // printf("%s", errmsg);
      free(errmsg); // free allocated memory for returned error message
    }
  }

  // Reset the file content before filling it with directory listing data
  if (reset_file_cont_NEW(&p_flerr->file.cont, calc_dir_cont_size(&lsdir_set)) != 0) {
    // TODO: check an error number
    p_flerr->err.num = 86;
    sprintf(p_flerr->err.err_inf_u.msg,
            "Error %i: Failed to reset the file content\n", p_flerr->err.num);
    return p_flerr->err.num;
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

// A special error number if an error occurred while resetting the error info (used as workaround)
enum { ERRNUM_RST_ERR = -1 };

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
 *  p_flerr   - a pointer to the file_err RPC struct to store file & error info.
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
  if (reset_err_inf_NEW(&p_flerr->err) != 0) {
    // NOTE: just a workaround - return a special value if an error has occurred 
    // while resetting the error info; but maybe another solution should be used here
    p_flerr->err.num = ERRNUM_RST_ERR;
    return p_flerr->err.num;
  }

  // Reset the file name & type 
  if (reset_file_name_type_NEW(&p_flerr->file) != 0) {
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
    // Copy the selected file name into the file info object
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
      // Get the directory content and save it to file_err object
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
 * The following functions are always client-side.
 */
// TODO: move the following functions to the separate files client/interact.h and client/interact.c

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
  file_err flerr; // a local struct object to store&pass the information about a file&error

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
  
  if (DBG_PATH) 
    printf("[get_filename_inter] 1, path_curr: '%s'\n\toffset: %d\n", path_curr, offset);

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
              flerr.err.num != ERRNUM_RST_ERR 
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

    if (DBG_PATH)
      printf("[get_filename_inter] 3, path_curr + offset(%i): '%s'\n\tfname_inp: '%s', pfname_inp: '%s'\n",
             offset, path_curr + offset - 2, fname_inp, pfname_inp);

    // Construct the full path of the selected file
    nwrt_fname = construct_full_path(pfname_inp, LEN_PATH_MAX - offset, path_curr + offset);
    if (nwrt_fname <= 0) return NULL;
    
    // Increment the offset by the number of chars written to path_curr (after completed checks)
    offset += nwrt_fname;

    if (DBG_PATH)
      printf("[get_filename_inter] 4, path_curr: '%s'\n\tnwrt_fname: %d,  offset: %d\n", 
             path_curr, nwrt_fname, offset);
  }

  // TODO: maybe need to free the file name & content memory - free_file_inf_NEW(), at least
  // if file selection performs on client.
  // TODO: think what to do if file selection performs on server.
  return NULL;
}

#if DBG_MAIN == 1
// DELETE: it's just for testing
// Compilation: gcc -I/usr/include/tirpc fs_opers.c
int main()
{
  // Select the source file
  if (!get_filename_inter(".", filename_src, sel_ftype_source))
    return 1;
  printf("!!! Source file has been selected:\n'%s'\n", filename_src);

  // Select the target file
  if (!get_filename_inter(".", filename_trg, sel_ftype_target))
    return 2;
  printf("!!! Target file has been selected:\n'%s'\n", filename_trg);
  
  return 0;
}
#endif