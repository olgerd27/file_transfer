#ifndef _MEM_OPERS_H_
#define _MEM_OPERS_H_

#include "../rpcgen/fltr.h"

/* The memory manipulation operations */

/*
 * Allocate memory to store the file content.
 *
 * This function allocates memory to store file content if it hasn't already been
 * allocated. If memory has already been allocated, it returns a pointer to the
 * existing memory.
 *
 * Parameters:
 * p_flcont - A pointer to a file content instance where the allocated memory will be stored.
 * size     - The size of the memory to allocate.
 * p_err    - A pointer to an error info instance where error info will be written in case of an error.
 *
 * Return value:
 *  A pointer to the allocated memory, or NULL in case of an error.
 */
char *alloc_file_cont(t_flcont *p_flcont, size_t size);

/*
 * Free the file name memory.
 * This function frees the memory allocated for a file name string.
 *
 * Parameters:
 *  p_flname - A pointer to a file name string whose memory is to be freed.
 */
void free_file_name(t_flname *p_flname);

/*
 * Free the file content memory.
 * This function frees the memory allocated for a file content instance.
 *
 * Parameters:
 *  p_flcont - A pointer to a file content instance whose memory is to be freed.
 */
void free_file_cont(t_flcont *p_flcont);

/*
 * Free the file info memory.
 * This function frees the memory allocated for a file info instance.
 *
 * Parameters:
 *  p_file - A pointer to a file info instance whose memory is to be freed.
 */
void free_file_inf(file_inf *p_file);

/*
 * Free the error info memory.
 * This function frees the memory allocated for an error info instance.
 *
 * Parameters:
 *  p_err - A pointer to an error info instance whose memory is to be freed.
 */
void free_err_inf(err_inf *p_err);

/*
 * Reset (init) the file name & type.
 *
 * The file name's memory size is constant (LEN_PATH_MAX) and it's allocated just once
 * if it is currently unallocated.
 * If name is NULL -> memory allocation occurs, if name is NOT NULL -> set memory to 0.
 * Memory deallocation must be done separately outside of this function.
 *
 * Any errors encountered in this function are printed to stderr on the same side where they occur,
 * as these details should not be of interest to the other side. The caller of this function
 * can generate a more general error message and communicate it to the other side
 * through the error info instance.
 *
 * Note: This function does not populate the error info struct instance to convey errors
 * to the other side.
 *
 * Parameters:
 *  p_file      - A pointer to a file info struct instance to reset.
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int reset_file_name_type(file_inf *p_file);

/*
 * Reset (init) the file content.
 *
 * Memory deallocation and allocation of new one with the size_fcont size.
 * Due to its variable size, the file contents are reset by reallocating memory
 * rather than setting it to 0.
 * Memory deallocation must be done separately outside of this function.
 *
 * Parameters:
 *  p_flcont    - A pointer to a file content instance to reset.
 *  size_fcont  - The size of the memory to allocate for the file content.
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int reset_file_cont(t_flcont *p_flcont, size_t size_fcont);

/*
 * Reset (init) the file info.
 *
 * This function resets (initializes) the file information, including the file name, type, and content.
 * It allocates new memory for the file content based on the specified size.
 *
 * Parameters:
 *  p_file      - A pointer to a file info instance to reset.
 *  size_fcont  - The size of the memory to allocate for the file content.
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int reset_file_inf_NEW(file_inf *p_file, size_t size_fcont);

/*
 * Reset (init) the error info.
 *
 * This function resets (initializes) an error info instance by allocating memory for the error
 * message if it is not already allocated and setting the error number and system
 * error number to 0. The error message is reset if the error number is set.
 *
 * Parameters:
 *  p_err - A pointer to an error info instance to reset.
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int reset_err_inf(err_inf *p_err);

#endif