/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 *
 * This is a debug version that can be used to traversal through the XDRs functions.
 * To use this debug version instead of a normal (release) one generated by rpcgen, do the following:
 * $> cd rpcgen/
 * $> cp fltr_xdr_DBG.c fltr_xdr.c
 * $> make -f makefile.rpc fltr_xdr.o  # build the object file
 * $> cd ../client/
 * $> make -f makefile.client  # build the client with a debug version of fltr_xdr.o
 * $> ./prg_clnt .....   # execute the client
 *
 * To use the release (normal) version of this file just make a build:
 * $> cd rpcgen/
 * $> make -f makefile.rpc clean
 * $> make -f makefile.rpc 
 */

#include "fltr.h"

const char * x_op_str(enum xdr_op op)
{
	switch (op)
	{
	case XDR_ENCODE:
		return "Encode";
	case XDR_DECODE:
		return "Decode";
	case XDR_FREE:
		return "Free";
	default:
		return "UNKNOWN";
	}
}

bool_t
xdr_t_flname (XDR *xdrs, t_flname *objp)
{
	register int32_t *buf;
	printf("[xdr_t_flname] 0, xdr_op=%s, t_flname ptr=%p\n", x_op_str(xdrs->x_op), objp);

	 if (!xdr_string (xdrs, objp, LEN_PATH_MAX)) {
		 printf("[xdr_t_flname] 1, FALSE xdr_string(), t_flname ptr=%p\n", objp);
		 return FALSE;
	 }
	printf("[xdr_t_flname] TRUE->DONE, t_flname ptr=%p\n", objp);
	return TRUE;
}

bool_t
xdr_t_flcont (XDR *xdrs, t_flcont *objp)
{
	register int32_t *buf;
	printf("[xdr_t_flcont] 0, xdr_op=%s, t_flcont ptr=%p\n", x_op_str(xdrs->x_op), objp);

	 if (!xdr_bytes (xdrs, (char **)&objp->t_flcont_val, (u_int *) &objp->t_flcont_len, ~0)) {
		 printf("[xdr_t_flcont] 1, FALSE xdr_bytes(), t_flcont ptr=%p\n", objp);
		 return FALSE;
	 }
	printf("[xdr_t_flcont] TRUE->DONE, t_flcont ptr=%p\n", objp);
	return TRUE;
}

bool_t
xdr_filetype (XDR *xdrs, filetype *objp)
{
	register int32_t *buf;
	printf("[xdr_filetype] 0, xdr_op=%s, filetype ptr=%p\n", x_op_str(xdrs->x_op), objp);

	 if (!xdr_enum (xdrs, (enum_t *) objp)) {
		 printf("[xdr_filetype] 1, FALSE xdr_enum(), filetype ptr=%p\n", objp);
		 return FALSE;
	 }
	printf("[xdr_filetype] TRUE->DONE, filetype ptr=%p\n", objp);
	return TRUE;
}

bool_t
xdr_pick_ftype (XDR *xdrs, pick_ftype *objp)
{
	register int32_t *buf;

	 if (!xdr_enum (xdrs, (enum_t *) objp))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_picked_file (XDR *xdrs, picked_file *objp)
{
	register int32_t *buf;

	 if (!xdr_t_flname (xdrs, &objp->name))
		 return FALSE;
	 if (!xdr_pick_ftype (xdrs, &objp->pftype))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_file_inf (XDR *xdrs, file_inf *objp)
{
	register int32_t *buf;
	printf("[xdr_file_inf] 0, xdr_op=%s, file_inf ptr=%p\n", x_op_str(xdrs->x_op), objp);

	 if (!xdr_t_flname (xdrs, &objp->name)) {
		 printf("[xdr_file_inf] 1, FALSE xdr_t_flname(), file_inf ptr=%p\n", objp);
		 return FALSE;
	 }
	 if (!xdr_filetype (xdrs, &objp->type)) {
		 printf("[xdr_file_inf] 2, FALSE xdr_filetype(), file_inf ptr=%p\n", objp);
		 return FALSE;
	 }
	 if (!xdr_t_flcont (xdrs, &objp->cont)) {
		 printf("[xdr_file_inf] 3, FALSE xdr_t_flcont(), file_inf ptr=%p\n", objp);
		 return FALSE;
	 }
	printf("[xdr_file_inf] TRUE->DONE, file_inf ptr=%p\n", objp);
	return TRUE;
}

bool_t
xdr_err_inf (XDR *xdrs, err_inf *objp)
{
	register int32_t *buf;
	printf("[xdr_err_inf] 0, xdr_op=%s, err_inf ptr=%p\n", x_op_str(xdrs->x_op), objp);

	 if (!xdr_int (xdrs, &objp->num)) {
		 printf("[xdr_err_inf] 1, FALSE xdr_int(), err_inf ptr=%p\n", objp);
		 return FALSE;
	 }
	switch (objp->num) {
	case 0:
		break;
	default:
		 if (!xdr_string (xdrs, &objp->err_inf_u.msg, LEN_ERRMSG_MAX)) {
			 printf("[xdr_err_inf] 2, FALSE xdr_string(), err_inf ptr=%p\n", objp);
			 return FALSE;
		 }
		break;
	}
	printf("[xdr_err_inf] TRUE->DONE, err_inf ptr=%p\n", objp);
	return TRUE;
}

bool_t
xdr_file_err (XDR *xdrs, file_err *objp)
{
	register int32_t *buf;
	printf("[xdr_file_err] 0, xdr_op=%s, file_err ptr=%p\n", x_op_str(xdrs->x_op), objp);

	 if (!xdr_file_inf (xdrs, &objp->file)) {
		 printf("[xdr_file_err] 1, FALSE xdr_file_inf(), file_err ptr=%p\n", objp);
		 return FALSE;
	 }
	 if (!xdr_err_inf (xdrs, &objp->err)) {
		 printf("[xdr_file_err] 2, FALSE xdr_err_inf(), file_err ptr=%p\n", objp);
		 return FALSE;
	 }
	printf("[xdr_file_err] TRUE->DONE, file_err ptr=%p\n", objp);
	return TRUE;
}
