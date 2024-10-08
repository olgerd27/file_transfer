/*
 * prg_clnt.c: the client program to initiate the remote file transfers
 */

#include <stdio.h>
#include <string.h> 
#include <errno.h>
#include "../common/mem_opers.h" /* for the memory manipulations */
#include "../common/fs_opers.h" /* for working with the File System */
#include "../common/file_opers.h" /* for the files manipulations */
#include "interact.h" /* for interaction operations */

#define DBG_CLNT 1

// Global definitions
static CLIENT *pclient;       // a client handle
static const char *rmt_host;  // a remote host name
static char *filename_src; // a source file name on a client (if upload) or server (if download) side
static char *filename_trg; // a target file name on a server (if upload) or client (if download) side
static char *dynamic_src = NULL; // pointer to dynamically allocated memory to store the source file name
static char *dynamic_trg = NULL; // pointer to dynamically allocated memory to store the target file name

extern int errno; // global system error number

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
static void print_help(const char *this_prg_name, enum Help_types help_type)
{
  // Print a part of the full help info
  if (help_type == hlp_full)
    fprintf(stderr, "The RPC client program that uploads files to and downloads from the remote server.\n\n");

  // Print the mandatory part of help info
  fprintf(stderr, "Usage:\n"
    "%s [-u | -d] [server] [file_src] [file_targ]\n"
    "%s [-u | -d] [server] -i\n"
    "%s [-h]\n\n", this_prg_name, this_prg_name, this_prg_name); 

  // Print a part of the full help info
  if (help_type == hlp_full)
    fprintf(stderr,
      "Options:\n"
      "-u         action: upload a file to the remote server\n"
      "-d         action: download a file from the remote server\n"
      "server     a remote server hostname\n"
      "file_src   a source file name on a client (if upload action) or server (if download action) side\n"
      "file_targ  a target file name on a server (if upload action) or client (if download action) side\n"
      "-i         action: use interactive mode to choose the source and target files\n"
      "-h         action: print this help\n"
      "\nExamples:\n"
      "1. Upload the local file /tmp/file to server 'serva' and save it remotely as /tmp/file_upld:\n"
      "%s -u serva /tmp/file /tmp/file_upld\n\n"
      "2. Download the remote file /tmp/file from server 'servb' and save it locally as /tmp/file_down:\n"
      "%s -d servb /tmp/file /tmp/file_down\n\n"
      "3. Choose the local and remote files in interactive mode and make an Upload to server 'servc':\n"
      "%s -u servc -i\n\n"
      "4. Choose the local and remote files in interactive mode and make an Download from server 'servd':\n"
      "%s -d servd -i\n"
      , this_prg_name, this_prg_name, this_prg_name, this_prg_name);
    else
      fprintf(stderr, "To see the extended help info use '-h' option.\n");
}

// Parse & verify the command-line arguments and determine the action
static enum Action process_args(int argc, char *argv[])
{
  enum Action action = act_none;

  // User didn't pass any arguments or specified incorrect number of them: 
  // show a short help info
  if (argc == 1 || argc == 3 || argc > 5) {
    fprintf(stderr, "!--Error 3: Wrong number of arguments\n\n");
    return act_help_short;
  }

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
      
      // Add 1 bit indicating the interaction action
      action |= act_interact;
      
      // Allocate the memory for filenames and set it also to the separate 'dynamic' pointers,
      // that allows to free this memory correctly afterwards
      filename_src = dynamic_src = (char *)malloc(LEN_PATH_MAX);
      filename_trg = dynamic_trg = (char *)malloc(LEN_PATH_MAX);
      filename_src[0] = filename_trg[0] = '\0'; // init filenames, because they'll be set interactively later
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
static CLIENT * create_client()
{
  pclient = clnt_create(rmt_host, FLTRPROG, FLTRVERS, "tcp");
  if (pclient == (CLIENT *)NULL) {
    // Print an error indication why a client handle could not be created.
    // Used when clnt_create() call fails.
    clnt_pcreateerror(rmt_host);
    exit(6);
  }
  return pclient;
}

// Process and print error info related to file operation.
// p_errinf - A pointer to an `err_inf` structure containing error details.
static void process_file_error(err_inf *p_errinf)
{
  fprintf(stderr, "!--Error %d: %s\n", p_errinf->num, p_errinf->err_inf_u.msg);
  xdr_free((xdrproc_t)xdr_err_inf, p_errinf);
}

/*
 * The Upload File section
 * Error numbers range: 10-19
 */
// The main function to Upload file through RPC call
static int file_upload()
{
  if (DBG_CLNT)
    printf("[file_upload] 0, initiate File Upload - source file:\n  '%s'\n", filename_src);
  file_inf fileinf; // file info object
  err_inf *p_err_srv = NULL; // result from a server - error info

  // Init the file name & type
  fileinf.name = NULL; // init with NULL for stable memory allocation
  fileinf.cont.t_flcont_val = NULL; // init with NULL for stable memory allocation
  reset_file_name_type(&fileinf);
  if (DBG_CLNT) printf("[file_upload] 1, file name & type info was init'ed\n");

  // Set the target file name to the file object
  strncpy(fileinf.name, filename_trg, strlen(filename_trg) + 1);
  if (DBG_CLNT) printf("[file_upload] 2\n");

  // Get (read) the file content and save it into the file object for transfering
  if ( read_file_cont(filename_src, &fileinf.cont, &p_err_srv) != 0 ) {
    process_file_error(p_err_srv);
    xdr_free((xdrproc_t)xdr_file_inf, &fileinf);
    return 12;
  }
  if (DBG_CLNT) printf("[file_upload] 3, read file DONE\n");

  // Make a file upload to a server through RPC
  p_err_srv = upload_file_1(&fileinf, pclient);
  if (DBG_CLNT)
    printf("[file_upload] 4, RPC operation DONE, filename:\n  '%s'\n", fileinf.name);

  // Print a message to STDERR indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by upload_file_1().
  if (p_err_srv == (err_inf *)NULL) {
    if (DBG_CLNT) printf("[file_upload] 5, RPC error\n");
    clnt_perror(pclient, rmt_host);
    xdr_free((xdrproc_t)xdr_file_inf, &fileinf); // free the local file info
    return 13;
  }
  if (DBG_CLNT) printf("[file_upload] 6\n");

  // Check an error that may occur on the server
  if (p_err_srv->num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", 
            p_err_srv->num, p_err_srv->err_inf_u.msg);
    xdr_free((xdrproc_t)xdr_file_inf, &fileinf); // free the local file info
    xdr_free((xdrproc_t)xdr_err_inf, p_err_srv); // free the error info returned from server
    clnt_destroy(pclient); // delete the client object
    return 14;
  }

  // Okay, we successfully called the remote procedure.

  // Free the memory
  if (DBG_CLNT)
    printf("[file_upload] 7, RPC call -> OK, before memory freeing, pointers: "
           "&fileinf=%p, p_err_srv=%p\n", &fileinf, p_err_srv);
  xdr_free((xdrproc_t)xdr_file_inf, &fileinf); // free the local file info
  xdr_free((xdrproc_t)xdr_err_inf, p_err_srv); // free the error info returned from server
 
  if (DBG_CLNT) printf("[file_upload] DONE\n");
  return 0;
}

/*
 * The Download File section
 * Error numbers range: 20-29
 */
// The main function to Download file through RPC call
// TODO: check the return codes in this function
static int file_download()
{
  if (DBG_CLNT) printf("[file_download] 0, initiate File Download - source file:\n  '%s'\n", filename_src);

  // Make a file download from a server through RPC and return the downloaded
  // file info & error info object (error info is filled in if an error occurs)
  file_err *p_flerr_srv = download_file_1((char **)&filename_src, pclient);

  if (DBG_CLNT) printf("[file_download] 1, RPC operation DONE\n");

  // Print a message to STDERR indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by download_file_1().
  if (p_flerr_srv == (file_err *)NULL) {
    if (DBG_CLNT) printf("[file_download] 2, RPC error: NULL was returned\n");
    clnt_perror(pclient, rmt_host);
    return 20;
  }

  // Check an error that may occur on the server
  if (p_flerr_srv->err.num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", 
            p_flerr_srv->err.num, p_flerr_srv->err.err_inf_u.msg);
    xdr_free((xdrproc_t)xdr_file_err, p_flerr_srv); // free file & error info returned from server
    clnt_destroy(pclient); // delete the client object
    return 21;
  }

  // Okay, we successfully called the remote procedure.
  if (DBG_CLNT)
    printf("[file_download] 3, RPC is successful, Downloaded file:\n  '%s'\n", p_flerr_srv->file.name);
  
  // Save (write) the remote file content to a local file
  err_inf *p_err_tmp = NULL; // a nulled pointer to an error info structure
  if ( save_file_cont(filename_trg, &p_flerr_srv->file.cont, &p_err_tmp) != 0 ) {
    process_file_error(p_err_tmp);
    xdr_free((xdrproc_t)xdr_file_err, p_flerr_srv);
    return 22;
  }
  if (DBG_CLNT) 
    printf("[file_download] 4, file saved, filename:\n  '%s'\n", filename_trg);

  // Free the file & error info returned from server
  if (DBG_CLNT)
    printf("[file_download] 5, before memory freeing: "
           "file_err=%p, file=%p, err=%p\n",
           p_flerr_srv, &p_flerr_srv->file, &p_flerr_srv->err);
  xdr_free((xdrproc_t)xdr_file_err, p_flerr_srv);

  if (DBG_CLNT) printf("[file_download] DONE\n");
  return 0;
}

/*
 * The Pick File section
 * Error numbers range: ??-??
 */
// The main function to Pick (choose) file remotelly through the RPC call.
// Also, this function is a wrapper of the pick_file() RPC function call.
file_err * file_select_rmt(picked_file *p_flpkd)
{
  if (DBG_CLNT) printf("[file_select_rmt] 0, init filename:\n  '%s'\n", p_flpkd->name);

  // Choose a file on the server via RPC and return the choosen file info or an error
  file_err *p_flerr_srv = pick_file_1(p_flpkd, pclient);
  
  if (DBG_CLNT) printf("[file_select_rmt] 1, RPC operation DONE\n");

  // Print a message to STDERR indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by download_file_1().
  if (p_flerr_srv == (file_err *)NULL) {
    if (DBG_CLNT) printf("[file_select_rmt] 2, RPC error: NULL was returned\n");
    clnt_perror(pclient, rmt_host);
    exit(20); // TODO: change to return NULL?
  }

  // Check an error that may occur on the server
  if (p_flerr_srv->err.num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", 
            p_flerr_srv->err.num, p_flerr_srv->err.err_inf_u.msg);
    xdr_free((xdrproc_t)xdr_file_err, p_flerr_srv); // free the file & error info
    clnt_destroy(pclient); // delete the client object
    exit(21); // TODO: change to return p_flerr_srv?
  }

  // Okay, we successfully called the remote procedure.
  if (DBG_CLNT)
    printf("[file_select_rmt] DONE, RPC was successful, selected file:\n  '%s'\n",
           p_flerr_srv->file.name);
  return p_flerr_srv;
}

// Print the confirmation prompt for the RPC operation to transfer the file.
static void print_confirm_trop_msg(enum Action *act)
{
  printf("\n%s Request:\n"
         "    Source: %s:%s\n"
         "    Target: %s:%s\n"
         "Confirm this operation? (y/n) [y]: ",
    (*act & act_upload ? "Upload" : "Download"),
    (*act & act_upload ? "localhost" : rmt_host), filename_src,
    (*act & act_upload ? rmt_host : "localhost"), filename_trg
  );
}

static char get_stdin_char()
{
  char ch, ans = getchar(); // get one (first) character
  // If just Enter was pressed, return it;
  // If any other character was entered, clear the newline character left
  // in the buffer and return the inputted character. 
  if (ans == '\n') return ans;
  while ((ch = getchar()) != '\n' && ch != EOF);
  return ans;
}

// Get user input to confirm a operation.
static int get_user_confirm()
{
  char ans = get_stdin_char();
  while (ans != 'y' && ans != 'n' && ans != '\n') {
    printf("Incorrect input, please repeat (y/n) [y]: ");
    ans = get_stdin_char();
  }
  if (DBG_CLNT) printf("[confirm_operation] DONE, ans: '%c'\n", ans);
  return (ans == 'y' || ans == '\n') ? 0 : 1;
}

// Get the filename and prompt the user to confirm the selection.
// Can be used for all types of selection: Source & Target file on Client & Server.
static char * get_and_confirm_filename(const picked_file *p_flpkd, const char *hostname,
                                T_pf_select pf_select, char *selected_filename)
{
  do {
    if (!get_filename_inter(p_flpkd, pf_select, hostname, selected_filename))
      return NULL;
    printf("'%s'\nDo you really want to select this file? (y/n) [y]: ", selected_filename);
  } while (get_user_confirm() != 0);
  printf("The %s file was successfully selected on %s.\n",
         get_pkd_ftype_name(p_flpkd->pftype), hostname);
  return selected_filename;
}

static void interact(enum Action *act)
{
  const char *hostname = NULL;

  // Get and set the source & target file names
  if (*act & act_upload) {
    // Select a Source file on a local host
    // strcpy(filename_src, "../test/transfer_files/file_orig.txt"); for debugging
    if ( !get_and_confirm_filename(&(picked_file){".", pk_ftype_source}, "localhost",
                                    select_file, filename_src) ) { return; }

    // Select a Target file on a remote host
    // strcpy(filename_trg, "/home/oleh/space/c/studying/linux/rpc/file_transfer/test/transfer_files/file_4.txt"); // for debugging
    if ( !get_and_confirm_filename(&(picked_file){".", pk_ftype_target}, rmt_host,
                                    file_select_rmt, filename_trg) ) { return; }
  }
  else if (*act & act_download) {
    // Select a Source file on a remote host
    // strcpy(filename_src, "/home/oleh/space/c/studying/linux/rpc/file_transfer/test/transfer_files/file_orig.txt"); // for debugging
    if ( !get_and_confirm_filename(&(picked_file){".", pk_ftype_source}, rmt_host,
                                    file_select_rmt, filename_src) ) { return; }

    // Select a Target file on a local host
    // strcpy(filename_trg, "../test/transfer_files/file_6.txt"); // for debugging
    if ( !get_and_confirm_filename(&(picked_file){".", pk_ftype_target}, "localhost",
                                    select_file, filename_trg) ) { return; }
  }

  // Confirm the RPC action after completing all interactive actions.
  // If confirmed, substruct the act_interact action from *act; if not -> doesn't do a substruction 
  // and interact() will be called again by the caller function do_RPC_action().
  print_confirm_trop_msg(act);
  if (get_user_confirm() == 0)
    *act &= ~act_interact;
  if (DBG_CLNT) printf("[interact] DONE\n");
}

// Perform a RPC action
static int do_RPC_action(enum Action act)
{
  // Make the interaction operation: set source & target file names interactively while
  // act_interact is ON. File transfer operation can proceed only when act_interact is OFF.
  while (act & act_interact)
    interact(&act);

  // Make the file transfer operation
  int rc;
  switch (act) {
    case act_upload:
      // upload file to the server
      rc = file_upload(); 
      break;
    case act_download:
      // download file from the server
      rc = file_download();
      break;
    default:
      fprintf(stderr, "Unknown program execution mode\n");
      rc = 7;
  }

  // Free the memory allocated for file names
  free(dynamic_src);
  free(dynamic_trg);

  return rc;
}

// Perform a non-RPC action
// The non-RPC actions should be called before setting up the RPC parameters.
static void do_non_RPC_action(const char *curr_prg_name, enum Action act)
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
  (void)create_client();

  // Do an RPC action
  (void)do_RPC_action(action);

  // Delete the client object
  clnt_destroy(pclient);

  return 0;
}

