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

/*
 * Return the file type for the specified file.
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

/*
 * Get the file size in bytes.
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

/*
 * Convert the passed relative path to the full (absolute) one.
 *
 * This function converts a relative path to an absolute path. If the conversion
 * fails, it allocates memory for an error message and sets it in the provided errmsg pointer.
 *
 * Parameters:
 *  path_rel  - A relative path to be converted.
 *  path_full - A buffer to store the resulting absolute path.
 *  errmsg    - A pointer to a char pointer where the error message will be stored
 *              if the conversion fails.
 *
 * Return value:
 *  The full (absolute) path on success (same as path_full), or NULL on failure with 
 *  the error messages in errmsg which should be freed by a caller of this function.
 */
char *rel_to_full_path(const char *path_rel, char *path_full, char **errmsg);

/*
 * Copy a source path to a target path with a maximum length.
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

/*
 * List the directory content into the file content (char array) stored in the file_err instance.
 *
 * This function reads the contents of a specified directory and stores the directory entries
 * in the file content field of the passed file_err structure. The function assumes that the
 * file name in the passed struct specifies the directory to be read and that the file type 
 * is a directory.
 *
 * Errors encountered during these operations are stored in the error info field of 
 * the file_err structure.
 *
 * The function performs the following steps:
 * 1. Opens the specified directory.
 * 2. Iterates through the directory entries to gather settings for flexible listing.
 * 3. Resets the file content buffer based on the calculated size of the directory content.
 * 4. Iterates through the directory entries again to populate the file content buffer 
 *    with the directory listing.
 *
 * Parameters:
 *  p_flerr - Pointer to an allocated and nulled RPC struct instance to store file & error info.
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int ls_dir_str(file_err *p_flerr);

// TODO: update the documentation
/*
 * Select a file: determine its type and get its full (absolute) path.
 *
 * This function determines the type of the specified file and converts its
 * relative path to an absolute path. It resets the error info before performing
 * these operations. The results, including any errors, are stored in the provided
 * file_err structure.
 *
 * Parameters:
 *  path    - a path of the file that needs to be selected.
 *  p_flerr - a pointer to the file_err RPC struct instance to store file & error info.
 *            This struct is used to set and return the result through the function argument.
 *  pftype  - an enum value of type pick_ftype indicating whether the file to be selected
 *            is a source or target file.
 *
 * Return value:
 *  RC: 0 on success, >0 on failure.
 */
file_err * select_file(picked_file *p_flpicked);

#endif