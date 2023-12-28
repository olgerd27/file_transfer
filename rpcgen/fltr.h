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

#define SIZE_FNAME 256
#define SIZE_ERRMSG 512

typedef char *t_flname;

typedef struct {
	u_int t_flcont_len;
	char *t_flcont_val;
} t_flcont;

typedef char *t_errmsg;

struct file {
	t_flname name;
	t_flcont cont;
};
typedef struct file file;

#define FLTRPROG 0x20000027
#define FLTRVERS 1

#if defined(__STDC__) || defined(__cplusplus)
#define upload_file 1
extern  int * upload_file_1(file *, CLIENT *);
extern  int * upload_file_1_svc(file *, struct svc_req *);
#define download_file 2
extern  t_flcont * download_file_1(t_flname *, CLIENT *);
extern  t_flcont * download_file_1_svc(t_flname *, struct svc_req *);
#define get_error_msg 3
extern  t_errmsg * get_error_msg_1(void *, CLIENT *);
extern  t_errmsg * get_error_msg_1_svc(void *, struct svc_req *);
extern int fltrprog_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define upload_file 1
extern  int * upload_file_1();
extern  int * upload_file_1_svc();
#define download_file 2
extern  t_flcont * download_file_1();
extern  t_flcont * download_file_1_svc();
#define get_error_msg 3
extern  t_errmsg * get_error_msg_1();
extern  t_errmsg * get_error_msg_1_svc();
extern int fltrprog_1_freeresult ();
#endif /* K&R C */

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_t_flname (XDR *, t_flname*);
extern  bool_t xdr_t_flcont (XDR *, t_flcont*);
extern  bool_t xdr_t_errmsg (XDR *, t_errmsg*);
extern  bool_t xdr_file (XDR *, file*);

#else /* K&R C */
extern bool_t xdr_t_flname ();
extern bool_t xdr_t_flcont ();
extern bool_t xdr_t_errmsg ();
extern bool_t xdr_file ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_FLTR_H_RPCGEN */
