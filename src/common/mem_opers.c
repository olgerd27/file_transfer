/*
 * mem_opers.c: a set of functions for memory manipulations like allocate, free, reset.
 * Errors range: 31-45 (reserve 31-45)
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "mem_opers.h"
#include "fs_opers.h" // to access ERRNUM_ERRINF_ERR
#include "logging.h"

extern int errno; // global system error number

/* Allocate memory to store the file content.
 *
 * This function allocates memory to store file content if it hasn't already been
 * allocated. If memory has already been allocated, it returns a pointer to the
 * existing memory.
 *
 * Parameters:
 * p_flcont - A pointer to a file content instance where the allocated memory will be stored.
 * size     - The size of the memory to allocate.
 * 
 * Return value:
 *  A pointer to the allocated memory, or NULL in case of an error.
 */
char * alloc_file_cont(t_flcont *p_flcont, size_t size)
{
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Begin");
  
  // Check if the memory for the file content instance is allocated
  if (!p_flcont) {
    LOG(LOG_TYPE_MEM, LOG_LEVEL_ERROR, 
      "An invalid file content pointer has passed, p_flcont=%p", (void *)p_flcont);
    return NULL;
  }

  // Memory allocation 
  if (!p_flcont->t_flcont_val) {
    p_flcont->t_flcont_val = (char*)malloc(size * sizeof(char));
    if (!p_flcont->t_flcont_val) {
      p_flcont->t_flcont_len = 0;
      // NOTE: print this error only on this function caller side.
      // Because these are the details that another side should not be interested in.
      // A caller of this function can produce a more general error message and send it
      // to another side through the error info instance, which is not accessible here.
      LOG(LOG_TYPE_MEM, LOG_LEVEL_ERROR, 
          "Failed to allocate memory for file content, required size: %ld", size);
      return NULL;
    }
    p_flcont->t_flcont_val[0] = '\0'; // required to ensure strlen() works correctly on this memory
    p_flcont->t_flcont_len = size;
    LOG(LOG_TYPE_MEM, LOG_LEVEL_INFO, 
        "memory for file contents has been allocated, ptr=%p, size=%d", 
        p_flcont->t_flcont_val, p_flcont->t_flcont_len);
  }
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Done.");
  return p_flcont->t_flcont_val;
}

/* Free the file name memory.
 * This function frees the memory allocated for a file name string.
 *
 * Parameters:
 *  p_flname - A pointer to a file name string whose memory is to be freed.
 */
void free_file_name(t_flname *p_flname)
{
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Begin");
  if (p_flname) {
    LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "before freeing, ptr=%p", *p_flname);
    free(*p_flname);
    *p_flname = NULL;
    LOG(LOG_TYPE_MEM, LOG_LEVEL_INFO, "freeing completed");
  }
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Done.");
}

/* Free the file content memory.
 * This function frees the memory allocated for a file content instance.
 *
 * Parameters:
 *  p_flcont - A pointer to a file content instance whose memory is to be freed.
 */
void free_file_cont(t_flcont *p_flcont)
{
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Begin");
  if (p_flcont && p_flcont->t_flcont_val) {
    LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "before freeing, ptr=%p", p_flcont->t_flcont_val);
    free(p_flcont->t_flcont_val);
    p_flcont->t_flcont_val = NULL;
    p_flcont->t_flcont_len = 0;
    LOG(LOG_TYPE_MEM, LOG_LEVEL_INFO, "freeing completed");
  }
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Done.");
}

/* Free the file info memory.
 * This function frees the memory allocated for a file info instance.
 *
 * Parameters:
 *  p_file - A pointer to a file info instance whose memory is to be freed.
 */
void free_file_inf(file_inf *p_file)
{
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Begin");
  if (p_file) {
    free_file_name(&p_file->name); // free the file name memory
    free_file_cont(&p_file->cont); // free the file content memory
  }
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Done.");
}

/* Free the error info memory.
 * This function frees the memory allocated for an error info instance.
 *
 * Parameters:
 *  p_err - A pointer to an error info instance whose memory is to be freed.
 */
void free_err_inf(err_inf *p_err)
{
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Begin");
  if (p_err && p_err->err_inf_u.msg) {
    LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "before freeing, ptr=%p", p_err->err_inf_u.msg);
    free(p_err->err_inf_u.msg);
    p_err->err_inf_u.msg = NULL;
    p_err->num = 0;
    LOG(LOG_TYPE_MEM, LOG_LEVEL_INFO, "freeing completed");
  }
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Done.");
}

/* Reset (init) the file name & type.
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
int reset_file_name_type(file_inf *p_file)
{
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Begin");

  // Check if the memory for the file info struct instance is allocated
  if (!p_file) {
    LOG(LOG_TYPE_MEM, LOG_LEVEL_ERROR, 
      "An invalid file info pointer has passed, p_file=%p", (void *)p_file);
    return 1;
  }

  // Init the file name
  if (!p_file->name) {
    // Initial dynamic memory allocation.
    // It performs for the first call of this function or after memory freeing.
    // Deallocation is required later
    p_file->name = (char*)malloc(LEN_PATH_MAX * sizeof(char));
    if (!p_file->name) {
      LOG(LOG_TYPE_MEM, LOG_LEVEL_ERROR,
        "Failed to allocate a memory for the file name, name ptr=%p, req size=%d", (void *)p_file->name, LEN_PATH_MAX);
      return 2;
    }
    p_file->name[0] = '\0'; // required to ensure strlen() works correctly on this memory
    LOG(LOG_TYPE_MEM, LOG_LEVEL_INFO, 
        "memory for file name has been allocated, ptr=%p, size=%d", p_file->name, LEN_PATH_MAX);
  }
  else {
    // Reset the previous file name.
    // Due to its constant size, the file name is reset by setting the memory to 0.
    // Since the file name changes often, a memory reset should occur in each call of this function.
    memset(p_file->name, 0, strlen(p_file->name));
    LOG(LOG_TYPE_MEM, LOG_LEVEL_INFO, "file name set to 0, ptr=%p", p_file->name);
  }

  // Init the file type
  p_file->type = FTYPE_DFL;

  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Done.");
  return 0;
}

/* Reset (init) the file content.
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
int reset_file_cont(t_flcont *p_flcont, size_t size_fcont)
{
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Begin");
  free_file_cont(p_flcont);
  if ( alloc_file_cont(p_flcont, size_fcont) == NULL )
    return 1;
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Done.");
  return 0;
}

/* Reset (init) the file info.
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
int reset_file_inf(file_inf *p_file, size_t size_fcont)
{
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Begin");

  // check if the memory for the file info instance is allocated
  if (!p_file) {
    LOG(LOG_TYPE_MEM, LOG_LEVEL_ERROR, 
      "An invalid file info pointer has passed, p_file=%p", (void *)p_file);
    return 1;
  }

  // reset the file name & type
  int rc;
  if ( (rc = reset_file_name_type(p_file)) != 0 )
    return rc;
  
  // reset the file content
  if ( (rc = reset_file_cont(&p_file->cont, size_fcont)) != 0 )
    return rc;

  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Done.");
  return 0;
}

/* Reset (init) the error info.
 *
 * This function resets (initializes) an error info instance by allocating memory for 
 * the error message if it is not already allocated and setting the error number and 
 * system error number to 0. The error message is reset if the error number is set.
 *
 * Parameters:
 *  p_err - A pointer to an error info instance to reset.
 * 
 * Return value:
 *  0 on success, >0 on failure.
 * 
 * Notes:
 * - error message memory is allocated only if it was not done previously (size is LEN_ERRMSG_MAX)
 * - error number is set to 0
 * - error message is set to 0 if the error number is set
 * - system error number is set to 0
 */
int reset_err_inf(err_inf *p_err)
{
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Begin");

  // Check if the memory for the error info instance is allocated
  if (!p_err) {
    LOG(LOG_TYPE_MEM, LOG_LEVEL_ERROR, 
      "An invalid error info pointer has passed, p_err=%p", (void *)p_err);
    return 1;
  }

  // Init the error message
  if (!p_err->err_inf_u.msg) {
    // Initial dynamic memory allocation.
    // It performs for the first call of this function or after memory freeing.
    // Deallocation is required later
    p_err->err_inf_u.msg = (char*)malloc(LEN_ERRMSG_MAX * sizeof(char));
    if (!p_err->err_inf_u.msg) {
      LOG(LOG_TYPE_MEM, LOG_LEVEL_ERROR, "Failed to allocate a memory for the error info");
      return 1;
    }
    p_err->err_inf_u.msg[0] = '\0'; // required to ensure strlen() works correctly on this memory
    p_err->num = 0;
    LOG(LOG_TYPE_MEM, LOG_LEVEL_INFO, "memory for error info has been allocated, ptr=%p, size=%d", 
        p_err->err_inf_u.msg, LEN_ERRMSG_MAX);
  }
  else {
    // Reset the previous error info.
    // Due to its constant size, the error's message is reset by setting the memory to 0.
    // Since the error message does not change very often, a memory reset will occur only
    // if the error number is set.
    if (p_err->num && p_err->num != ERRNUM_ERRINF_ERR) {
      p_err->num = 0;
      memset(p_err->err_inf_u.msg, 0, strlen(p_err->err_inf_u.msg));
      LOG(LOG_TYPE_MEM, LOG_LEVEL_INFO, "error info set to 0, ptr=%p", p_err->err_inf_u.msg);
    }
  }

  // Reset the system error number
  if (errno) errno = 0;

  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Done.");
  return 0;
}

/* Allocate and reset the error information structure.
 * This function allocates memory for an error information structure and resets its
 * contents to prepare it for storing error data. If memory allocation fails, it
 * prints an error message and returns a failure code.
 *
 * Parameters:
 *  pp_errinf - A double pointer to an error info structure to allocate and reset.
 *
 * Return value:
 *  0 on success, >0 on failure. (e.g., memory allocation error).
 */
int alloc_reset_err_inf(err_inf **pp_errinf)
{
  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Begin");

  // Allocate memory if needed
  *pp_errinf = (err_inf*)malloc(sizeof(err_inf));
  if (!*pp_errinf) {
    LOG(LOG_TYPE_MEM, LOG_LEVEL_ERROR, "Memory allocation failed for the error info instance");
    return 1;
  }
  
  // Allocate or reset the error message buffer
  (*pp_errinf)->err_inf_u.msg = NULL; // needed for a correct work of reset_err_inf()
  if (reset_err_inf(*pp_errinf) != 0) { // allocate memory for error message buffer
    LOG(LOG_TYPE_MEM, LOG_LEVEL_ERROR, "Cannot reset the memory for the error info instance");
    return 2;
  }

  LOG(LOG_TYPE_MEM, LOG_LEVEL_DEBUG, "Done.");
  return 0;
}