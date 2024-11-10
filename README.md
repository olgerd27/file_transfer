# file_transfer
File Transfer RPC Client-Server System.

A simple client-server RPC (Remote Procedure Call) service for file transfers. This system allows a client to upload and download files to/from a remote server via RPC calls.

## Task
Implement a basic RPC service to facilitate file uploads and downloads between a client and server. Also add the RPC-based Interactive Mode, which provides clients with a dynamic and flexible interface for selecting files on the server.

### Objective
Create an RPC-based service that enables clients to:
1. Upload files to the server.
2. Download files from the server.
3. Select files on the server interactively.

## Requirements
**1. File Transfer RPC Interface:**
Define an RPC interface in a `.x` file that includes the following remote procedures (simplified):

- `int upload_file(string filename, opaque file_content)`: Upload a file to the server. 
  Return 1 on success, 0 on failure.
- `opaque download_file(string filename)`: Download a file from the server. 
  Return the content of the file as an opaque data type.
- `opaque pick_file(string filename)`: Selects a file to operate on in Interactive Mode, streamlining file selection and management.

**2. Server Implementation:**
- [ ] Implement the server-side code for the RPC service. 
- [ ] The Server should handle the Upload and Download operations.
- [ ] The Server should support the Interaction operation with its File System.
- [ ] The uploaded files should be stored on the server.

**3. Client Implementation:**
A command-line Client that:
- [ ] Uploads a specified file to the Server. 
- [ ] Downloads a specified file from the Server.
- [ ] Provides interactive mode for file selection.
- [ ] Verifies actions with user confirmation prompts.

**4. Error Handling:**
- [ ] Implement appropriate error handling mechanisms for scenarios such as file not found, Upload/Download failures, access issues, etc.

**5. Testing:**
- [ ] Demonstrate the functionality by Uploading and Downloading files using the client program.

## Client usage
```
Usage:
  prg_clnt [-u | -d] [server] [file_src] [file_targ]
  prg_clnt [-u | -d] [server] -i
  prg_clnt [-h]
```
Options:
* -u: Upload a file to the remote server.
* -d: Download a file from the remote server.
* server: The hostname of the remote server.
* file_src: Source file name on the Client (for Upload) or Server (for Download).
* Target file name on the Server (for Upload) or Client (for Download).
* -i: Interactive mode to select source and target files.
* -h: Display help information.

### Examples:
- Upload a Local File:
  Command:
  ```
  prg_clnt -u serva /tmp/local_file /tmp/remote_file
  ```
  Uploads the local file `/tmp/local_file` to the Server `serva` and saves it as `/tmp/remote_file` on the Server.
  
- Download a Remote File:
  Command:
  ```
  prg_clnt -d servb /tmp/remote_file /tmp/local_file
  ```
  Downloads the file `/tmp/remote_file` from Server `servb` and saves it as `/tmp/local_file` on the Client.
  
- Interactive Mode for File Selection:
  Command:
  ```
  prg_clnt -u servc -i
  ```
  Allows users to select files interactively for Upload to Server `servc`.

### Note
* Use the appropriate data types for file content, and ensure that the RPC interface definitions are clear and concise.
* Consider security and error scenarios in your implementation.
* The focus is on both the correctness and clarity of the RPC service implementation.
* Logging: Configurable logging allows monitoring of Client and Server operations for debugging and auditing.

## Useful admin commands
- Check the status for `rpcbind`:
```
systemctl status rpcbind
```

- Bring up the `rpcbind`:
```
systemctl start rpcbind
```

- Getting the running server program(-s) with the transport protocol, address, and ports which it's used:
```
netstat -tulpn | grep [SERVER_NAME]
```

- Getting the RPC info about the running RPC server program(-s) with the transport protocol, address, ports which it's used:
```
rpcinfo
```
OR:
```
rpcinfo -p
```
## What I Learned
* Implemented and utilized TI-RPC on Linux Fedora.
* Defined and implemented an RPC interface in C.
* Applied advanced file system handling techniques on Linux.
* Optimized command-line user interface design and usability.
