/*
 * prg_clnt.c: the client program to initiate the remote file transfers
 */

#include <stdio.h>
#include <string.h> 
#include <errno.h>
#include "../rpcgen/fltr.h"     /* RPC protocol definitions - created by rpcgen */
#include "../common/mem_opers.h" /* for the memory manipulations */
#include "../common/fs_opers.h" /* for working with the File System */
#include "interact.h" /* for interaction operations */

#define DBG_CLNT 0

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
void print_help(const char *this_prg_name, enum Help_types help_type)
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
enum Action process_args(int argc, char *argv[])
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
      filename_src[0] = filename_trg[0] = '\0'; // reset filenames, because they'll be set interactively later
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
CLIENT * create_client()
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

// The main function to Upload file through RPC call
void file_upload()
{
  if (DBG_CLNT) printf("[file_upload] 0\n");
  file_inf fileinf; // file info object
  err_inf *p_err_srv; // result from a server - error info
  // TODO: figure out if a memory freeing required for p_err_srv?

  // Init the file name & type
  fileinf.name = NULL; // init with NULL for stable memory allocation
  fileinf.cont.t_flcont_val = NULL; // init with NULL for stable memory allocation
  reset_file_name_type(&fileinf);
  if (DBG_CLNT) printf("[file_upload] 1\n");

  // Set the target file name to the file object
  strncpy(fileinf.name, filename_trg, strlen(filename_trg) + 1);
  if (DBG_CLNT) printf("[file_upload] 2\n");

  // Get the file content and set it to the file object
  if (read_file(filename_src, &fileinf) != 0) {
    free_file_name(&fileinf.name);
    exit(12);
  }
  if (DBG_CLNT) printf("[file_upload] 3, read file DONE\n");

  // Make a file upload to a server through RPC
  p_err_srv = upload_file_1(&fileinf, pclient);
  if (DBG_CLNT)
    printf("[file_upload] 4, RPC operation DONE, filename:\n  '%s'\n", fileinf.name);

  // Print a message to standard error indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by upload_file_1().
  if (p_err_srv == (err_inf *)NULL) {
    if (DBG_CLNT) printf("[file_upload] 5, RPC error\n");
    clnt_perror(pclient, rmt_host);
    // free_file_cont(&fileinf.cont); // free the file content memory in case of error
    free_file_inf(&fileinf); // free the file info memory in case of error
    exit(13);
  }
  if (DBG_CLNT) printf("[file_upload] 6\n");

  // Check an error that may occur on the server
  if (p_err_srv->num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", 
            p_err_srv->num, p_err_srv->err_inf_u.msg);
    // free_file_cont(&fileinf.cont); // free the file content memory in case of error
    free_file_inf(&fileinf); // free the file info memory in case of error
    clnt_destroy(pclient);   // delete the client object
    exit(14);
  }

  // Okay, we successfully called the remote procedure.

  if (DBG_CLNT) printf("[file_upload] 7, RPC is successful\n");

  // Free the file info memory
  free_file_inf(&fileinf); 
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

// The main function to Download file through RPC call
void file_download()
{
  if (DBG_CLNT) printf("[file_download] 0, filename_src:\n  '%s'\n", filename_src);

  // Result from a server - the downloaded file & error info
  file_err *p_flerr_srv;

  // Make a file download from a server through RPC
  p_flerr_srv = download_file_1((char **)&filename_src, pclient);
  
  if (DBG_CLNT) printf("[file_download] 1, RPC operation DONE\n");

  // Print a message to standard error indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by download_file_1().
  if (p_flerr_srv == (file_err *)NULL) {
    if (DBG_CLNT) printf("[file_download] 2, RPC error: NULL was returned\n");
    clnt_perror(pclient, rmt_host);
    exit(20);
  }

  // Check an error that may occur on the server
  if (p_flerr_srv->err.num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", 
            p_flerr_srv->err.num, p_flerr_srv->err.err_inf_u.msg);
    clnt_destroy(pclient); // delete the client object
    exit(21);
  }

  // Okay, we successfully called the remote procedure.
  
  if (DBG_CLNT)
    printf("[file_download] 3, RPC is successful, Downloaded file:\n  '%s'\n",
      p_flerr_srv->file.name);
  
  // Save the remote file content to a local file
  save_file(filename_trg, &p_flerr_srv->file.cont);
  if (DBG_CLNT) 
    printf("[file_download] 4, file save DONE, filename:\n  '%s'\n", filename_trg);

  // Free the local memory with a remote file content
  free_file_cont(&p_flerr_srv->file.cont);
  if (DBG_CLNT) printf("[file_download] DONE\n");
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

  // Print a message to standard error indicating why an RPC call failed.
  // Used after clnt_call(), that is called here by download_file_1().
  if (p_flerr_srv == (file_err *)NULL) {
    if (DBG_CLNT) printf("[file_select_rmt] 2, RPC error: NULL was returned\n");
    clnt_perror(pclient, rmt_host);
    exit(20);
  }

  // Check an error that may occur on the server
  if (p_flerr_srv->err.num != 0) {
    // Error on a server has occurred. Print error message and die.
    fprintf(stderr, "!--Server error %d: %s\n", 
            p_flerr_srv->err.num, p_flerr_srv->err.err_inf_u.msg);
    clnt_destroy(pclient); // delete the client object
    exit(21);
  }

  // Okay, we successfully called the remote procedure.

  if (DBG_CLNT)
    printf("[file_select_rmt] 3, RPC was successful, selected file:\n  '%s'\n",
           p_flerr_srv->file.name);

  // Free the local memory with a remote file content
  // free_file_cont(&p_flerr_srv->file.cont);
  if (DBG_CLNT) printf("[file_select_rmt] DONE\n");

  return p_flerr_srv;
}

/*
 * The confirmation prompt.
 */
void print_confirm_msg(enum Action *act)
{
  // Print the prompt message
  printf("\n%s Request:\n"
         "    Source: %s:%s\n"
         "    Target: %s:%s\n"
         "Confirm this operation? (y/n) [y]: ",
    (*act & act_upload ? "Upload" : "Download"),
    (*act & act_upload ? "localhost" : rmt_host), filename_src,
    (*act & act_upload ? rmt_host : "localhost"), filename_trg
  );
}

char get_stdin_char()
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
int get_user_confirm()
{
  char ans = get_stdin_char();
  while (ans != 'y' && ans != 'n' && ans != '\n') {
    printf("Incorrect input, please repeat (y/n) [y]: ");
    ans = get_stdin_char();
  }
  if (DBG_CLNT) printf("[confirm_operation] DONE, ans: '%c'\n", ans);
  return (ans == 'y' || ans == '\n') ? 0 : 1;
}

// TODO: complete this function and use it for all 4 cases in interact()
// char * get_and_confirm_filename(const picked_file *p_flpkd, const char *hostname,
//                                 T_pf_select pf_select, char *selected_filename)
// {
//   do {
//     if (!get_filename_inter(p_flpkd, pf_select, hostname, selected_filename))
//       return NULL;
//     printf("%s\nDo you really want to select the following file? (y/n) [y]: ", selected_filename);
//   } while (get_user_confirm() != 0);
//   printf("The %s file was successfully selected on %s.\n", 
//     p_flpkd->pftype == pk_ftype_source ? "Source" : "Target", hostname);
//   return selected_filename;
// }

void interact(enum Action *act)
{
  const char *hostname = NULL;

  // Get and set the source & target file names
  if (*act & act_upload) {
    // Select a Source file on a local host
    // strcpy(filename_src, "../test/transfer_files/file_orig.txt"); // for debugging
    do {
      hostname = "localhost";
      if (!get_filename_inter(&(picked_file){".", pk_ftype_source}, 
                              select_file, hostname, filename_src))
        return;
      printf("Do you really want to select the following file?\n'%s' (y/n) [y]: ", filename_src);
    } while (get_user_confirm() != 0);
    printf("The Source file was successfully selected on %s.\n", hostname);
    // if ( !get_and_confirm_filename(&(picked_file){".", pk_ftype_source}, "localhost",
    //                                 select_file, filename_src) ) { return; }
    
    // Select a Target file on a remote host
    // strcpy(filename_trg, "/home/oleh/space/c/studying/linux/rpc/file_transfer/test/transfer_files/file_4.txt"); // for debugging
    do {
      hostname = rmt_host;
      if (!get_filename_inter(&(picked_file){".", pk_ftype_target}, 
                              file_select_rmt, hostname, filename_trg))
        return;
      printf("Do you really want to select the following file?\n'%s' (y/n) [y]: ", filename_trg);
    } while (get_user_confirm() != 0);
    printf("The Target file was successfully selected on %s.\n", hostname);
  }
  else if (*act & act_download) {
    // Select a Source file on a remote host
    // strcpy(filename_src, "/home/oleh/space/c/studying/linux/rpc/file_transfer/test/transfer_files/file_orig.txt");
    if (!get_filename_inter(&(picked_file){".", pk_ftype_source}, 
                            file_select_rmt, rmt_host, filename_src))
      return;
    
    // Select a Target file on a local host
    // strcpy(filename_trg, "../test/transfer_files/file_6.txt");
    if (!get_filename_inter(&(picked_file){".", pk_ftype_target},
                            select_file, "localhost", filename_trg))
      return;
  }

  // Confirm the RPC action after completing all interactive actions.
  // If confirmed, substruct the act_interact action from *act; if not -> doesn't do a substruction 
  // and interact() will be called again by the caller function do_RPC_action().
  print_confirm_msg(act);
  if (get_user_confirm() == 0)
    *act &= ~act_interact;
  if (DBG_CLNT) printf("[interact] DONE\n");
}

// Perform a RPC action
void do_RPC_action(enum Action act)
{
  // Make the interaction operation: set source & target file names interactively while
  // act_interact is ON. File transfer operation can proceed only when act_interact is OFF.
  while (act & act_interact)
    interact(&act);

  // Make the file transfer operation
  switch (act) {
    case act_upload:
      if (DBG_CLNT) printf("[do_RPC_action] before file_upload\n");
      file_upload(); // upload file to the server
      if (DBG_CLNT) printf("[do_RPC_action] after file_upload\n");
      break;
    case act_download:
      if (DBG_CLNT) printf("[do_RPC_action] before file_download\n");
      file_download(); // download file from the server
      if (DBG_CLNT) printf("[do_RPC_action] after file_download\n");
      break;
    default:
      fprintf(stderr, "Unknown program execution mode\n");
      clnt_destroy(pclient); // delete the client object
      exit(7);
  }

  // Free the memory allocated for file names
  free(dynamic_src);
  free(dynamic_trg);

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
  (void)create_client();

  // Do an RPC action
  do_RPC_action(action);

  // Delete the client object
  clnt_destroy(pclient);

  return 0;
}

