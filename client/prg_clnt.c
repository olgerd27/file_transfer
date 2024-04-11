/*
 * prg_clnt.c: the client program to initiate the remote file transfers
 */

#include <stdio.h>
#include <errno.h>
#include "../rpcgen/fltr.h" /* generated by rpcgen */

// Define the global variables for remote host and file names
static const char *rmt_host; // a remote host name
static const char *filename_src; // a source file name on a client (if upload) or server (if download) side
static const char *filename_trg; // a target file name on a server (if upload) or client (if download) side

/*
 * Print the help info
 *
 * Input:
 * this_prg_name - this program name
 * extend_help   - print the extended (1), or short help info (0)
 */
// TODO: replace int arg with the enum type that should describe short and extended type of help
void print_help(const char *this_prg_name, int extend_help)
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
      "1. Upload the local file /tmp/file to server 'serva' and save it remotely as /tmp/file_upld:\n"
      "%s -u serva /tmp/file /tmp/file_upld\n\n"
      "2. Download the remote file /tmp/file from server 'servb' and save it locally as /tmp/file_down:\n"
      "%s -d servb /tmp/file /tmp/file_down\n"
      , this_prg_name, this_prg_name);
    else
      fprintf(stderr, "To see the extended help info use '-h' option.\n");
}

/*
 * The supported program actions
 */
// Possible actions
enum Action {
    act_none       = (0 << 0)
  , act_help_short = (1 << 0)
  , act_help_full  = (1 << 1)
  , act_upload     = (1 << 2)
  , act_download   = (1 << 3)
  , act_interact   = (1 << 4)
  , act_invalid    = (1 << 5)
};

// Parse & verify the command-line arguments and determine the action
enum Action process_args(int argc, char *argv[])
{
  enum Action action = act_none;

  // User didn't pass any arguments - show a short help info
  if (argc == 1) return act_help_short;

  // Check if user wants to see the full help info
  if (argc == 2 && strcmp(argv[1], "-h") == 0)
    action = act_help_full;

  // Process the RPC action arguments
  if ((argc == 4 || argc == 5) && argv[1][0] == '-') {
    switch (argv[1][1]) {
      case 'u':
        // user wants to upload a file to a server
	action = act_upload;
        break;
      case 'd':
        // user wants to download a file from a server
	action = act_download;
        break;
      default:
        // invalid action
	fprintf(stderr, "!--Error 2: Invalid action: %s\n\n", argv[1]);
	return act_invalid;
    }
    rmt_host = argv[2]; // set the remote host name

    if (strcmp(argv[3], "-i") == 0) {
      // User wants to choose the source & target file names in the interactive mode
      action |= act_interact; // add 1 bit indicating the interaction action
      filename_src = filename_trg = NULL; // reset filenames, because they'll be set interactively later
    }
    else {
      // User wants to upload or download file, set filenames that were passed through the command line
      filename_src = argv[3]; // set the source file name
      filename_trg = argv[4]; // set the target file name
    }
  }

  // User specified an invalid target filename on a remote server for the upload operation
  if (action == act_upload && filename_trg[0] != '/') {
    fprintf(stderr, "!--Error 4: an invalid target filename has passed for the upload operation.\n"
      "Please specify the full path+name for the file on the remote host.\n\n");
    return act_invalid;
  }

  // User specified an invalid source filename on a remote server for the download operation
  if (action == act_download && argv[3][0] != '/') {
    fprintf(stderr, "!--Error 5: an invalid source filename has passed for the download operation.\n"
      "Please specify the full path+name for the file on the remote host.\n\n");
    return act_invalid;
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
  if (!p_flcont) {
    fprintf(stderr, "!--Error 8: Cannot free the file content. p_flcont=%p\n", p_flcont);
    exit(8);
  }

  // free the file content
  if (p_flcont && p_flcont->t_flcont_val) {
    free(p_flcont->t_flcont_val);
    p_flcont->t_flcont_val = NULL;
    p_flcont->t_flcont_len = 0;
  }
}

/*
 * The Upload File section
 * Error numbers range: 10-19
 */

// Get the file size
unsigned get_file_size(FILE *hfile)
{
  fseek(hfile, 0, SEEK_END);
  unsigned size = ftell(hfile);
  rewind(hfile);
  return size;
}

// Allocate the memory to store the file content.
// A pointer to the allocated memory set to p_fcont and return as a result.
char * alloc_mem_file_cont(FILE *hfile, t_flcont *p_fcont)
{
  // Obtain file size
  p_fcont->t_flcont_len = get_file_size(hfile);

  // Allocate memory to contain the whole file
  p_fcont->t_flcont_val = (char*)malloc(p_fcont->t_flcont_len);
  if (p_fcont->t_flcont_val == NULL) {
    fprintf(stderr, "!--Error 11: memory allocation error, size=%ld\n", p_fcont->t_flcont_len);
    fclose(hfile);
    exit(11);
  }
  return p_fcont->t_flcont_val;
}

// Read the file to get its content for transfering
void read_file(const char *filename, file_inf *fileinf)
{
  // Open the file
  FILE *hfile = fopen(filename, "rb");
  if (hfile == NULL) {
    fprintf(stderr, "!--Error 10: cannot open file '%s' for reading\n"
                    "System error %i: %s\n",
                    filename, errno, strerror(errno));
    exit(10);
  }

  // Allocate the memory to store the file content
  if ( alloc_mem_file_cont(hfile, &fileinf->cont) == NULL )
    return;

  // Read the file content into the buffer
  size_t nch = fread(fileinf->cont.t_flcont_val, 1, fileinf->cont.t_flcont_len, hfile);

  // Check a number of items read and if an error has occurred
  if (nch < fileinf->cont.t_flcont_len || ferror(hfile)) {
    fprintf(stderr, "!--Error 12: file reading error: '%s'.\n"
                    "System error %i: %s\n",
                    filename, errno, strerror(errno));
    fclose(hfile);
    free_file_cont(&fileinf->cont); // free the file content memory in case of error
    exit(12);
  }

  fclose(hfile);
}

// The main function to Upload file
void file_upload(CLIENT *client)
{
  file_inf file_inf; // file info object
  err_inf *srv_errinf; // result from a server - error info
  // TODO: figure out if a memory freeing required for srv_errinf?

  // Set the target file name to the file object
  file_inf.name = (char *)filename_trg;

  // Get the file content and set it to the file object
  read_file(filename_src, &file_inf);

  // Make a file upload to a server through RPC
  srv_errinf = upload_file_1(&file_inf, client);

  // Print a message to standard error indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by upload_file_1().
  if (srv_errinf == (err_inf *)NULL) {
    clnt_perror(client, rmt_host);
    free_file_cont(&file_inf.cont); // free the file content memory in case of error
    exit(13);
  }

  // Check an error that may occur on the server
  if (srv_errinf->num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", srv_errinf->num, srv_errinf->err_inf_u.msg);
    free_file_cont(&file_inf.cont); // free the file content memory in case of error
    exit(14);
  }

  // Okay, we successfully called the remote procedure.

  // Freeing the memory that stores the file content
  free_file_cont(&file_inf.cont);
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
void file_download(CLIENT *client)
{
  // Result from a server - the downloaded file content and error info
  file_err *srv_flerr;

  // Make a file download from a server through RPC
  srv_flerr = download_file_1((char **)&filename_src, client);

  // Print a message to standard error indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by download_file_1().
  if (srv_flerr == (file_err *)NULL) {
    clnt_perror(client, rmt_host);
    exit(20);
  }

  // Okay, we successfully called the remote procedure.

  // Check an error that may occur on the server
  if (srv_flerr->err.num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", 
            srv_flerr->err.num, srv_flerr->err.err_inf_u.msg);
    exit(21);
  }
  
  // Save the remote file content to a local file
  save_file(filename_trg, &srv_flerr->file.cont);

  // Free the local memory with a remote file content
  free_file_cont(&srv_flerr->file.cont);
}

void interact(CLIENT *clnt, enum Action *act)
{
  // Substruct the act_interact from the act argument
  *act &= ~act_interact;

  // Get and set the source & target file names
  if (*act == act_upload) {
    filename_src = "../test/test_trans_files/file_orig.txt";
    filename_trg = "/home/oleh/space/c/studying/linux/rpc/file_transfer/test/test_trans_files/file_5.txt";
  }
  else if (*act == act_download) {
    filename_src = "/home/oleh/space/c/studying/linux/rpc/file_transfer/test/test_trans_files/file_orig.txt";
    filename_trg = "../test/test_trans_files/file_6.txt";
  }
}

// Perform a RPC action
void do_RPC_action(CLIENT *clnt, enum Action act)
{
  if (act & act_interact) { // use bitwise '&' to check if interactive mode has choosen
    interact(clnt, &act); // set the source & target file names in the interactive mode
    do_RPC_action(clnt, act); // recursive call of this function to do the file transfer operation
  }
  else if (act == act_upload)
    file_upload(clnt); // upload file to the server
  else if (act == act_download)
    file_download(clnt); // download file from the server
  else {
    fprintf(stderr, "Unknown program execution mode\n");
    clnt_destroy(clnt); // delete the client object
    exit(7);
  }
}

// Perform a non-RPC action
// The non-RPC actions should be called before setting up the RPC parameters.
void do_non_RPC_action(const char *curr_prg_name, enum Action act)
{
  // Print the short help info and exit with error code
  if (act == act_help_short || act == act_invalid) {
    print_help(curr_prg_name, 0);
    exit(1);
  }

  // Print the full help info and exit with success code
  if (act == act_help_full) {
    print_help(curr_prg_name, 1);
    exit(0);
  }
}

int main(int argc, char *argv[])
{
  // Verify the passed command-line arguments and determine the action
  enum Action action = process_args(argc, argv);

  // Do a non-RPC action
  do_non_RPC_action(argv[0], action);

  // Create the client object
  CLIENT *client = create_client();

  // Do an RPC action
  do_RPC_action(client, action);

  // Delete the client object
  clnt_destroy(client);

  return 0;
}

