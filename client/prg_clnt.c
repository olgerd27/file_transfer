/*
 * prg_clnt.c: the client program to initiate the remote file transfers
 */

#include <stdio.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* generated by rpcgen */

const char *rmt_host; // a remote host name

/*
 * Print the help info
 *
 * Input:
 * this_prg_name - this program name
 * extend_help   - print the extended (1), or short help info (0)
 */
void print_help(char *this_prg_name, int extend_help)
{
  
  // Print the extended part of help info
  if (extend_help == 1)
    fprintf(stderr, "The RPC client program that uploads files to and downloads from the remote server.\n\n");

  // Print the mandatory part of help info
  fprintf(stderr, "Usage:\n"
    "%s [-u | -d] [server] [file_src] [file_targ]\n"
    "%s [-h]\n\n", this_prg_name, this_prg_name); 

  // Print the extended part of help info
  if (extend_help == 1)
    fprintf(stderr,
      "Options:\n"
      "-u         action: upload a file to the remote server\n"
      "-d         action: download a file from the remote server\n"
      "server     a remote server hostname\n"
      "file_src   a source file name on a client (if upload action) or server (if download action) side\n"
      "file_targ  a target file name on a server (if upload action) or client (if download action) side\n"
      "-h         action: print this help\n"
      "\nExamples:\n"
      "1 Upload the local file /tmp/file to server 'serva' and save it there as /tmp/file_upld:\n"
      "%s -u serva /tmp/file /tmp/file_upld\n\n"
      "2 Download the remote file /tmp/file from server 'servb' and save it locally as /tmp/file_down:\n"
      "%s -d servb /tmp/file /tmp/file_down\n"
      , this_prg_name, this_prg_name);
    else
      fprintf(stderr, "To see the extended help info use '-h' option.\n");
}

/*
 * The supported program actions
 */
enum Action { act_upload, act_download, act_help, act_invalid };

// Verify the command-line arguments and determine the action
enum Action process_args(int argc, char *argv[])
{
  enum Action action = act_invalid;

  // User didn't pass any arguments
  if (argc == 1) {
    print_help(argv[0], 0);
    exit(1);
  }

  // Process the action argument
  if (argc >= 2 && argv[1][0] == '-') {
    switch (argv[1][1]) {
      case 'u':
        // user wants to upload file to a server
	action = act_upload;
        break;
      case 'd':
        // user wants to download file from a server
	action = act_download;
        break;
      case 'h':
        // user wants to see the help
	action = act_help;
        break;
      default:
        // invalid action
	fprintf(stderr, "!--Error 2: an invalid action has passed\n\n");
	print_help(argv[0], 0);
	exit(2);
    }
  }

  // User specified an invalid number of arguments.
  // argc should be 5 except if help was choosen
  if ( action != act_help && argc != 5 ) {
    fprintf(stderr, "!--Error 3: invalid number of arguments\n\n");
    print_help(argv[0], 0);
    exit(3);
  }

  // User specified an invalid target filename on a remote server
  if (action == act_upload && argv[4][0] != '/') {
    fprintf(stderr, "!--Error 4: passed an invalid target filename.\n"
      "Please specify the full path+name for the uploaded (target) file on the remote host.\n\n");
    print_help(argv[0], 0);
    exit(4);
  }

  // User specified an invalid source filename on a remote server
  if (action == act_download && argv[3][0] != '/') {
    fprintf(stderr, "!--Error 5: passed an invalid source filename.\n"
      "Please specify the full path+name for the downloaded (source) file on the remote host.\n\n");
    print_help(argv[0], 0);
    exit(5);
  }

  return action;
}

/*
 * Create client "handle" used for calling FLTRPROG 
 * on the server designated on the command line.
 */
CLIENT * create_client()
{
  CLIENT *clnt = clnt_create(rmt_host, FLTRPROG, FLTRVERS, "tcp");
  if (clnt == (CLIENT *)NULL) {
    // Print an error indication why a client handle could not be created.
    // Used when clnt_create() call fails.
    clnt_pcreateerror(rmt_host);
    exit(6);
  }
  return clnt;
}

// Free the memory for storing file content
void free_file_cont(t_flcont *p_flcont)
{
  if (p_flcont && p_flcont->t_flcont_val) {
    free(p_flcont->t_flcont_val);
    p_flcont->t_flcont_val = NULL;
    p_flcont->t_flcont_len = 0;
  } else {
    fprintf(stderr, "!--Error 8: Cannot free the file content, p_flcont=%p\n", (void*)p_flcont);
    exit(8);
  }
}

/*
 * The Upload File section
 * Error numbers range: 10-19
 */
// Read the file to get its content for transfering
void read_file(const char *filename, file *fobj)
{
  // Create the pointers for quick access
  u_int *pf_len = &fobj->cont.t_flcont_len; // pointer to file length
  char **pf_data = &fobj->cont.t_flcont_val; // pointer to file data

  // Open the file
  FILE *hfile = fopen(filename, "rb");
  if (hfile == NULL) {
    fprintf(stderr, "!--Error 10: cannot open file '%s' for reading\n"
                    "System error %i: %s\n",
                    filename, errno, strerror(errno));
    exit(10);
  }

  // Obtain file size
  fseek(hfile, 0, SEEK_END);
  *pf_len = ftell(hfile);
  rewind(hfile);

  // Allocate memory to contain the whole file
  *pf_data = (char*)malloc(*pf_len);
  if (*pf_data == NULL) {
    fprintf(stderr, "!--Error 11: memory allocation error, size=%ld\n", *pf_len);
    fclose(hfile);
    exit(11);
  }

  // Read the file content into the buffer
  size_t nch = fread(*pf_data, 1, *pf_len, hfile);

  // Check a number of items read and if an error has occurred
  if (nch < *pf_len || ferror(hfile)) {
    fprintf(stderr, "!--Error 12: file reading error: '%s'.\n"
                    "System error %i: %s\n",
                    filename, errno, strerror(errno));
    fclose(hfile);
    free_file_cont(&fobj->cont); // free the file content memory in case of error
    exit(12);
  }

  fclose(hfile);
}

// The main function to Upload file
void file_upload(CLIENT *client, const char *flnm_src_clnt, /*const*/ char *flnm_dst_serv)
{
  file file_obj; // file object
  errinf *srv_errinf; // result from a server - error info
  // TODO: figure out if a memory freeing required for srv_errinf?

  // Set the target file name to the file object
  file_obj.name = flnm_dst_serv;

  // Get the file content and set it to the file object
  read_file(flnm_src_clnt, &file_obj);

  // Make a file upload to a server through RPC
  srv_errinf = upload_file_1(&file_obj, client);

  // Print a message to standard error indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by upload_file_1().
  if (srv_errinf == (errinf *)NULL) {
    clnt_perror(client, rmt_host);
    free_file_cont(&file_obj.cont); // free the file content memory in case of error
    exit(13);
  }

  // Check an error that may occur on the server
  if (srv_errinf->num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", srv_errinf->num, srv_errinf->errinf_u.msg);
    free_file_cont(&file_obj.cont); // free the file content memory in case of error
    exit(14);
  }

  // Okay, we successfully called the remote procedure.

  // Freeing the memory that stores the file content
  free_file_cont(&file_obj.cont); // free the file content memory
}

/*
 * The Download File section
 * Error numbers range: 20-29
 */
// Save a file downloaded on the server to a new local file
void save_file(const char *flname, t_flcont *flcont)
{
  // Open the file 
  FILE *hfile = fopen(flname, "wbx");
  if (hfile == NULL) {
    fprintf(stderr, 
      "!--Error 22: The file '%s' already exists or could not be opened in the write mode.\n"
      "System error %i: %s\n", 
      flname, errno, strerror(errno));
    exit(22);
  }

  // Write the server file data to a new file
  size_t nch = fwrite(flcont->t_flcont_val, 1, flcont->t_flcont_len, hfile);

  // Check a number of writtem items and if an error has occurred
  if (nch < flcont->t_flcont_len || ferror(hfile)) {
    fprintf(stderr,
            "!--Error 23: error writing to the file: '%s'.\n"
            "System error %i: %s\n",
            flname, errno, strerror(errno));
    fclose(hfile);
    exit(23);
  }

  fclose(hfile);
}

// The main function to Download file
void file_download(CLIENT *client, char *flnm_src_serv, const char *flnm_dst_clnt)
{
  // Result from a server - the downloaded file content and error info
  flcont_errinf *srv_flerr;

  // Make a file download from a server through RPC
  srv_flerr = download_file_1(&flnm_src_serv, client);

  // Print a message to standard error indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by download_file_1().
  if (srv_flerr == (flcont_errinf *)NULL) {
    clnt_perror(client, rmt_host);
    exit(20);
  }

  // Okay, we successfully called the remote procedure.

  // Check an error that may occur on the server
  if (srv_flerr->err_inf.num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", 
            srv_flerr->err_inf.num, srv_flerr->err_inf.errinf_u.msg);
    exit(21);
  }
  
  // Save the remote file content to a local file
  save_file(flnm_dst_clnt, &srv_flerr->file_cont);

  // Free the local memory with a remote file content
  free_file_cont(&srv_flerr->file_cont); // free the file content memory
}

int main(int argc, char *argv[])
{
  // Verify the passed command-line arguments and determine the action
  enum Action action = process_args(argc, argv);

  // The help action is not the RPC action. 
  // It should be called before setting up the RPC parameters.
  // Print the help info if it was choosen and exit.
  if (action == act_help) {
    print_help(argv[0], 1);
    return 0;
  }

  // RPC action is determined, client can be created

  // Get the command line arguments
  rmt_host = argv[2]; // a remote host name
  char *filename_src = argv[3]; // a source file name on a client (if upload) or server (if download) side
  char *filename_dst = argv[4]; // a target file name on a server (if upload) or client (if download) side

  CLIENT *clnt = create_client(); // create the client object

  // Perform the RPC action
  switch(action) {
    case act_upload:
      // upload file to a server
      file_upload(clnt, filename_src, filename_dst);
      break;
    case act_download:
      // download file from the server
      file_download(clnt, filename_src, filename_dst);
      break;
    default:
      fprintf(stderr, "Unknown program execution mode\n");
      clnt_destroy(clnt); // delete the client object
      return 7;
  }

  clnt_destroy(clnt); // delete the client object
  return 0;
}

