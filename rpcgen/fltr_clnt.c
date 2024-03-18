/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "fltr.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

errinf *
upload_file_1(file *argp, CLIENT *clnt)
{
	static errinf clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, upload_file,
		(xdrproc_t) xdr_file, (caddr_t) argp,
		(xdrproc_t) xdr_errinf, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

flcont_errinf *
download_file_1(t_flname *argp, CLIENT *clnt)
{
	static flcont_errinf clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, download_file,
		(xdrproc_t) xdr_t_flname, (caddr_t) argp,
		(xdrproc_t) xdr_flcont_errinf, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
