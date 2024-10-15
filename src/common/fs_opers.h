#ifndef _FS_OPERS_H_
#define _FS_OPERS_H_

#include <stdio.h>

/*
 * Forward definitions for the types defined in the RPC protocol. The actual types
 * will be available in source file after inclusion the RPC protocol header file.
 */
typedef enum filetype filetype;
typedef struct picked_file picked_file;
typedef struct file_err file_err;

/*
 * A special error number if an error occurred while resetting the error info.
 * Used as workaround when the error info intstance cannot be created but
 * you need to return smth pointed out on this issue.
 */
enum { ERRNUM_ERRINF_ERR = -1 };

/* Return the file type for the specified file.
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
enum filetype get_file_type(const char *filepath);

/* Get the file size in bytes.
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
size_t get_file_size(FILE *hfile);

/* Copy a source path to a target path with a maximum length.
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
int copy_path(const char *path_src, char *path_trg);

/* Select a file: determine its type and get its full (absolute) path.
 *
 * This function determines the type of the specified file and converts its
 * relative path to an absolute path.
 * It initializes the error info, file name & type, and file content buffer
 * before performing these operations. The results, including any errors, are stored
 * in the returned instance of file_err structure.
 *
 * Parameters:
 * p_flpicked - a pointer to a `picked_file` structure containing the file path and file type
 *              (source or target).
 *
 * Return:
 *  A pointer to a `file_err` structure containing the file information and any error details.
 *
 * Return value of `file_err.err.num`:
 *  0 on success,
 *  non-zero on failure.
 * Note: Errors in this function can be related to the file system or other causes.
 *       To differentiate, check the `file.type` field of the returned `file_err` object.
 *       If `file.type == FTYPE_DFL`, the issue is not file system related.
 *       Any other value from the `filetype` enum indicates a file system-related error.
 * - For source file selection, only regular files can be selected.
 * - For target file selection, only non-existent files are valid.
 */
file_err * select_file(picked_file *p_flpicked);

#endif