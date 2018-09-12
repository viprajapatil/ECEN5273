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

#define MAXBUFSIZE 512


int main (int argc, char * argv[] )
{
	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	int buff_size = 100;
	int a = 0;
	size_t file_size;

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

	// Buffer to store data to be sent to the client
	char data_buffer[1024*1024*4];

	//According to the command entered by the user, perform the appropriate task
	if (strncmp(buffer, "get", 3) == 0)
	{
		char *ret = strchr(buffer, ' ');
		FILE *fd = fopen(ret+1,"r");
  		if(fd==NULL)
    		{
      			perror("fopen failed\n");
    		}

		// Get file size
  		fseek(fd,0,SEEK_END);
  		file_size = ftell(fd);
  		fseek(fd,0,SEEK_SET);
		printf("%ld\n",file_size);

		// Copy data from the file into a buffer to transmit it to the client
  		size_t fr;
		char file_size_str[100];
		sprintf(file_size_str,"%ld", file_size);
		strcpy(data_buffer,file_size_str);
		nbytes = sendto(sock, (const char *)data_buffer, MAXBUFSIZE,
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			remote_length);
		printf("nbytes %i\n", nbytes);
		file_size = atoi(data_buffer);

		while(a <= file_size)
		{
			a += buff_size;
			int num = a - file_size;
			if (num > 0)
			{
				printf("In while loop...%ld\n",(a-file_size));
				buff_size = file_size - (a-buff_size);
			}
			printf("buff size %i\n", buff_size);
			fr = fread(data_buffer,buff_size,1,fd);
			if (fr<0)
    			{
      				perror("fread failed\n");
    			}

			nbytes = sendto(sock, (const char *)data_buffer, buff_size,
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			remote_length);

			printf(" send bytes %i\n",nbytes);
		}

		printf("File sent...\n");
		fclose(fd);

	}
	else if (strncmp(buffer, "put", 3) == 0)
	{
		nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
	                MSG_WAITALL, ( struct sockaddr *) &remote,
	                &remote_length);

		file_size = atoi(buffer);
		printf("file_size %s\n", buffer);
		int num_put;
		FILE *fp;
		fp = fopen("client_received_file","w+");
		while(a<= file_size)
		{
			a += buff_size;
			num_put = a-file_size;
			if (num_put > 0)
			{
				buff_size = (a-buff_size)-file_size;
			}
			printf("put byte size %i\n", buff_size);
			nbytes = recvfrom(sock, (char *)buffer, buff_size,
		                MSG_WAITALL, ( struct sockaddr *) &remote,
		                &remote_length);
			printf("received bytes %i\n",nbytes);
			if(fwrite(buffer,1,nbytes,fp)<0)
    			{
      				perror("error writting file");
    			}
		}
		printf("File received...\n");
		fclose(fp);
	}
	else if (strncmp(buffer, "delete", 3) == 0)
	{
		char *ret = strchr(buffer, ' ');
		printf("Deleting %s\n", ret+1);
		remove(ret+1);
	}
	else if (strcmp(buffer, "ls") == 0)
	{
		printf("ls entered. Output:\n");
		FILE *fls = popen("ls>ls_op.txt", "r");
		FILE *fd = fopen("ls_op.txt","r");
  		if(fd==NULL)
    		{
      			perror("fopen failed");
    		}

		// Get file size
  		fseek(fd,0,SEEK_END);
  		size_t file_size_ls = ftell(fd);
  		fseek(fd,0,SEEK_SET);

		// Copy data from the file into a buffer to transmit it to the client
  		size_t fr = fread(data_buffer,file_size_ls,1,fd);
		if (fr<=0)
    		{
      			perror("fread failed\n");
    		}
		printf("%ld\n",file_size_ls);

		// Copy data from the file into a buffer to transmit it to the client
		char file_size_str[100];
		sprintf(file_size_str,"%ld", file_size_ls);
		strcpy(data_buffer,file_size_str);
		nbytes = sendto(sock, (const char *)data_buffer, MAXBUFSIZE,
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			remote_length);
		printf("nbytes %i\n", nbytes);
		file_size = atoi(data_buffer);

		while(a <= file_size_ls)
		{
			a += buff_size;
			int num = a - file_size_ls;
			if (num > 0)
			{
				printf("In while loop...%ld\n",(a-file_size_ls));
				buff_size = file_size_ls - (a-buff_size);
			}
			printf("buff size %i\n", buff_size);
			fr = fread(data_buffer,buff_size,1,fd);
			if (fr<0)
    			{
      				perror("fread failed\n");
    			}

			nbytes = sendto(sock, (const char *)data_buffer, buff_size,
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			remote_length);

			printf(" send bytes %i\n",nbytes);
		}

		printf("File sent...\n");
		fclose(fd);

	}
	else if (strcmp(buffer, "exit") == 0)
	{
		printf("Server exiting...\n");
		exit(0);

	}
	else printf("Incorrect command entered, do nothing.\n");

	printf("Message sent from server\n");

	close(sock);
}
