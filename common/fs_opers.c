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
#include "mem_opers.h"

#define DBG_FT 0    // debug the file type info
#define DBG_PATH 0  // debug the path's manipulation

extern int errno; // global system error number

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
 * Get the file size in bytes.
 *
 * This function calculates the size of a file by seeking to the end of the file,
 * retrieving the position (which represents the file size in bytes), and then
 * rewinding to the beginning of the file.
 *
 * Parameters:
 *  hfile - A pointer to the FILE object representing the open file.
 * 
 * Return value:
 *  The size of the file in bytes.
 */
size_t get_file_size(FILE *hfile)
{
  fseek(hfile, 0, SEEK_END);
  unsigned size = ftell(hfile);
  rewind(hfile);
  return size;
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
static int get_file_stat(const char *dirname, const char *filename, struct stat *p_statbuf, char **errmsg)
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
static struct lsdir_setts
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
static int numb_digits(long val)
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
static void update_lsdir_setts(struct stat *p_statbuf, const char *filename, struct lsdir_setts *p_lsd_set)
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
static size_t calc_dir_cont_size(const struct lsdir_setts *p_lsd_set)
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
 * List the directory content into the file content (char array) stored in the file_err instance.
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
 *  p_flerr - Pointer to an allocated and nulled RPC struct instance to store file & error info.
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
      // again and placed in the p_flerr->err instance during the next loop through the directories.
      // printf("%s", errmsg);
      free(errmsg); // free allocated memory for returned error message
    }
  }

  // Reset the file content before filling it with directory listing data
  if (reset_file_cont(&p_flerr->file.cont, calc_dir_cont_size(&lsdir_set)) != 0) {
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

/*
 * Select a file: determine its type and get its full (absolute) path.
 *
 * This function determines the type of the specified file and converts its
 * relative path to an absolute path.
 * It resets the error info, file name & type, and also the file content buffer in ls_dir_str() 
 * before performing these operations. The results, including any errors, are stored 
 * in the provided file_err structure.
 *
 * Parameters:
 *  path    - a path of the file that needs to be selected.
 *  p_flerr - a pointer to the file_err RPC struct instance to store file & error info.
 *            This struct is used to set and return the result through the function argument.
 *  pftype  - an enum value of type pick_ftype indicating whether the file to be selected
 *            is a source or target file.
 *
 * Return value:
 *  RC: 0 on success, >0 on failure.
 */
// TODO: update the documentation for this function
file_err * select_file(picked_file *p_flpicked)
{
  static file_err flerr;

  // Reset the error info before file selection
  if (reset_err_inf(&flerr.err) != 0) {
    // A workaround: set a special value if an error has occurred while resetting the error info
    flerr.err.num = ERRNUM_ERRINF_ERR;
    return &flerr;
  }

  // Reset the file name & type 
  if (reset_file_name_type(&flerr.file) != 0) {
    flerr.err.num = 81;
    sprintf(flerr.err.err_inf_u.msg,
            "Error %i: Failed to reset the file name & type\n", flerr.err.num);
    return &flerr;
  }

  // Determine the file type
  flerr.file.type = get_file_type(p_flpicked->name);

  // Process the case of non-existent file required for the target file.
  // Should be processed before conversion p_flpicked->name to the absolute one.
  if (flerr.file.type == FTYPE_NEX) {
    // Copy the selected file name into the file info instance
    // TODO: delete a line with strncpy() after final acception of the line with copy_path()
    // strncpy(flerr.file.name, p_flpicked->name, strlen(p_flpicked->name) + 1);
    copy_path(p_flpicked->name, flerr.file.name);

    // Check the correctness of the current file selection based on the selection file type
    if (p_flpicked->pftype == pk_ftype_target)
      ; // OK: the non-existent file type was selected as expected for a 'target' selection file type
    else if (p_flpicked->pftype == pk_ftype_source) {
      // Fail: the source file selection (a regular file) was expected, but the target file selection
      // (a non-existent file) was actually attempted -> produce the error
      flerr.err.num = 82;
      sprintf(flerr.err.err_inf_u.msg,
              "Error %i: The selected file does not exist:\n'%s'\n"
              "Only the regular file can be selected as the source file.\n",
              flerr.err.num, flerr.file.name);
    }
    return &flerr;
  }

  // Convert the passed path into the full (absolute) path - needed to any type of existent file
  char *errmsg = NULL;
  if (!rel_to_full_path(p_flpicked->name, flerr.file.name, &errmsg)) {
    flerr.err.num = 80;
    sprintf(flerr.err.err_inf_u.msg, "Error %i: %s\n", flerr.err.num, errmsg);
    free(errmsg); // free allocated memory
    return &flerr;
  }

  // Process the cases of existent file
  switch (flerr.file.type) {
    case FTYPE_DIR: /* directory */
      // Get the directory content and save it to file_err instance
      // If error has occurred it sets to flerr, no need to check the RC
      (void)ls_dir_str(&flerr);
      break;

    case FTYPE_REG: /* regular file */
      // Check the correctness of the current file selection based on the selection file type
      if (p_flpicked->pftype == pk_ftype_source)
        ; // OK: the regular file type was selected as expected for a 'source' selection file type
      else if (p_flpicked->pftype == pk_ftype_target) {
        // Fail: the target file selection (a non-existent file) was expected, but the source file selection
        // (a regular file) was actually attempted -> produce the error
        flerr.err.num = 83;
        sprintf(flerr.err.err_inf_u.msg,
                "Error %i: The wrong file type was selected - regular file:\n'%s'\n"
                "Only the non-existent file can be selected as the target file.\n",
                flerr.err.num, flerr.file.name);
      }
      break;

    case FTYPE_OTH: /* any other file type like link, socket, etc. */
      flerr.err.num = 84;
      sprintf(flerr.err.err_inf_u.msg,
              "Error %i: Unsupported file type was selected (other):\n'%s'\n",
              flerr.err.num, flerr.file.name);
      break;

    case FTYPE_INV: /* invalid file */
      // NOTE: If the rel_to_full_path() call above fails, this case will never be reached.
      flerr.err.num = 85;
      sprintf(flerr.err.err_inf_u.msg,
              "Error %i: Invalid file was selected:\n'%s'\n%s\n",
              flerr.err.num, flerr.file.name, strerror(errno));
      break;
  }
  return &flerr;
}
