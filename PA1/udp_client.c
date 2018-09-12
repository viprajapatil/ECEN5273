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
#include <errno.h>

#define MAXBUFSIZE 100

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
	long int a = 0;
	struct sockaddr_in remote;              //"Internet socket address structure"
	int buff_size = 100;
  int file_size;

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("unable to create socket");
	}

	//Ask the user to enter the command
	char command[100];

	printf("Enter the appropriate command:\n");
	scanf("%[^\n]s",command);

	/******************
	  sendto() sends immediately.
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	nbytes = sendto(sock, (const char *)command, strlen(command),
        	MSG_CONFIRM, (const struct sockaddr *) &remote,
            	sizeof(remote));
	printf("Message sent from Client\n");

	// Blocks till bytes are received
	struct sockaddr_in from_addr;
	int addr_length = sizeof(struct sockaddr);
	bzero(buffer,sizeof(buffer));

	if (strncmp(command, "get", 3) == 0)
	{
		nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
									MSG_WAITALL, (struct sockaddr *) &from_addr,
									&addr_length);

		file_size = atoi(buffer);
		printf("file_size %s\n", buffer);

		FILE *fp;
		fp = fopen("server_received_file","w+");
		while(a<= file_size)
		{
			a += buff_size;
			if (a%file_size < buff_size)
			{
				buff_size = a%file_size;
			}

			nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
                		0, (struct sockaddr *)&from_addr,
                		&addr_length);
			printf("received bytes %i\n",nbytes);
			if(fwrite(buffer,1,nbytes,fp)<0)
    			{
      				perror("error writting file");
    			}
		}
			printf("File received...\n");
		fclose(fp);
	}
	else printf("Server says\n%s", buffer);

	close(sock);

}
