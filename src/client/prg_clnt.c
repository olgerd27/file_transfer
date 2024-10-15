/*
 * prg_clnt.c: the client program to initiate the remote requests.
 * Errors range: 1-5 (reserve 6-10)
 */
#include <stdio.h>
#include <string.h> 
#include <errno.h>
#include "../common/mem_opers.h"  /* for the memory manipulations */
#include "../common/fs_opers.h"   /* for working with the File System */
#include "../common/file_opers.h" /* for the files manipulations */
#include "../common/logging.h"    /* for logging */
#include "interact.h"             /* for interaction operations */

// Global definitions
static CLIENT *pclient;           // a client handle
static const char *rmt_host;      // a remote host name
static char *filename_src;        // a source file name on a client (if upload) or server (if download) side
static char *filename_trg;        // a target file name on a server (if upload) or client (if download) side
static char *dynamic_src = NULL;  // pointer to dynamically allocated memory to store the source file name
static char *dynamic_trg = NULL;  // pointer to dynamically allocated memory to store the target file name

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
    exit(2);
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

// Upload the File through RPC
static void file_upload()
{
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "Begin: initiate File Upload - local source file:\n  %s", filename_src);
  file_inf fileinf; // file info object
  err_inf *p_err_srv = NULL; // result from a server - error info

  // Init the file name & type
  fileinf.name = fileinf.cont.t_flcont_val = NULL; // init for stable memory allocation
  if ( reset_file_name_type(&fileinf) != 0 )
    exit(3);
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "file name & type was init'ed");

  // Set the target file name to the file object
  strncpy(fileinf.name, filename_trg, LEN_PATH_MAX);
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "target filename was set to:\n  %s", fileinf.name);

  // Get (read) the file content and save it into the file object for transfering
  if ( read_file_cont(filename_src, &fileinf.cont, &p_err_srv) != 0 ) {
    LOG(LOG_TYPE_CLNT, LOG_LEVEL_ERROR, "Error reading the local file:\n  %s", filename_src);
    process_file_error(p_err_srv);
    xdr_free((xdrproc_t)xdr_file_inf, &fileinf);
    exit(4);
  }
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_INFO, "file contents was read, before RPC");

  // Make a file upload to a server through RPC
  p_err_srv = upload_file_1(&fileinf, pclient);
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "RPC operation DONE");

  // Print an error message indicating why an RPC failed.
  // Used after clnt_call(), that is called here by upload_file_1().
  if (p_err_srv == (err_inf *)NULL) {
    LOG(LOG_TYPE_CLNT, LOG_LEVEL_ERROR, "RPC failed - NULL returned");
    clnt_perror(pclient, rmt_host);
    clnt_destroy(pclient);                       // delete the client object
    xdr_free((xdrproc_t)xdr_file_inf, &fileinf); // free the local file info
    exit(5);
  }

  // Check an error that may occur on the server
  if (p_err_srv->num != 0) {
    // Error on a server has occurred. Print error message and die.
    LOG(LOG_TYPE_CLNT, LOG_LEVEL_ERROR, "Server error occurred:\n%s", p_err_srv->err_inf_u.msg);
    fprintf(stderr, "!--Server error %d: %s\n", 
            p_err_srv->num, p_err_srv->err_inf_u.msg);
    clnt_destroy(pclient);                       // delete the client object
    xdr_free((xdrproc_t)xdr_file_inf, &fileinf); // free the local file info
    xdr_free((xdrproc_t)xdr_err_inf, p_err_srv); // free the error info returned from server
    exit(p_err_srv->num);
  }

  // Okay, we successfully called the remote procedure.
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_INFO, "RPC was successful");

  // Free the memory
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG,
    "before memory freeing: &fileinf=%p, p_err_srv=%p", &fileinf, p_err_srv);
  xdr_free((xdrproc_t)xdr_file_inf, &fileinf); // free the local file info
  xdr_free((xdrproc_t)xdr_err_inf, p_err_srv); // free the error info returned from server
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "Done.");
}

// Download the File through RPC
static void file_download()
{
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "Begin: initiate File Download - remote source file:\n  %s", filename_src);

  // Perform a file download from a server through RPC and return the downloaded
  // file info & error info object (error info is filled in if an error occurs)
  file_err *p_flerr_srv = download_file_1((char **)&filename_src, pclient);
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "RPC operation DONE");

  // Print an error message indicating why an RPC failed.
  // Used after clnt_call(), that is called here by download_file_1().
  if (p_flerr_srv == (file_err *)NULL) {
    LOG(LOG_TYPE_CLNT, LOG_LEVEL_ERROR, "RPC failed - NULL returned");
    clnt_perror(pclient, rmt_host);
    clnt_destroy(pclient); // delete the client object
    exit(5);
  }

  // Check an error that may occur on the server
  if (p_flerr_srv->err.num != 0) {
    // Error on a server has occurred. Print error message and die.
    LOG(LOG_TYPE_CLNT, LOG_LEVEL_ERROR, "Server error occurred:\n  %s", p_flerr_srv->err.err_inf_u.msg);
    fprintf(stderr, "!--Server error %d: %s\n", 
            p_flerr_srv->err.num, p_flerr_srv->err.err_inf_u.msg);
    clnt_destroy(pclient); // delete the client object
    xdr_free((xdrproc_t)xdr_file_err, p_flerr_srv); // free file & error info returned from server
    exit(p_flerr_srv->err.num);
  }

  // Okay, we successfully called the remote procedure.
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_INFO, "RPC was successful, downloaded remote file:\n  %s", p_flerr_srv->file.name);
  
  // Save (write) the remote file content to a local file
  err_inf *p_err_tmp = NULL; // a nulled pointer to an error info structure
  if ( save_file_cont(filename_trg, &p_flerr_srv->file.cont, &p_err_tmp) != 0 ) {
    LOG(LOG_TYPE_CLNT, LOG_LEVEL_ERROR, "Error saving the file:\n  %s", filename_trg);
    process_file_error(p_err_tmp);
    xdr_free((xdrproc_t)xdr_file_err, p_flerr_srv);
    exit(6);
  }
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_INFO, "file contents was saved to:\n  %s", filename_trg);

  // Free the file & error info returned from server
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, 
    "before memory freeing: file_err=%p, file=%p, err=%p", p_flerr_srv, &p_flerr_srv->file, &p_flerr_srv->err);
  xdr_free((xdrproc_t)xdr_file_err, p_flerr_srv);
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "Done.");
}

/*
 * The Pick File section
 * Error numbers range: ??-??
 */
// The main function to Pick (choose) file remotelly through the RPC.
// Also, this function is a wrapper of the pick_file() RPC function call.
file_err * file_select_rmt(picked_file *p_flpkd)
{
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "Begin: initiate File Selection - init filename:\n  %s", p_flpkd->name);

  // Choose a file on the server via RPC and return the choosen file info or an error
  file_err *p_flerr_srv = pick_file_1(p_flpkd, pclient);
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "RPC operation DONE");

  // Print an error message indicating why an RPC failed.
  // Used after clnt_call(), that is called here by download_file_1().
  if (p_flerr_srv == (file_err *)NULL) {
    LOG(LOG_TYPE_CLNT, LOG_LEVEL_ERROR, "RPC failed - NULL returned");
    clnt_perror(pclient, rmt_host);
    clnt_destroy(pclient); // delete the client object
    exit(5);
  }

  // Check an error that may occur on the server
  if (p_flerr_srv->err.num != 0) {
    // Error on a server has occurred. Print error message and die.
    LOG(LOG_TYPE_CLNT, LOG_LEVEL_ERROR, "Server error occurred:\n  %s", p_flerr_srv->err.err_inf_u.msg);
    fprintf(stderr, "!--Server error %d: %s\n", 
            p_flerr_srv->err.num, p_flerr_srv->err.err_inf_u.msg);
    return p_flerr_srv;
  }

  // Okay, we successfully called the remote procedure.
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_INFO, "RPC was successful, selected file:\n  %s", p_flerr_srv->file.name);
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "Done.");
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
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "Begin");
  char ch, ans = getchar(); // get one (first) character
  // If any character + Enter is typed, the newline character left in the buffer is 
  // cleared and the typed character is returned.
  // If only Enter is typed, just return it.
  if (ans != '\n') while ((ch = getchar()) != '\n' && ch != EOF);
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "inputed char (int): '%c' (%d). Done.", ans, (int)ans);
  return ans;
}

// Get user input to confirm a operation.
static int get_user_confirm()
{
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "Begin");
  char ans = get_stdin_char();
  while (ans != 'y' && ans != 'n' && ans != '\n') {
    printf("Incorrect input, please repeat (y/n) [y]: ");
    ans = get_stdin_char();
  }
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, 
      "input %s. Done.", (ans == 'y' || ans == '\n') ? "confirmed" : "NOT confirmed");
  return (ans == 'y' || ans == '\n') ? 0 : 1;
}

// Get the filename and prompt the user to confirm the selection.
// Can be used for all types of selection: Source & Target file on Client & Server.
static char * get_and_confirm_filename(const picked_file *p_flpkd, const char *hostname,
                                       T_pf_select pf_select, char *selected_filename)
{
  do {
    if ( !get_filename_inter(p_flpkd, pf_select, hostname, selected_filename) )
      return NULL;
    printf("'%s'\nDo you really want to select this file? (y/n) [y]: ", selected_filename);
  } while ( get_user_confirm() != 0 );
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
    if ( !get_and_confirm_filename(&(picked_file){".", pk_ftype_source}, "localhost",
                                    select_file, filename_src) ) { return; }

    // Select a Target file on a remote host
    if ( !get_and_confirm_filename(&(picked_file){".", pk_ftype_target}, rmt_host,
                                    file_select_rmt, filename_trg) ) { return; }
  }
  else if (*act & act_download) {
    // Select a Source file on a remote host
    if ( !get_and_confirm_filename(&(picked_file){".", pk_ftype_source}, rmt_host,
                                    file_select_rmt, filename_src) ) { return; }

    // Select a Target file on a local host
    if ( !get_and_confirm_filename(&(picked_file){".", pk_ftype_target}, "localhost",
                                    select_file, filename_trg) ) { return; }
  }

  // Confirm the RPC action after completing all interactive actions.
  // If confirmed, substruct the act_interact action from *act; if not -> doesn't do a substruction 
  // and interact() will be called again by the caller function do_RPC_action().
  print_confirm_trop_msg(act);
  if (get_user_confirm() == 0)
    *act &= ~act_interact;
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "Done.");
}

// Perform a RPC action
static void do_RPC_action(enum Action act)
{
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, 
      "Begin %s", (act & act_interact) ? ", before Interaction" : "");
  // Make the interaction operation: set source & target file names interactively while
  // act_interact is ON. File transfer operation can proceed only when act_interact is OFF.
  while (act & act_interact)
    interact(&act);

  // Make the file transfer operation
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "before File Transfer operation");
  switch (act) {
    case act_upload:
      // upload file to the server
      file_upload(); 
      break;
    case act_download:
      // download file from the server
      file_download();
      break;
    default:
      LOG(LOG_TYPE_CLNT, LOG_LEVEL_ERROR, "Unknown program execution mode");
      fprintf(stderr, "Unknown program execution mode\n");
  }
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "File Transfer completed");

  // Free the memory allocated for file names
  free(dynamic_src);
  free(dynamic_trg);
  LOG(LOG_TYPE_CLNT, LOG_LEVEL_DEBUG, "Done.");
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
  do_RPC_action(action);

  // Delete the client object
  clnt_destroy(pclient);

  return 0;
}

