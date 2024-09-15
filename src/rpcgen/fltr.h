/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _FLTR_H_RPCGEN
#define _FLTR_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif

#define LEN_PATH_MAX 4096
#define LEN_ERRMSG_MAX 4096

typedef char *t_flname;

typedef struct {
	u_int t_flcont_len;
	char *t_flcont_val;
} t_flcont;

enum filetype {
	FTYPE_DFL = 0,
	FTYPE_REG = 1,
	FTYPE_DIR = 2,
	FTYPE_OTH = 3,
	FTYPE_NEX = 4,
	FTYPE_INV = 5,
};
typedef enum filetype filetype;

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

struct file_inf {
	t_flname name;
	filetype type;
	t_flcont cont;
};
typedef struct file_inf file_inf;

struct err_inf {
	int num;
	union {
		char *msg;
	} err_inf_u;
};
typedef struct err_inf err_inf;

struct file_err {
	file_inf file;
	err_inf err;
};
typedef struct file_err file_err;

#define FLTRPROG 0x20000027
#define FLTRVERS 1

#if defined(__STDC__) || defined(__cplusplus)
#define upload_file 1
extern  err_inf * upload_file_1(file_inf *, CLIENT *);
extern  err_inf * upload_file_1_svc(file_inf *, struct svc_req *);
#define download_file 2
extern  file_err * download_file_1(t_flname *, CLIENT *);
extern  file_err * download_file_1_svc(t_flname *, struct svc_req *);
#define pick_file 3
extern  file_err * pick_file_1(picked_file *, CLIENT *);
extern  file_err * pick_file_1_svc(picked_file *, struct svc_req *);
extern int fltrprog_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define upload_file 1
extern  err_inf * upload_file_1();
extern  err_inf * upload_file_1_svc();
#define download_file 2
extern  file_err * download_file_1();
extern  file_err * download_file_1_svc();
#define pick_file 3
extern  file_err * pick_file_1();
extern  file_err * pick_file_1_svc();
extern int fltrprog_1_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_t_flname (XDR *, t_flname*);
extern  bool_t xdr_t_flcont (XDR *, t_flcont*);
extern  bool_t xdr_filetype (XDR *, filetype*);
extern  bool_t xdr_pick_ftype (XDR *, pick_ftype*);
extern  bool_t xdr_picked_file (XDR *, picked_file*);
extern  bool_t xdr_file_inf (XDR *, file_inf*);
extern  bool_t xdr_err_inf (XDR *, err_inf*);
extern  bool_t xdr_file_err (XDR *, file_err*);

#else /* K&R C */
extern bool_t xdr_t_flname ();
extern bool_t xdr_t_flcont ();
extern bool_t xdr_filetype ();
extern bool_t xdr_pick_ftype ();
extern bool_t xdr_picked_file ();
extern bool_t xdr_file_inf ();
extern bool_t xdr_err_inf ();
extern bool_t xdr_file_err ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_FLTR_H_RPCGEN */
