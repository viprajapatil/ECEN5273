File: server.c
Author: Vipraja Patil

Description:
Created a HTTP-server which handles multiple request from the user. After reeceiving a request we create a child thread to complete that request, this way we can handle multiple requests from multiple clients. It handles HTTP/1.1 and HTTP/1.0 requests.
The server receives a request from the client. Then we parse the request and check for the requested data, request type(POST or GET), HTTP version and if the connection is alive or closed. Accordingly the method for the particular request type is called.
Before sending the requested data we send Header information before the file content. In the header information, we mention the HTTP version, content type, content length etc.

installation instructions:
gcc server.c - o server
./server <port number>

Extra cerdit:
1. Handled POST requests  
