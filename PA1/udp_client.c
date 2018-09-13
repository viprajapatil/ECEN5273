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

typedef struct message
{
	int status;
	int sequence;
	char data[100];
}msg;


int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
	long int a = 0;
	struct sockaddr_in remote;              //"Internet socket address structure"
	int buff_size = 100;
  int file_size;
	char data_buffer[1024*1024*4];

	msg msg_struct;

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


	if ((strcmp(command, "ls") == 0)|(strncmp(command, "get", 3) == 0))
	{
		nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
									MSG_WAITALL, (struct sockaddr *) &from_addr,
									&addr_length);

		file_size = atoi(buffer);

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
			printf("received nbytes %i\n",nbytes);
			if(fwrite(buffer,1,nbytes,fp)<0)
    			{
      				perror("error writting file");
    			}
		}
			printf("File received...\n");
		fclose(fp);
	}
	else if (strncmp(command, "put", 3) == 0)
	{
		char *ret = strchr(command, ' ');
		FILE *fd = fopen(ret+1,"r");
  		if(fd==NULL)
    		{
      			perror("fopen failed\n");
    		}

		// Get file size
  		fseek(fd,0,SEEK_END);
  		size_t file_size_put = ftell(fd);
  		fseek(fd,0,SEEK_SET);
		printf("%ld\n",file_size_put);

		// Copy data from the file into a buffer to transmit it to the client
  		size_t fr;
		char file_size_str[100];
		sprintf(file_size_str,"%ld", file_size_put);
		strcpy(data_buffer,file_size_str);
		nbytes = sendto(sock, (const char *)data_buffer, strlen(data_buffer),
		        	MSG_CONFIRM, (const struct sockaddr *) &remote,
		            	sizeof(remote));
		printf("file size %s\n", data_buffer);

		while(a <= file_size_put)
		{
			sleep(0.1);
			a += buff_size;
			int num = a - file_size_put;
			if (num > 0)
			{
				printf("In while loop...%ld\n",(a-file_size_put));
				buff_size = file_size_put - (a-buff_size);
			}

			fr = fread(data_buffer,buff_size,1,fd);
			if (fr<0)
    			{
      				perror("fread failed\n");
    			}

			nbytes = sendto(sock, (const char *)data_buffer, buff_size,
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			sizeof(remote));
		}
	}
	else
	{
		nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
									MSG_WAITALL, (struct sockaddr *) &from_addr,
									&addr_length);
									printf("reecived nbytes %i\n", nbytes);
		printf("Server says %s\n", buffer);
	}

	close(sock);

}
