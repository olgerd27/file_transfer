/*
 * fltr.x: The file transfer protocol
 */

const SIZE_FNAME = 255; /* max length of file name */

typedef string t_flname<SIZE_FNAME>; /* file name type */
typedef opaque t_flcont<>; /* file content type */
typedef string t_errmsg<>; /* error message type */

struct file {
  t_flname name; /* file name */
  t_flcont content; /* file content */
};

/* The file transfer program definition */
program FLTRPROG {
   version FLTRVERS {
     int upload_file(file fl) = 1;
     t_flcont download_file(t_flname filename) = 1;
     t_errmsg get_error_msg(void) = 1;
   } = 1;
} = 0x20000027;
