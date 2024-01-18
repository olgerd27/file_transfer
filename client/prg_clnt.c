/*
 * prg_clnt.c: the client program to initiate the remote file transfers
 *
 * Build this program:
 * gcc prg_clnt.c ../rpcgen/fltr_clnt.c ../rpcgen/fltr_xdr.o -I/usr/include/tirpc -o prg_clnt -lnsl -ltirpc
 */

#include <stdio.h>
#include "../rpcgen/fltr.h" /* generated by rpcgen */

const char *pserver_name; // server name

/*
 * Read the binary file to get its content for transfering
 */
void read_file(const char *filename, file *fobj)
{
  u_int *pf_len = &fobj->cont.t_flcont_len; // pointer to file length, for quick access
  char **pf_data = &fobj->cont.t_flcont_val; // pointer to file data, for quick access

  // Open the file
  FILE *hfile = fopen(filename, "rb");
  if (hfile == NULL) {
    fprintf(stderr, "!--Error 2: cannot open file '%s' for reading\n", filename);
    exit(2);
  }

  // obtain file size
  fseek(hfile, 0, SEEK_END);
  *pf_len = ftell(hfile);
  rewind(hfile);

  // allocate memory to contain the whole file
  *pf_data = (char*)malloc(*pf_len);
  if (*pf_data == NULL) {
    fprintf(stderr, "!--Error 3: memory allocation error, size=%ld\n", *pf_len);
    exit(3);
  }

  // copy the file into the buffer
  size_t res_read = fread(*pf_data, 1, *pf_len, hfile);
  if (res_read != *pf_len) {
    fprintf(stderr, "!--Error 4: file '%s' reading error\n", filename);
    exit(4);
  }

  fclose(hfile);
}

/*
 * The Upload file main function
 */
void file_upload(CLIENT *client, const char *flnm_src, /*const*/ char *flnm_dst)
{
  file file_obj; // file object
  errinf *res_serv; // result returned from a server

  // Set the destination file name to the file object
  file_obj.name = flnm_dst;

  // Get the file content and set it to the file object
  read_file(flnm_src, &file_obj);

  // Make an upload file through RPC on a server
  res_serv = upload_file_1(&file_obj, client);

  // Freeing the memory that stores the content of a file 
  // immediately after calling the remote function
  free(file_obj.cont.t_flcont_val);

  // Print a message to standard error indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by upload_file_1().
  if (res_serv == (errinf *)NULL) {
    clnt_perror(client, pserver_name);
    exit(6);
  }

  // Okay, we successfully called the remote procedure.
  // Check an error that may occur on the server
  if (res_serv->num != 0) {
    // Remote system error. Print error message and die.
    printf("!---Server error %d: %s\n", res_serv->num, res_serv->errinf_u.msg);
    exit(res_serv->num);
  }
}

/*
 * The Download file main function
 */
void file_download()
{
}

/*
 * Create client "handle" used for calling FLTRPROG 
 * on the server designated on the command line.
 */
CLIENT * create_client()
{
  CLIENT *clnt = clnt_create(pserver_name, FLTRPROG, FLTRVERS, "tcp");
  if (clnt == (CLIENT *)NULL) {
    // Print an error indication why a client handle could not be created.
    // Used when clnt_create() call fails.
    clnt_pcreateerror(pserver_name);
    exit(5);
  }
  return clnt;
}

int main(int argc, char *argv[])
{
//  printf("[main] 0\n");
  // Check the command line arguments
  if (argc != 4) {
    fprintf(stderr, "!--Error 1: Usage: %s [host] [file_source] [file_dest]\n", argv[0]);
    return 1;
  }

  // Get the command line arguments
  pserver_name = argv[1];
  // TODO: move the definitions of both source and destination files to file_upload()
  // Maybe ask user through stdin about these two values.
  char *filename_src = argv[2]; // a source file name on a client side that will be transferred to a server
  char *filename_dst = argv[3]; // a name of destination file on a server side that will be saved on a server
//  printf("[main] 1\n");

   CLIENT *clnt = create_client(); // create the client object
//  printf("[main] 2\n");

  file_upload(clnt, filename_src, filename_dst); // upload file to a server
//  printf("[main] 3\n");

  clnt_destroy(clnt); // delete the client object
//  printf("[main] 4\n");

  return 0;
}

