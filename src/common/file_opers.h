#ifndef _FILE_OPERS_H_
#define _FILE_OPERS_H_

#include <stdio.h>

/*
 * Forward definitions for the types defined in the RPC protocol. The actual types
 * will be available in source file after inclusion the RPC protocol header file.
 */
typedef struct err_inf err_inf;

// TODO: document it
FILE *open_file(const char *const flname, const char *const mode, err_inf *p_errinf);

#endif