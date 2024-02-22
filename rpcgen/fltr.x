/*
 * fltr.x: The file transfer protocol
 */

const SIZE_FNAME = 256; /* max length for file name */
const SIZE_ERRMSG = 512; /* max length for error message */

typedef string t_flname<SIZE_FNAME>; /* file name type */
typedef opaque t_flcont<>; /* file content type */

/* File information */
struct file {
  t_flname name; /* file name */
  t_flcont cont; /* file content */
};

/* The file transfer program definition */
program FLTRPROG {
   version FLTRVERS {
     int upload_file(file fl) = 1;
     t_flcont download_file(t_flname filename) = 2;
   } = 1;
} = 0x20000027;
