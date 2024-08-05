#include <stdio.h>
#include <stdlib.h>

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
	int name;
	int error;
};
typedef struct file_err file_err;

// create and return the object on heap using malloc 
file_err * create_object_malloc(picked_file *p_pkdfile)
{
    static file_err *p_flerr = NULL;
    if (!p_flerr) {
        p_flerr = (file_err*)malloc(sizeof(struct file_err));
        printf("Object %p has been created\n", (void*)p_flerr);
    }
    printf("[create_object_malloc] file_err obj: ptr=%p  ptr to ptr=%p\n", p_flerr, &p_flerr);
    printf("[create_object_malloc] picked_file arg, name: '%s', pftype: '%i'\n", 
            p_pkdfile->name, (int)p_pkdfile->pftype);
    return p_flerr;
}

// create and return the automatic object
file_err * create_object_auto(picked_file *p_pkdfile)
{
    static file_err flerr;
    printf("[create_object_auto] file_err obj: ptr=%p\n", &flerr);
    printf("[create_object_auto] picked_file arg, name: '%s', pftype: '%i'\n", 
            p_pkdfile->name, (int)p_pkdfile->pftype);
    return &flerr;
}

int main()
{
    file_err *p_flerr = NULL;
    int i;
    for (i = 0; i < 3; ++i) {
        picked_file pkdfile = {"/path/to/some/file", pk_ftype_target};
        p_flerr = create_object_malloc(&pkdfile);
        p_flerr = create_object_auto(&pkdfile);
        printf("\n");
    }
    return 0;
}
