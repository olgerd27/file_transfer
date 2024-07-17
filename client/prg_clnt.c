/*
 * prg_clnt.c: the client program to initiate the remote file transfers
 */

#include <stdio.h>
#include <string.h> 
#include <errno.h>
#include "../rpcgen/fltr.h"     /* RPC protocol definitions - created by rpcgen */
#include "../common/fs_opers.h" /* functions for working with the File System */
#include "../common/mem_opers.h" /* function for the memory manipulations */

#define DBG_CLNT 1

// Global definitions
static const char *rmt_host;            // a remote host name
static char filename_src[LEN_PATH_MAX]; // a source file name on a client (if upload) or server (if download) side
static char filename_trg[LEN_PATH_MAX]; // a target file name on a server (if upload) or client (if download) side
extern int errno;                       // global system error number

// The supported program actions
enum Action {
    act_none       = (0 << 0)
  , act_help_short = (1 << 0)
  , act_help_full  = (1 << 1)
  , act_upload     = (1 << 2)
  , act_download   = (1 << 3)
  , act_interact   = (1 << 4)
  , act_invalid    = (1 << 5)
};

// The supported types of help info
enum Help_types { hlp_short, hlp_full };

/*
 * Print the help info
 *
 * Input:
 * this_prg_name - this program name
 * help_type     - type of the help info to be printed
 */
void print_help(const char *this_prg_name, enum Help_types help_type)
{
  // Print a part of the full help info
  if (help_type == hlp_full)
    fprintf(stderr, "The RPC client program that uploads files to and downloads from the remote server.\n\n");

  // Print the mandatory part of help info
  fprintf(stderr, "Usage:\n"
    "%s [-u | -d] [server] [file_src] [file_targ]\n"
    "%s [-h]\n\n", this_prg_name, this_prg_name); 

  // Print a part of the full help info
  if (help_type == hlp_full)
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
      fprintf(stderr, "!--Error 2: Invalid RPC action: %s\n\n", argv[1]);
      return act_invalid;
    }
    rmt_host = argv[2]; // set the remote host name

    if (strcmp(argv[3], "-i") == 0) {
      // User wants to choose the source & target file names in the interactive mode
      action |= act_interact;             // add 1 bit indicating the interaction action
      filename_src[0] = filename_trg[0] = '\0'; // reset filenames, because they'll be set interactively later
    }
    else {
      // User wants to upload or download file, set filenames that were passed through the command line
      strncpy(filename_src, argv[3], LEN_PATH_MAX); // set the source file name
      strncpy(filename_trg, argv[4], LEN_PATH_MAX); // set the target file name
    }
  }

  // User specified an invalid target filename on a remote server for the upload operation
  if (action == act_upload && filename_trg[0] != '/') {
    fprintf(stderr, "!--Error 4: an invalid target filename has passed for the upload operation.\n"
      "Please specify the full path for the file on the remote host.\n\n");
    return act_invalid;
  }

  // User specified an invalid source filename on a remote server for the download operation
  if (action == act_download && argv[3][0] != '/') {
    fprintf(stderr, "!--Error 5: an invalid source filename has passed for the download operation.\n"
      "Please specify the full path for the file on the remote host.\n\n");
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

/*
 * The Upload File section
 * Error numbers range: 10-19
 */

/*
 * Read the file to get its content for transfering.
 *
 * Parameters:
 * filename
 * p_flinf
 *
 * Return value:
 *  0 on success, >0 on failure.
 */
int read_file(const char *filename, file_inf *p_flinf)
{
  // Open the file
  FILE *hfile = fopen(filename, "rb");
  if (hfile == NULL) {
    fprintf(stderr, "!--Error 10: cannot open file '%s' for reading\n"
                    "System error %i: %s\n",
                    filename, errno, strerror(errno));
    return 10;
  }

  // Allocate the memory to store the file content
  p_flinf->cont.t_flcont_val = NULL; // needed to allocate memory
  if ( !alloc_file_cont(&p_flinf->cont, get_file_size(hfile)) ) {
    fclose(hfile);
    return 11;
  }

  // Read the file content into the buffer
  size_t nch = fread(p_flinf->cont.t_flcont_val, 1, p_flinf->cont.t_flcont_len, hfile);

  // Check a number of items read and if an error has occurred
  if (ferror(hfile) || nch < p_flinf->cont.t_flcont_len) {
    fprintf(stderr, "!--Error 12: file reading error: '%s'.\n"
                    "System error %i: %s\n",
                    filename, errno, strerror(errno));
    fclose(hfile);
    free_file_cont(&p_flinf->cont); // free the file content memory in case of error
    return 12;
  }

  fclose(hfile);
}

// The main function to Upload file
void file_upload(CLIENT *client)
{
  if (DBG_CLNT) printf("[file_upload] 0\n");
  file_inf file_inf; // file info object
  err_inf *srv_errinf; // result from a server - error info
  // TODO: figure out if a memory freeing required for srv_errinf?

  // Init the file name & type
  reset_file_name_type(&file_inf);
  if (DBG_CLNT) printf("[file_upload] 1\n");
  // Set the target file name to the file object
  strncpy(file_inf.name, filename_trg, strlen(filename_trg) + 1);
  if (DBG_CLNT) printf("[file_upload] 2\n");

  // Get the file content and set it to the file object
  if (read_file(filename_src, &file_inf) != 0) {
    free_file_name(&file_inf.name); // TODO: continue here - why the crash occurred here in case of choosing file_noperm file?
    exit(12);
  }
  if (DBG_CLNT) printf("[file_upload] 3\n");

  // Make a file upload to a server through RPC
  srv_errinf = upload_file_1(&file_inf, client);
  if (DBG_CLNT) printf("[file_upload] 4, RPC operation DONE,\nfilename: '%s'\n", file_inf.name);

  // Print a message to standard error indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by upload_file_1().
  if (srv_errinf == (err_inf *)NULL) {
    clnt_perror(client, rmt_host);
    // free_file_cont(&file_inf.cont); // free the file content memory in case of error
    free_file_inf(&file_inf); // free the file info memory in case of error
    if (DBG_CLNT) printf("[file_upload] 5\n");
    exit(13);
  }
  if (DBG_CLNT) printf("[file_upload] 6\n");

  // Check an error that may occur on the server
  if (srv_errinf->num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", srv_errinf->num, srv_errinf->err_inf_u.msg);
    // free_file_cont(&file_inf.cont); // free the file content memory in case of error
    free_file_inf(&file_inf); // free the file info memory in case of error
    exit(14);
  }

  // Okay, we successfully called the remote procedure.

  if (DBG_CLNT) printf("[file_upload] before file content freeing\n");
  free_file_inf(&file_inf); // free the file info memory
  if (DBG_CLNT) printf("[file_upload] DONE\n");
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

char get_stdin_char()
{
  char ch, ans = getchar(); // get one (first) character
  // Clear the newline character left in the buffer only if the input is not just a newline character
  if (ans != '\n')
    while ((ch = getchar()) != '\n' && ch != EOF);
  return ans;
}

/*
 * The confirmation prompt.
 */
// TODO: change the behaviour from "(y/n) " and incorrect input for just pressing Enter
// to "y/n [y] " when pressing Enter means 'y'
int confirm_operation(enum Action *act)
{
  // Print the prompt message
  printf("%s Request:\n"
         "    Source: %s:%s\n"
         "    Target: %s:%s\n"
         "Confirm this operation? (y/n) ",
    (*act & act_upload ? "Upload" : "Download"),
    (*act & act_upload ? "localhost" : rmt_host), filename_src,
    (*act & act_upload ? rmt_host : "localhost"), filename_trg
  );

  // Get user input
  char ans = get_stdin_char();
  while (ans != 'y' && ans != 'n') {
    printf("Incorrect input, please repeat (y/n): ");
    ans = get_stdin_char();
  }
  if (DBG_CLNT) printf("[confirm_operation] DONE, ans: '%c'\n", ans);
  return (ans == 'y' ? 0 : 1);
}

void interact(CLIENT *clnt, enum Action *act)
{
  // Get and set the source & target file names
  if (*act & act_upload) {
    // strcpy(filename_src, "../test/transfer_files/file_orig.txt");
    if (!get_filename_inter(".", filename_src, sel_ftype_source)) return;
    strcpy(filename_trg, "/home/oleh/space/c/studying/linux/rpc/file_transfer/test/transfer_files/file_4.txt");
  }
  else if (*act & act_download) {
    strcpy(filename_src, "/home/oleh/space/c/studying/linux/rpc/file_transfer/test/transfer_files/file_orig.txt");
    // strcpy(filename_trg, "../test/transfer_files/file_6.txt");
    if (!get_filename_inter(".", filename_trg, sel_ftype_target)) return;
  }

  // Confirm the RPC action after completing all interactive actions.
  // If confirmed, substruct the act_interact action from *act; if not -> doesn't do a substruction 
  // and interact() will be called again by the caller function do_RPC_action().
  if (confirm_operation(act) == 0)
    *act &= ~act_interact;
  if (DBG_CLNT) printf("[interact] DONE\n");
}

// Perform a RPC action
void do_RPC_action(CLIENT *clnt, enum Action act)
{
  // If the interaction action has choosen, set the source & target file names in the interactive mode.
  // Call the interactive mode while the act_interact flag is turned on.
  while (act & act_interact)
    interact(clnt, &act);

  // Make the file transfer operation
  switch (act) {
    case act_upload:
      if (DBG_CLNT) printf("[do_RPC_action] file_upload before\n");
      file_upload(clnt); // upload file to the server
      if (DBG_CLNT) printf("[do_RPC_action] file_upload after\n");
      break;
    case act_download:
      if (DBG_CLNT) printf("[do_RPC_action] file_download before\n");
      file_download(clnt); // download file from the server
      if (DBG_CLNT) printf("[do_RPC_action] file_download after\n");
      break;
    default:
      fprintf(stderr, "Unknown program execution mode\n");
      clnt_destroy(clnt); // delete the client object
      exit(7);
  }
  if (DBG_CLNT) printf("[do_RPC_action] DONE\n");
}

// Perform a non-RPC action
// The non-RPC actions should be called before setting up the RPC parameters.
void do_non_RPC_action(const char *curr_prg_name, enum Action act)
{
  // A short help has choosen - print short help info and exit with error code
  if (act == act_help_short || act == act_invalid) {
    print_help(curr_prg_name, hlp_short);
    exit(1);
  }

  // A full help has choosen - print full help info and exit with success code
  if (act == act_help_full) {
    print_help(curr_prg_name, hlp_full);
    exit(0);
  }
}

int main(int argc, char *argv[])
{
  // Verify the passed command-line arguments and determine the action
  enum Action action = process_args(argc, argv);

  // Do a non-RPC action, if it has choosen
  do_non_RPC_action(argv[0], action);

  // Create the client object
  CLIENT *client = create_client();

  // Do an RPC action
  do_RPC_action(client, action);

  // Delete the client object
  clnt_destroy(client);

  return 0;
}

