#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

enum DFS_server{
  DFS1 = 1,
  DFS2 = 2,
  DFS3 = 3,
  DFS4 = 4,
};

int server_num;
int socket_server,accept_var[100], socket_server_proxy;
struct sockaddr_in server_addr, client_addr;
int i = 0;


int main(int argc, char **argv)
{
    int socket_server;
    int port = atoi(argv[1]);
    if (argc < 3)
    {
        printf("Less args <directory> <portno>\n");
        exit(-1);
    }
    socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_server < 0)
    {
        perror("Error creating the socket");
        exit(-1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(socket_server, (struct sockaddr *) &server_addr,sizeof(server_addr)) < 0)
    {
        perror("Error on binding");
    }
    else printf("Bind Sucessful\n");

    if(listen(socket_server ,5) < 0)
	  {
		    perror("ERROR listening");
	  }
	  else printf("Listen success\n");
    int addr_length = sizeof(client_addr);
    accept_var[i] = accept(socket_server, (struct sockaddr *) &client_addr, &addr_length);
    if (accept_var[i]<0)
    {
	       perror("ERROR on accept");
		     close(accept_var[i]);
    }
    else printf("Accept complete\n");
    return 0;
}
