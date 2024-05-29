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
 * List the directory content.
 * - dirname -> the directory name
 * RC:  0 on success, >0 on failure.
 */
int ls_dir(const char *dirname)
{
  DIR            *hdir;
  struct dirent  *de; // directory entry
  struct stat     statbuf;
  char            strperm[11];
  struct passwd  *pwd;
  struct group   *grp;
  struct tm      *tm;
  char            datestring[256];
  char            fullpath[LEN_PATH_MAX];
  int             offset_dn; // offset from the beginning in fullpath for the constant string dirname+'/'
  int             nwrt_fname; // number of characters written to the fullpath in each iteration

  // Open the passed directory
  if ( (hdir = opendir(dirname)) == NULL ) {
//    fprintf(stderr, "Cannot open directory '%s'\n%s\n", dirname, strerror(errno));
    perror("Failed to open the specified directory");
    return 1;
  }

  // Init the full path with the first constant part dirname+'/'
  offset_dn = snprintf(fullpath, LEN_PATH_MAX, "%s/", dirname); // '\0' appends automatically
  if (offset_dn < 0) {
    fprintf(stderr, "Invalid dirname: '%s'\n", dirname);
    closedir(hdir);
    return 2;
  }

  /* Loop through directory entries */
  while ((de = readdir(hdir)) != NULL) {

    // Construct the full path to the directory entry
    // A NULL-character appends to fullpath automatically
    nwrt_fname = snprintf(fullpath + offset_dn, LEN_PATH_MAX - offset_dn, "%s", de->d_name);
    if (nwrt_fname < 0) {
      fprintf(stderr, "Invalid filename: '%s'\n", de->d_name);
      continue;
    }

    // Get entry's information
    if (lstat(fullpath, &statbuf) == -1)
        continue;

    // Print out type and permissions
    printf("%s", str_perm(statbuf.st_mode, strperm));

    // Print out owner's name if it is found using getpwuid()
    // TODO: determine the length of the field as a max length among all the entries
    // And then the following format can be used: "  %-Ns"
    if ((pwd = getpwuid(statbuf.st_uid)) != NULL)
        printf("  %-8.8s", pwd->pw_name);
    else
        printf("  %-8d", statbuf.st_uid);

    // Print out group name if it is found using getgrgid()
    if ((grp = getgrgid(statbuf.st_gid)) != NULL)
        printf(" %-8.8s", grp->gr_name);
    else
        printf(" %-8d", statbuf.st_gid);

    // Print size of file
    printf(" %9jd", (intmax_t)statbuf.st_size);

    tm = localtime(&statbuf.st_mtime);

    // Get localized date string
    // TODO: the ls command has 2 different formats for this date&time string:
    // - if a mod. time is less than 1 year then it's used the format with time and without year,
    // - if a mod. time is more than 1 year then it's used the format without time but with year.
    // I suppose a similar approach can be used here. 
    // Maybe use difftime() function to determine the format.
    strftime(datestring, sizeof(datestring), "%b %d %R %Y", tm);

    printf(" %s %s\n", datestring, de->d_name);
  }

  closedir(hdir);
  return 0;
}

/*
 * Get user input of filename.
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
// TODO: move these versions to the new files like common/memops.c, 
 * and use them everywhere where they're required.
 */
/*
 * Allocate the memory to store the file content.
 * Allocate only if it has not already done, else - return a pointer to the existing memory.
 * - p_flcont: pointer to a file content object to store the allocated memory
 * - size: the size of the memory to allocate
 * - p_err: pointer to an error info object, where the error info will be written in case of error.
 * RC: returns a pointer to the allocated memory or NULL in case of error.
// TODO: check if the error number is ok in this function; this function should be used here and in both client and server code.
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

  printf("[reset_file_inf_NEW] file info reset DONE\n");
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

  printf("[reset_err_inf_NEW] error info reset DONE\n");
  return 0;
}

/*
 * List the directory content into the char array stored in file_err struct.
 * - dirname: the directory name
 * - p_flerr: pointer to an allocated and nulled RPC struct to store file & error info;
 *   is used here to set and return of the result through the function argument.
 * RC: 0 on success, >0 on failure.
 */
int ls_dir_str(const char *dirname, file_err *p_flerr)
{
  DIR           *hdir;
  struct dirent *de; // directory entry
  struct stat    statbuf;
  char           strperm[11];
  struct passwd *pwd;
  struct group  *grp;
  struct tm     *tm;
  char           datestring[256];
  char           fullpath[LEN_PATH_MAX];
  int            offset_dn; // offset from the beginning of fullpath for the constant string: dirname+'/'

  char *p_errmsg = p_flerr->err.err_inf_u.msg; // pointer to the error message
  char *p_flcont = p_flerr->file.cont.t_flcont_val; // pointer to the file content data

  // Open the passed directory
  if ( (hdir = opendir(dirname)) == NULL ) {
    p_flerr->err.num = 85;
    sprintf(p_errmsg, "Error %i: Cannot open directory '%s'\n%s\n",
                      p_flerr->err.num, dirname, strerror(errno));
    return p_flerr->err.num;
  }

  // Init the full path with the first constant part dirname+'/'
  offset_dn = snprintf(fullpath, LEN_PATH_MAX, "%s/", dirname); // '\0' appends automatically
  if (offset_dn < 0) {
    closedir(hdir);
    p_flerr->err.num = 86;
    sprintf(p_errmsg, "Error %i: Invalid dirname: '%s'\n", p_flerr->err.num, dirname);
    return p_flerr->err.num;
  }

  /* Loop through directory entries */
  while ((de = readdir(hdir)) != NULL) {

    // Construct the full path to the directory entry
    // A NULL-character appends to fullpath automatically
    if ( snprintf(fullpath + offset_dn, LEN_PATH_MAX - offset_dn, "%s", de->d_name) < 0 ) {
      p_flcont += sprintf(p_flcont, "Invalid filename: '%s'\n", de->d_name);
      continue;
    }

    // Get entry's information
    if (lstat(fullpath, &statbuf) == -1)
        continue;

    // Print out type and permissions
    p_flcont += sprintf(p_flcont, "%s", str_perm(statbuf.st_mode, strperm));

    // Print out owner's name if it is found using getpwuid()
    // TODO: determine the length of the field as a max length among all the entries
    // And then the following format can be used: "  %-Ns"
    if ((pwd = getpwuid(statbuf.st_uid)) != NULL)
        p_flcont += sprintf(p_flcont, "  %-8.8s", pwd->pw_name);
    else
        p_flcont += sprintf(p_flcont, "  %-8d", statbuf.st_uid);

    // Print out group name if it is found using getgrgid()
    if ((grp = getgrgid(statbuf.st_gid)) != NULL)
        p_flcont += sprintf(p_flcont, " %-8.8s", grp->gr_name);
    else
        p_flcont += sprintf(p_flcont, " %-8d", statbuf.st_gid);

    // Print size of file
    p_flcont += sprintf(p_flcont, " %9jd", (intmax_t)statbuf.st_size);

    tm = localtime(&statbuf.st_mtime);

    // Get localized date string
    // TODO: the ls command has 2 different formats for this date&time string:
    // - if a mod.time is less than 1 year then it's used the format with time and without year,
    // - if a mod.time is more than 1 year then it's used the format without time but with year.
    // I suppose a similar approach can be used here. 
    // Maybe use difftime() function to determine the format.
    strftime(datestring, sizeof(datestring), "%b %d %R %Y", tm);

    p_flcont += sprintf(p_flcont, " %s %s\n", datestring, de->d_name);
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

  // Process the file type
  switch (p_flerr->file.type) {
    case FTYPE_DIR:
    case FTYPE_REG:
      // Convert the passed path to the full path
      // TODO: think, maybe create a separate function to convert relative path to the full one, and then
      // call it for FTYPE_OTH and FTYPE_INV cases? Just to have a clear path in the error messages for these
      // kinds of file types.
      if (!realpath(path, p_flerr->file.name)) {
        p_flerr->err.num = 80;
        sprintf(p_flerr->err.err_inf_u.msg,
                "Error %i: Failed to resolve the specified path: '%s'\n%s",
                p_flerr->err.num, path, strerror(errno));
        break; // leave switch statement
      }
      // If it's a directory, get its content and save it to file_err object
      // TODO: think, maybe pass a full path to the ls_dir_str() function (just p_flerr)?
      // Because if error occurs in ls_dir_str() the failed relative path printed to user in error message looks unclear.
      if (p_flerr->file.type == FTYPE_DIR)
        ls_dir_str(path, p_flerr); // if error has occurred it was set to p_flerr, so no need to check RC
      break;
    case FTYPE_OTH:
      p_flerr->err.num = 81;
      sprintf(p_flerr->err.err_inf_u.msg,
              "Error %i: Invalid file: '%s'\n"
              "Only regular file can be chosen", p_flerr->err.num, path);
      break;
    case FTYPE_NEX:
    case FTYPE_INV:
      p_flerr->err.num = 82;
      sprintf(p_flerr->err.err_inf_u.msg,
              "Error %i: Invalid file: '%s'\n%s",
              p_flerr->err.num, path, strerror(errno));
      break;
  }
  return p_flerr->err.num;
}

/*
 * Get the filename in the interactive mode.
 * - dir_start -> a starting directory for the traversal
 * - path_res  -> an allocated char array to store the resulting file path
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
  char fname_inp[NAME_MAX]; // inputted filename (without a path)
  int len_fname_inp; // length of the inputted filename
  int offset; // offset from the beginning of the current path for the added subdir/file
  int offset_fullpath; // offset from the beginning of fname_inp; set 1 if a full path was inputted
  int nwrt_fname; // number of characters written to the current path in each iteration
  file_err flerr; // a local struct object to store&pass the information about a file&error

  // Init the full path and offset
  offset = strlen(dir_start); // define offset as the start dir length that will be copied into path_curr
  strncpy(path_curr, dir_start, offset + 1); // strncpy doesn't add/copy '\0', so add "+1" to copy it
  strcpy(path_prev, "/"); // init the previous path with a root dir as a guaranteed valid path
  // TODO: copy a home directory to the previous path instead of the root dir
  printf("[get_filename_inter] 0, path_curr: '%s'\n", path_curr);

//  while (file_type(path_curr) == FTYPE_DIR) {
  while (1) {

    // enum filetype ftype = file_type(path_curr);
    // printf("[get_filename_inter] 1, ftype: '%d'\n", ftype);
    // switch (ftype) {
    //   case FTYPE_DIR:
    //   case FTYPE_REG:
    //     // Convert the current path to the correct full (absolute) path
    //     if (!realpath(path_curr, path_res)) {
    //       perror("Failed to resolve the specified path");
    //       offset = copy_path(path_prev, path_curr); // restore the previous valid path
    //       break; // continue the loop
    //     }
    //     if (ftype == FTYPE_REG)
    //       return path_res; // return the selected file
    //     break; // continue the loop
    //   case FTYPE_OTH:
    //     fprintf(stderr, "Invalid file: Only the regular file can be chosen\n");
    //     offset = copy_path(path_prev, path_curr); // restore the previous valid path
    //     continue;
    //   case FTYPE_NEX:
    //   case FTYPE_INV:
    //     perror("Invalid file");
    //     offset = copy_path(path_prev, path_curr); // restore the previous valid path
    //     continue;
    // }

    // Reset the file info before each call of select_file()
    // TODO: determine, maybe move a call of the function to reset the file info in select_file()?
    // TODO: implement the dynamic memory reallocation for file_inf object 
    if (reset_file_inf_NEW(&flerr.file, 10000) != 0) {
      fprintf(stderr, "Failed to reset the file information\n");
      return NULL;
    }

    // Reset the error info before each call of select_file()
    // TODO: determine, maybe move a call of the function to reset the error info in select_file()?
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
      offset = copy_path(path_prev, path_curr); // restore the previous valid path
//      continue; // TODO: test if it's correct to call continue for all the data types,
                // and if yes - delete 2 lines below.
      printf("[get_filename_inter] 1, filetype=%i\n", (int)flerr.file.type);
      switch (flerr.file.type) {
        case FTYPE_DIR: case FTYPE_REG: break; // continue the current loop iteration
        case FTYPE_OTH:	case FTYPE_NEX:	case FTYPE_INV: continue; // start a new loop iteration
      }
    }
    
    /* At this point we're sure that the it's selected a valid directory */

    // Print the full path and content of the current directory
    printf("\n%s:\n%s\n", flerr.file.name, flerr.file.cont.t_flcont_val);

    // Print the directory content
    // if (ls_dir(path_curr) != 0) {
    //   offset = copy_path(path_prev, path_curr); // restore the previous valid path
    //   continue;
    // }

    // Get user input of filename
    if (input_filename(fname_inp) != 0)
      continue;
//    printf("[get_filename_inter] 1, fname_inp: '%s'\n", fname_inp);

    // Check if inputted fname_inp is empty (user just pressed ENTER)
    len_fname_inp = strlen(fname_inp);
    if (len_fname_inp == 0) continue;

    // Before changing the current path, set the previous path to the current valid path
    copy_path(path_curr, path_prev);

    // Settings for full and relative path
    if (*fname_inp == '/') {
      // If user specified a full path, reset path_curr
      memset(path_curr, 0, offset);
      offset = 0;
      offset_fullpath = 1; // set 1 for a full path to skip the first '/' symbol
    }
    else offset_fullpath = 0; // no need to skip symbols for relative pathes

    // Construct the full path of the choosed file
    nwrt_fname = snprintf(path_curr + offset, LEN_PATH_MAX - offset, "/%s", fname_inp + offset_fullpath);

    // Verify the path_curr construction
    // Check if nothing has written to a full path of the res filename
    if (nwrt_fname < 0) {
      fprintf(stderr, "Invalid filename: '%s'\n", fname_inp);
      return NULL;
    }

    // Check if the whole inputted filename was added to a full path of the res filename
    // Add '+1' as '/' was also written, and substruct offset_fullpath
    if (nwrt_fname != len_fname_inp + 1 - offset_fullpath) {
      fprintf(stderr, "Cannot append the inputted filename to the result filename\n");
      return NULL;
    }

   // Increment the offset by the number of chars written to path_curr (after completed checks)
   offset += nwrt_fname;

//    printf("[get_filename_inter] 2, path_curr: '%s'\n", path_curr);
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

