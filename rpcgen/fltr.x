/*
 * fltr.x: The file transfer protocol
 */

const SIZE_FNAME = 256; /* max length for file name */
const SIZE_ERRMSG = 512; /* max length for error message */

typedef string t_flname<SIZE_FNAME>; /* file name type */
typedef opaque t_flcont<>; /* file content type */

/* File information */
struct file_inf {
  t_flname name; /* file name */
  t_flcont cont; /* file content */
};

/* Error information */
union err_inf switch (int num) {
  case 0:
    void; /* no error - no message */
  default:
    string msg<SIZE_ERRMSG>; /* error occured: return error message */
};

/* File + error info */
struct file_err {
  file_inf file; /* file info */
  err_inf err; /* error info */
};

/* The file transfer program definition */
program FLTRPROG {
   version FLTRVERS {
     err_inf upload_file(file_inf fileinf) = 1;
     file_err download_file(t_flname filename) = 2;
   } = 1;
} = 0x20000027;
