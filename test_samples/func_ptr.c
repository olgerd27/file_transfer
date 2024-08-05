#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// struct & enum definitions
typedef struct { int id; } CLIENT;
static CLIENT *clnt;

typedef char *t_flname;

enum pick_ftype {
	pk_ftype_source = 0,
	pk_ftype_target = 1,
};
typedef enum pick_ftype pick_ftype;

struct picked_file {
    t_flname name;
    pick_ftype pftype;
};
typedef struct picked_file picked_file;

struct file_err {
    t_flname file;
    int err;
};
typedef struct file_err file_err;

// The common pointer type for both select_file() and file_pick_rmt() function
typedef file_err * (*T_pf_select)(picked_file *);

// Function 1 that can be assigned to the function pointer.
// The file_err object is created in this function only. 
file_err * select_file(picked_file *p_flpkd)
{
    static file_err flerr;

    // set some file name
    char *fname_chosen; // chosen file name
    switch (p_flpkd->pftype) {
        case pk_ftype_source:
            fname_chosen = "/space/rpc/file_source.c";
            break;
        case pk_ftype_target:
            fname_chosen = "/space/rpc/file_target.c";
            break;    
        default:
            fname_chosen = "/invalid/name";
    }

    flerr.file = (t_flname)malloc(1024);
    strcpy(flerr.file, p_flpkd->name);
    strcat(flerr.file, fname_chosen);

    // set some error number
    flerr.err = 0;

    return &flerr; // the passed pointer should be returned
}

file_err * pick_file_1(picked_file *p_flpkd, CLIENT *)
{
    static file_err *p_flerr_ret;
    p_flerr_ret = select_file(p_flpkd);
    return p_flerr_ret;
}

// Function 2 that can be assigned to the funcion pointer.
file_err * file_pick_rmt(picked_file *p_flpkd)
{
    file_err *p_flerr_serv = pick_file_1(p_flpkd, clnt);
    if (p_flerr_serv->err != 0)
        fprintf(stderr, "Error picking remote file\n");
    return p_flerr_serv;
}

// The main function to call a function the passed function pointer points to.
char *get_filename_inter(char *dir_start, enum pick_ftype pftype, T_pf_select pf_select)
{
    picked_file p_flpkd = {dir_start, pftype};
    file_err *p_flerr = (*pf_select)(&p_flpkd);
    return p_flerr->file;
}

int main()
{
    clnt = (CLIENT*)malloc(sizeof(CLIENT)); // create the CLIENT object

    char *flname_src = get_filename_inter("/home", pk_ftype_source, &select_file);
    char *flname_trg = get_filename_inter("/home", pk_ftype_target, &file_pick_rmt);

    printf("Selected source file name: '%s'\n", flname_src);
    printf("Selected target file name: '%s'\n", flname_trg);
    
    free(flname_src);
    free(flname_trg);
    free(clnt);
    return 0;
}

/*
Question to GPT:
Will the current approach be reliable if I add several new functions that have the same signature 
defined by the T_pf_select type and can also be passed to get_filename_inter()?
 */