#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "fs_opers.h"

#define LEN_PATH_MAX 1024
#define LEN_FNAME_MAX 255

// Global system variable that store the error number
extern int errno;
static char *filename_src; // TODO: delete, it's just for test

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

// Return the filetype (defined in fltr.h) for the specified file
// filepath - a full path to the file.
enum filetype file_type(const char *filepath)
{
  printf("[file_type] 0, filepath: '%s'\n", filepath);
  enum filetype ftype;
  struct stat statbuf;

  if (stat(filepath, &statbuf) == -1)
    return FTYPE_INV;

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
  printf("[file_type] $, filetype: %d\n", (int)ftype);
  return ftype;
}

// List the directory content.
// dirpath - a full path to the directory.
int ls_dir(const char *dirpath)
{
  DIR            *hdir;
  struct dirent  *dp;
  struct stat     statbuf;
  char            strperm[11];
  struct passwd  *pwd;
  struct group   *grp;
  struct tm      *tm;
  char            datestring[256];
  char            fullpath[LEN_PATH_MAX]; // TODO: replace with PATH_MAX here and through the function

  // Open the current working directory
  if ( (hdir = opendir(dirpath)) == NULL ) {
    fprintf(stderr, "Cannot open directory %s\n%s\n", dirpath, strerror(errno));
    return 2;
  }

  /* Loop through directory entries. */
  while ((dp = readdir(hdir)) != NULL) {

    /* Construct full path to the directory entry */
    // TODO: skip the dirpath part of fullpath, since it's the constant part of string
    // and we don't need to delete and print it each time in the loop
    memset(fullpath, 0, LEN_PATH_MAX);
    snprintf(fullpath, LEN_PATH_MAX, "%s/%s", dirpath, dp->d_name);

    /* Get entry's information. */
    if (lstat(fullpath, &statbuf) == -1)
        continue;

    /* Print out type and permissions. */
    printf("%s", str_perm(statbuf.st_mode, strperm));

    /* Print out owner's name if it is found using getpwuid(). */
    // TODO: determine the length of the field as a max length among all the entries
    // And then the following format can be used: "  %-Ns"
    if ((pwd = getpwuid(statbuf.st_uid)) != NULL)
        printf("  %-8.8s", pwd->pw_name);
    else
        printf("  %-8d", statbuf.st_uid);

    /* Print out group name if it is found using getgrgid(). */
    if ((grp = getgrgid(statbuf.st_gid)) != NULL)
        printf(" %-8.8s", grp->gr_name);
    else
        printf(" %-8d", statbuf.st_gid);

    /* Print size of file. */
    printf(" %9jd", (intmax_t)statbuf.st_size);

    tm = localtime(&statbuf.st_mtime);

    /* Get localized date string. */
    // TODO: the ls command has 2 different formats for this date&time string:
    // - if a mod. time is less than 1 year then it's used the format with time and without year,
    // - if a mod. time is more than 1 year then it's used the format without time but with year.
    // I suppose a similar approach can be used here. 
    // Maybe used difftime() function to determine format.
    strftime(datestring, sizeof(datestring), "%b %d %R %Y", tm);

    printf(" %s %s\n", datestring, dp->d_name);
  }

  closedir(hdir);
  return 0;
}

// Get the filename in the interactive mode
char * get_filename_inter(const char *dir_start)
{
  // 1. ls dir
  // 2. get user input
  // 3. add user input to the current path
  // 3. check the input:
  //    3.1 if it's a dir, go to p.1.
  //    3.2 if it's not a dir, do the following:
  //        >> for source file:
  //        - if it's a regular file - OK and return its full path+name
  //        - if it does NOT a regular file - error and message like please choose a regular file
  //        - if it does NOT exist - error and message like the file ... doesn't exist
  //        >> for target file:
  //        - if it does NOT exist - OK and return its full path+name
  //        - if it exists (type doesn't matter) - error and message like file exists, please specify another one
  char filename[LEN_FNAME_MAX]; // a file name without path
//  char fullpath[PATH_MAX]; // a file full path+name, PATH_MAX is taken from limits.h
  char *fullpath = malloc(PATH_MAX); // a file full path+name, PATH_MAX is taken from limits.h
  int offset = 0; // an offset from the beginning of fullpath to control the added strings
  int nch_wrt = 0; // a number of written characters in fullpath

  // init the full path and offset
  offset = strlen(dir_start);
  strncpy(fullpath, dir_start, offset + 1); // use offset here as it means the copied string length
  printf("[get_filename_inter] 0, fullpath: '%s'\n", fullpath);

  while (file_type(fullpath) == FTYPE_DIR) {

    // Print the directory content
    if (ls_dir(fullpath) != 0)
      return NULL; // TODO: check the return value, maybe it should be some other one

    // Get user choice
    printf("\n>>> ");
    if (fgets(filename, LEN_FNAME_MAX, stdin) == NULL)
      return NULL; // TODO: check the return value, maybe it should be some other one
    printf("\n");
    printf("[get_filename_inter] 1, filename: '%s'\n", filename);

    // TODO: verify inputted filename, if len==0, print warning and continue

    // Removing trailing newline character from fgets() input
    filename[strcspn(filename, "\n")] = '\0';

    // Construct the full path+name of the choosed file
    nch_wrt = snprintf(fullpath + offset, PATH_MAX - offset, "/%s", filename);
    printf("[get_filename_inter] 2, fullpath: '%s'\n", fullpath);

    // Verify the fullpath construction
    if (nch_wrt < 0) {
      fprintf(stderr, "Invalid filename: '%s'\n", filename);
      return NULL; // TODO: check the return value, maybe it should be some other one
    }
    else offset += nch_wrt;
  }
  return fullpath;
}

// TODO: delete, it's just for tests
int main()
{
  filename_src = get_filename_inter(".");
  printf("Choosed file: '%s'\n", filename_src);
  free(filename_src);
  return 0;
}
