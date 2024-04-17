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

#define PATHLEN_MAX 512

// Global system variable that store the error number
extern int errno;

// Return a letter of a file type used in Unix-like OS
/*static*/ char get_file_type(mode_t mode)
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
/*static*/ char * str_perm(mode_t mode, char *strmode)
{
  strmode[0] = get_file_type(mode);
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

// List the directory content.
int ls_dir(const char *dirname)
{
  DIR            *hdir;
  struct dirent  *dp;
  struct stat     statbuf;
  char            strperm[11];
  struct passwd  *pwd;
  struct group   *grp;
  struct tm      *tm;
  char            datestring[256];
  char            fullpath[PATHLEN_MAX];

  // Open the current working directory
  if ( (hdir = opendir(dirname)) == NULL ) {
    fprintf(stderr, "Cannot open directory %s\n%s\n", dirname, strerror(errno));
    return 2;
  }

  /* Loop through directory entries. */
  while ((dp = readdir(hdir)) != NULL) {

    /* Construct full path to the directory entry */
    memset(fullpath, 0, PATHLEN_MAX);
    snprintf(fullpath, PATHLEN_MAX, "%s/%s", dirname, dp->d_name);

    /* Get entry's information. */
    if (lstat(fullpath, &statbuf) == -1)
        continue;

    /* Print out type and permissions. */
    printf("%s", str_perm(statbuf.st_mode, strperm));

    /* Print out owner's name if it is found using getpwuid(). */
    // TODO: determine the length of the field as a max length among all the entries
    // And then the following dormat can be used: "  %-Ns"
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

