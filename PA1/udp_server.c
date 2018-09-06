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

#define MAXBUFSIZE 100

int main (int argc, char * argv[] )
{
	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("unable to create socket");
	}


	/******************
	  Once we've created a socket, we must bind that socket to the 
	  local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		perror("unable to bind socket");
	}

	remote_length = sizeof(remote);

	//waits for an incoming message
	bzero(buffer,sizeof(buffer));
	nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE, 
                MSG_WAITALL, ( struct sockaddr *) &remote,
                &remote_length);

	printf("The client says %s\n", buffer);
	
	//According to the command entered by the user, perform the appropriate task
	if (strncmp(buffer, "get", 3) == 0)
	{
		printf("get entered\n");
		char *ret = strchr(buffer, ' ');
		FILE *fd = fopen(ret+1, "r");
		if (fd == NULL)
			perror("fopen");
	}
	else if (strncmp(buffer, "put", 3) == 0)
	{
		printf("put entered\n");
	}
	else if (strncmp(buffer, "delete", 3) == 0)
	{
		printf("delete entered\n");
	}
	else if (strcmp(buffer, "ls") == 0)
	{
		printf("ls entered. Output:\n");
		system(buffer);
	}
	else if (strcmp(buffer, "exit") == 0)
	{
		printf("Server exiting...\n");
		exit(0);

	}
	else printf("Incorrect command entered, do nothing.\n");

	char msg[] = "orange";
	nbytes = sendto(sock, (const char *)msg, strlen(msg), 
        	MSG_CONFIRM, (const struct sockaddr *) &remote,
            remote_length);
	printf("Message sent from server\n");

	close(sock);
}

