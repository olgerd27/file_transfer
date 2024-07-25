/*
 * fltr.x: The file transfer protocol
 */

const LEN_PATH_MAX = 4096; /* max length for file names, equal to standard PATH_MAX */
const LEN_ERRMSG_MAX = 512; /* max length for error messages */

typedef string t_flname<LEN_PATH_MAX>; /* file name type */
typedef opaque t_flcont<>; /* file content type */

/* File type enumeration */
enum filetype {
  FTYPE_DFL,  /* default file type */
  FTYPE_REG,  /* regular file */
  FTYPE_DIR,  /* directory */
  FTYPE_OTH,  /* any other file type like link, socket, etc. */
  FTYPE_NEX,  /* specified pathname does not exist */
  FTYPE_INV   /* invalid file type */
  /* Add more file types as needed */
};

/* File information */
struct file_inf {
  t_flname name; /* file name */
  filetype type; /* file type */
  t_flcont cont; /* file content */
};

/* Error information */
union err_inf switch (int num) {
  case 0:
    void; /* no error - no message */
  default:
    string msg<LEN_ERRMSG_MAX>; /* error occured: return error message */
};

/* File & error info */
struct file_err {
  file_inf file; /* file info */
  err_inf err; /* error info */
};

/* The file transfer program definition */
program FLTRPROG {
   version FLTRVERS {
     err_inf upload_file(file_inf fileinf) = 1;
     file_err download_file(t_flname filename) = 2;
     file_err pick_entity(t_flname filename) = 3;
   } = 1;
} = 0x20000027;
