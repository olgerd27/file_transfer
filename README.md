# file_transfer
The file transfer RPC service.

### Task
Implement a simple file transfer RPC service

### Objective
Create a basic RPC service that allows clients to upload and download files to/from a server. 
The server should be capable of storing files and returning them to clients upon request.

### Requirements
**1. File Transfer RPC Interface:**
Define an RPC interface in a `.x` file that includes the following remote procedures:

   - `int upload_file(string filename, opaque file_content)`: Upload a file to the server. 
     Return 1 on success, 0 on failure.
   - `opaque download_file(string filename)`: Download a file from the server. 
     Return the content of the file as an opaque data type.

**2. Server Implementation:**
- [ ] Implement the server-side code for the RPC service. 
- [ ] The server should handle the upload and download operations. 
- [ ] The uploaded files should be stored on the server.

**3. Client Implementation:**
- [ ] Write a simple client program that interacts with the RPC service. 
- [ ] The client should be able to upload a file to the server and download a file from the server.

**4. Error Handling:**
- [ ] Implement appropriate error handling mechanisms for scenarios such as file not found, upload/download failures, etc.

**5. Testing:**
- [ ] Demonstrate the functionality by uploading and downloading files using the client program.

### Note
* Use the appropriate data types for file content, and ensure that the RPC interface definitions are clear and concise.
* Consider security and error scenarios in your implementation.
* The focus is on both the correctness and clarity of the RPC service implementation.

This task assesses the candidate's understanding of RPC, their ability to define and implement an RPC interface, and their overall system design and coding skills. 
Additionally, it allows the interviewer to evaluate error handling and testing practices.

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
