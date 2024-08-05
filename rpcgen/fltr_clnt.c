/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "fltr.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

err_inf *
upload_file_1(file_inf *argp, CLIENT *clnt)
{
	static err_inf clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, upload_file,
		(xdrproc_t) xdr_file_inf, (caddr_t) argp,
		(xdrproc_t) xdr_err_inf, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

file_err *
download_file_1(t_flname *argp, CLIENT *clnt)
{
	static file_err clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, download_file,
		(xdrproc_t) xdr_t_flname, (caddr_t) argp,
		(xdrproc_t) xdr_file_err, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

file_err *
pick_file_1(picked_file *argp, CLIENT *clnt)
{
	static file_err clnt_res;

	memset((char *)&clnt_res, 0, sizeof(clnt_res));
	if (clnt_call (clnt, pick_file,
		(xdrproc_t) xdr_picked_file, (caddr_t) argp,
		(xdrproc_t) xdr_file_err, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
