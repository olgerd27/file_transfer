#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "mem_opers.h"

#define DBG_MEM 1 // debug the memory manipulation

extern int errno; // global system error number

// TODO: check if the error number is ok in this function; this function should be used here and in both client and server code.
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
char * alloc_file_cont(t_flcont *p_flcont, size_t size)
{
  // Check if the memory for the file content instance is allocated
  if (!p_flcont) {
    fprintf(stderr, "Error 6: Failed to allocate memory for the file content. p_flcont=%p\n", (void*)p_flcont);
    return NULL;
  }

  // Memory allocation 
  if (!p_flcont->t_flcont_val) {
    p_flcont->t_flcont_val = (char*)malloc(size);
    if (!p_flcont->t_flcont_val) {
      p_flcont->t_flcont_len = 0;
      // NOTE: print this error in stderr only on the side of the caller of this function.
      // Because these are the details that another side should not be interested in.
      // A caller of this function can produce a more general error message and send it 
      // to another side through the error info instance, which is not accessible here.
      fprintf(stderr, "Error 7: Failed to allocate memory for the file content, required size: %ld\n", size);
      if (DBG_MEM) 
        printf("[alloc_file_cont] file cont allocation error, size=%d\n", p_flcont->t_flcont_len);
      return NULL;
    }
    p_flcont->t_flcont_val[0] = '\0'; // required to ensure strlen() works correctly on this memory
    p_flcont->t_flcont_len = size;
    if (DBG_MEM)
      printf("[alloc_file_cont] file cont allocated, size=%d\n", p_flcont->t_flcont_len);
  }
  return p_flcont->t_flcont_val;
}

/*
 * Free the file name memory.
 * This function frees the memory allocated for a file name string.
 *
 * Parameters:
 *  p_flname - A pointer to a file name string whose memory is to be freed.
 */
void free_file_name(t_flname *p_flname)
{
  if (p_flname) {
    free(*p_flname);
    *p_flname = NULL;
    if (DBG_MEM) printf("[free_file_name] DONE\n");
  }
}

/*
 * Free the file content memory.
 * This function frees the memory allocated for a file content instance.
 *
 * Parameters:
 *  p_flcont - A pointer to a file content instance whose memory is to be freed.
 */
void free_file_cont(t_flcont *p_flcont)
{
  if (p_flcont && p_flcont->t_flcont_val) {
    free(p_flcont->t_flcont_val);
    p_flcont->t_flcont_val = NULL;
    p_flcont->t_flcont_len = 0;
    if (DBG_MEM) printf("[free_file_cont] DONE\n");
  }
}

/*
 * Free the file info memory.
 * This function frees the memory allocated for a file info instance.
 *
 * Parameters:
 *  p_file - A pointer to a file info instance whose memory is to be freed.
 */
void free_file_inf(file_inf *p_file)
{
  if (p_file) {
    free_file_name(&p_file->name); // free the file name memory
    free_file_cont(&p_file->cont); // free the file content memory
    if (DBG_MEM) printf("[free_file_inf] DONE\n");
  }
}

/*
 * Free the error info memory.
 * This function frees the memory allocated for an error info instance.
 *
 * Parameters:
 *  p_err - A pointer to an error info instance whose memory is to be freed.
 */
void free_err_inf(err_inf *p_err)
{
  if (p_err && p_err->err_inf_u.msg) {
    free(p_err->err_inf_u.msg);
    p_err->err_inf_u.msg = NULL;
    p_err->num = 0;
    if (DBG_MEM) printf("[free_err_inf] DONE\n");
  }
}

// TODO: check if the error numbers are ok in this function
/*
 * Reset the file name & type.
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
  // Check if the memory for the file info struct instance is allocated
  if (!p_file) {
    fprintf(stderr, "Error 8: Failed to reset the file name & type. p_file=%p\n", (void*)p_file);
    return 8;
  }

  // Reset the file name
  if (!p_file->name) {
    // Initial dynamic memory allocation.
    // It performs for the first call of this function or after memory freeing.
    // Deallocation is required later
    p_file->name = (char*)malloc(LEN_PATH_MAX);
    if (!p_file->name) {
      fprintf(stderr, "Error 9: Failed to allocate a memory for the file name, name ptr=%p\n", 
              (void*)p_file->name);
      return 9;
    }
    p_file->name[0] = '\0'; // required to ensure strlen() works correctly on this memory
    if (DBG_MEM) printf("[reset_file_name_type] file name allocated\n");
  }
  else {
    // Reset the previous file name.
    // Due to its constant size, the file name is reset by setting the memory to 0.
    // Since the file name changes often, a memory reset should occur in each call of this function.
    memset(p_file->name, 0, strlen(p_file->name));
    if (DBG_MEM) printf("[reset_file_name_type] file name set to 0\n");
  }

  // Reset the file type
  p_file->type = FTYPE_DFL;

  if (DBG_MEM) printf("[reset_file_name_type] DONE\n");
  return 0;
}

/*
 * Reset the file content.
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
  free_file_cont(p_flcont);
  if (!alloc_file_cont(p_flcont, size_fcont))
    return 10;
  if (DBG_MEM) printf("[reset_file_cont] DONE\n");
  return 0;
}

/*
 * Reset the file info.
 *
 * This function resets the file information, including the file name, type, and content.
 * It allocates new memory for the file content based on the specified size.
 *
 * Parameters:
 *  p_file      - A pointer to a file info instance to reset.
 *  size_fcont  - The size of the memory to allocate for the file content.
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int reset_file_inf_NEW(file_inf *p_file, size_t size_fcont)
{
  // check if the memory for the file info instance is allocated
  if (!p_file) {
    fprintf(stderr, "Error 8: Failed to reset the file info. p_file=%p\n", (void*)p_file);
    return 8;
  }

  // reset the file name & type
  int rc;
  if ( (rc = reset_file_name_type(p_file)) != 0 )
    return rc;
  
  // reset the file content
  if ( (rc = reset_file_cont(&p_file->cont, size_fcont)) != 0 )
    return rc;
  
  if (DBG_MEM) printf("[reset_file_inf_NEW] DONE\n");
  return 0;
}

// TODO: check if the error numbers are ok in this function
/*
 * Reset the error info.
 *
 * This function resets an error info instance by allocating memory for the error
 * message if it is not already allocated and setting the error number and system
 * error number to 0. The error message is reset if the error number is set.
 *
 * Parameters:
 *  p_err - A pointer to an error info instance to reset.
 * 
 * Return value:
 *  0 on success, >0 on failure.
 * 
 * Notes:
 * - error message memory is allocated only if it's not allocated (size is LEN_ERRMSG_MAX)
 * - error number is set to 0
 * - error message is set to 0 if the error number is set
 * - system error number is set to 0
 */
int reset_err_inf(err_inf *p_err)
{
  // Check if the memory for the error info instance is allocated
  if (!p_err) {
    fprintf(stderr, "Error 13: Failed to reset the error information, p_err=%p\n", (void*)p_err);
    return 11;
  }

  // Reset the error message
  if (!p_err->err_inf_u.msg) {
    // Initial dynamic memory allocation.
    // It performs for the first call of this function or after memory freeing.
    // Deallocation is required later
    p_err->err_inf_u.msg = (char*)malloc(LEN_ERRMSG_MAX);
    if (!p_err->err_inf_u.msg) {
      fprintf(stderr, "Error 14: Failed to allocate a memory for the error info, msg ptr=%p\n", 
              (void*)p_err->err_inf_u.msg);
      return 12;
    }
    p_err->err_inf_u.msg[0] = '\0'; // required to ensure strlen() works correctly on this memory
    p_err->num = 0;
    if (DBG_MEM) printf("[reset_err_inf] error info allocated\n");
  }
  else {
    // Reset the previous error info.
    // Due to its constant size, the error's message is reset by setting the memory to 0.
    // Since the error message does not change very often, a memory reset will occur only
    // if the error number is set.
    if (p_err->num) {
      p_err->num = 0;
      memset(p_err->err_inf_u.msg, 0, strlen(p_err->err_inf_u.msg));
      if (DBG_MEM) printf("[reset_err_inf] error info set to 0\n");
    }
  }

  // Reset the system error number
  if (errno) errno = 0;

  if (DBG_MEM) printf("[reset_err_inf] DONE\n");
  return 0;
}
