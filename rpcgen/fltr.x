/*
 * fltr.x: The file transfer protocol
 */

typedef opaque t_file_cont<>; /* the file content type */

struct file {
  string name<>; /* the file name */
  t_file_cont content; /* the file content */
};

typedef string errmsg<>; /* an error message occured on the server */

/* The file transfer program definition */
program FLTRPROG {
   version FLTRVERS {
     int upload_file(file fl) = 1;
     t_file_cont download_file(string filename) = 1;
     errmsg get_error_msg() = 1;
   } = 1;
} = 0x20000027;
