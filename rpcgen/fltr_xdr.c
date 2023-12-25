/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "fltr.h"

bool_t
xdr_t_flname (XDR *xdrs, t_flname *objp)
{
	register int32_t *buf;

	 if (!xdr_string (xdrs, objp, SIZE_FNAME))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_t_flcont (XDR *xdrs, t_flcont *objp)
{
	register int32_t *buf;

	 if (!xdr_bytes (xdrs, (char **)&objp->t_flcont_val, (u_int *) &objp->t_flcont_len, ~0))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_t_errmsg (XDR *xdrs, t_errmsg *objp)
{
	register int32_t *buf;

	 if (!xdr_string (xdrs, objp, ~0))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_file (XDR *xdrs, file *objp)
{
	register int32_t *buf;

	 if (!xdr_t_flname (xdrs, &objp->name))
		 return FALSE;
	 if (!xdr_t_flcont (xdrs, &objp->content))
		 return FALSE;
	return TRUE;
}
