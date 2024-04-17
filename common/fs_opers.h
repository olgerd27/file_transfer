#ifndef _FS_OPERS_H_
#define _FS_OPERS_H_

/*
 * Return a letter of a file type used in Unix-like OS:
 * 'd' -> directory
 * '-' -> regular file
 * 'b' -> block file
 * 'c' -> character device file
 * 'p' -> named pipe file
 * 'l' -> symbolic link file (if lstat() was used)
 * 's' -> socket file
 * '?' -> unknown type of file
 */
char get_file_type(mode_t mode);

/*
 * Convert permissions from the digit to the symbol form.
 * The function adds a file type at the 1st place, like same as 'ls -l' command does.
 */
char * str_perm(mode_t mode, char *strmode);

/*
 * List the directory content.
 * RC: 0 - success, >0 - failure
 */
int ls_dir(const char *dirname);

#endif
