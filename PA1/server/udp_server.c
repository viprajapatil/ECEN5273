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

typedef struct message
{
	long int sequence;
	char data[100];
}msg;

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

	msg msg_struct;
	int seq = 0;

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

		// Copy data from the file into a buffer to transmit it to the client
		size_t fr;
		char file_size_str[100];
		sprintf(file_size_str,"%ld", file_size);
		strcpy(data_buffer,file_size_str);
		nbytes = sendto(sock, (const char *)data_buffer, MAXBUFSIZE,
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			remote_length);

		file_size = atoi(data_buffer);

    // Get sequence number count
    int seq_count = (file_size/buff_size)+1;

    // Increment sequence number after every successful transfer

		while(a <= file_size)
		{
			sleep(0.1);
			seq += 1;
			a += buff_size;
			int num = a - file_size;
			if (num > 0)
			{
				printf("In while loop...%ld\n",(a-file_size));
				buff_size = file_size - (a-buff_size);
			}
			//printf("buff size %i\n", buff_size);
			fr = fread(data_buffer,buff_size,1,fd);
			if (fr<0)
    			{
      				perror("fread failed\n");
    			}

			//store sequence number, status and data in a struct and send it.
			msg_struct.sequence = seq;
      memcpy(msg_struct.data,data_buffer,sizeof(data_buffer));
			nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			remote_length);
			/*printf("send nbytes %i\n", nbytes);
			printf("send data %s\n", msg_struct.data);
			printf("**************************************************\n");*/
			printf("send sequence number %ld\n", msg_struct.sequence);
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
		printf("file size %ld\n", file_size);
		int num_put;
		FILE *fp;
		fp = fopen("client_received_file","w+");

		while(a<= file_size)
		{
			a += buff_size;
			num_put = a-file_size;
			if (num_put > 0)
			{
				printf("entered the loop...%i\n",num_put);
				buff_size = (a-buff_size)-file_size;
			}
			printf("a %i\n", a);
			printf("buff_size %i\n", buff_size);

			nbytes = recvfrom(sock, (char *)buffer, buff_size,
		                MSG_WAITALL, ( struct sockaddr *) &remote,
		                &remote_length);
			printf("received nbytes %i\n", nbytes);
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
		strcpy(data_buffer, "Deleting file");
		nbytes = sendto(sock, (const char *)data_buffer, MAXBUFSIZE,
						MSG_CONFIRM, (const struct sockaddr *) &remote,
								remote_length);
	}
	else if (strcmp(buffer, "ls") == 0)
	{
		FILE *fls = popen("ls>ls_op.txt", "r");
		sleep(1);
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
		if (fr<0)
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
		file_size = atoi(data_buffer);

		while(a <= file_size_ls)
		{
			a += buff_size;
			int num = a - file_size_ls;
			if (num > 0)
			{
				printf("In while loop...%i\n",(num));
				buff_size = file_size_ls - (a-buff_size);
			}

			fr = fread(data_buffer,buff_size,1,fd);
			if (fr<0)
    			{
      				perror("fread failed\n");
    			}

			nbytes = sendto(sock, (const char *)data_buffer, buff_size,
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			remote_length);
		}

		printf("File sent...\n");
		fclose(fd);

	}
	else if (strcmp(buffer, "exit") == 0)
	{
		strcpy(data_buffer, "exiting");
		nbytes = sendto(sock, (const char *)data_buffer, MAXBUFSIZE,
						MSG_CONFIRM, (const struct sockaddr *) &remote,
								remote_length);
		printf("Server exiting...\n");
		exit(0);
	}
	else
	{
		strcpy(data_buffer, "Wrong command");
		nbytes = sendto(sock, (const char *)data_buffer, MAXBUFSIZE,
						MSG_CONFIRM, (const struct sockaddr *) &remote,
								remote_length);
		printf("Incorrect command entered, do nothing.\n");
	}

	close(sock);
}
