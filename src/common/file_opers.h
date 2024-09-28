#ifndef _FILE_OPERS_H_
#define _FILE_OPERS_H_

#include <stdio.h>
#include "../rpcgen/fltr.h"

// Read the file content into the buffer
int read_file_cont(const t_flname flname, t_flcont *p_flcont, 
                   err_inf **pp_errinf);

// Save a file content to a new local file
int save_file_cont(const t_flname flname, const t_flcont *p_flcont, 
                   err_inf **pp_errinf);

#endif