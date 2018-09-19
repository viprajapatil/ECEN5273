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

enum status{
	RECEIVED = 0,
	NOT_RECEIVED = 1
};

typedef struct message
{
	long int sequence;
	char data[1024];
}msg;

typedef struct ack
{
	long int sequence;
	int status;
}ack_struct;

int main (int argc, char * argv[] )
{
	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	int buff_size;
	int a;
	size_t file_size;

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	msg msg_struct;
	ack_struct msg_struct_ack;
	int rec_size;

	int seq;
	int count_flag;

	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}
//while (1) {
	seq = 0;
	count_flag = 0;
	buff_size = 1024;
	a = 0;
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

	setsockopt(sock, SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

	// Buffer to store data to be sent to the client
	char data_buffer[1024*1024*4];

	//According to the command entered by the user, perform the appropriate task
	if (strncmp(buffer, "get", 3) == 0){
		char *ret = strchr(buffer, ' ');
		FILE *fd = fopen(ret+1,"r");
  	if(fd==NULL){
      	perror("fopen failed\n");
				strcpy(data_buffer, "File does not exist");
				nbytes = sendto(sock, (const char *)data_buffer, MAXBUFSIZE,
		        			MSG_CONFIRM, (const struct sockaddr *) &remote,
		            			remote_length);
				exit(1);
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
		printf("Sending file...\n");

		while(a <= file_size){
			seq += 1;
			a += buff_size;
			int num = a - file_size;
			if (num > 0){
				buff_size = file_size - (a-buff_size);
				printf("Almost done!\n");
			}
			//printf("buff size %i\n", buff_size);
			fr = fread(data_buffer,buff_size,1,fd);
			if (fr<0){
      		perror("fread failed\n");
					exit(1);
    		}

			//store sequence number, status and data in a struct and send it.
			msg_struct.sequence = seq;
      memcpy(msg_struct.data,data_buffer,sizeof(data_buffer));
			nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			remote_length);

			nbytes = recvfrom(sock, &msg_struct_ack, sizeof(msg_struct_ack),
		                MSG_WAITALL, ( struct sockaddr *) &remote,
		                &remote_length);

			while (nbytes == -1){
			// Reliability protocol
				while (nbytes == -1){
					nbytes = recvfrom(sock, &msg_struct_ack, sizeof(msg_struct_ack),
			                MSG_WAITALL, ( struct sockaddr *) &remote,
			                &remote_length);
					}
				if ((msg_struct_ack.sequence != seq) | ( msg_struct_ack.status != RECEIVED)){
					nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
		        			MSG_CONFIRM, (const struct sockaddr *) &remote,
		            			remote_length);
											printf("resending data \n");
					}
				nbytes = recvfrom(sock, &msg_struct_ack, sizeof(msg_struct_ack),
			                MSG_WAITALL, ( struct sockaddr *) &remote,
			                &remote_length);
			}
		}
		printf("File sent...\n");
		fclose(fd);

	}
	else if (strncmp(buffer, "put", 3) == 0){
		nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
	                MSG_WAITALL, ( struct sockaddr *) &remote,
	                &remote_length);

		file_size = atoi(buffer);

		if (file_size == 0){
			printf("File does not exist...\n");
			exit(0);
		}

		int sequence_count = (file_size/buff_size);

		int num_put;
		FILE *fp;
		fp = fopen("client_received_file","w+");

		while(sequence_count >= 0){
			seq +=1;
			rec_size =  sizeof(msg_struct);

			if (count_flag == 0){
				a += buff_size;
				}

			num_put = a-file_size;
			if (num_put > 0){
				buff_size = file_size-(a-buff_size);
				rec_size = buff_size+8;
				memset(msg_struct.data, 0, sizeof(msg_struct.data));
			}

			nbytes = recvfrom(sock, &msg_struct, rec_size,
		                MSG_WAITALL, ( struct sockaddr *) &remote,
		                &remote_length);
			printf("Receiving file...\n");

			while (nbytes == -1){
				msg_struct_ack.sequence = msg_struct.sequence;
			  msg_struct_ack.status = NOT_RECEIVED;
				count_flag = 1;
				nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
													        	MSG_CONFIRM, (const struct sockaddr *) &remote,
											            			remote_length);

				nbytes = recvfrom(sock,&msg_struct, rec_size,
																										0, ( struct sockaddr *) &remote,
																		                &remote_length);
				}

			if (nbytes >= 0){
				msg_struct_ack.sequence = msg_struct.sequence;
				msg_struct_ack.status = RECEIVED;
				count_flag = 0;
				nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
																		MSG_CONFIRM, (const struct sockaddr *) &remote,
																				remote_length);
				}

			if (msg_struct_ack.status == RECEIVED){
				count_flag = 0;
				if(fwrite(msg_struct.data,1,buff_size,fp)<0){
						perror("error writting file");
						exit(1);
					}
					else sequence_count -= 1;
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

		if(fd==NULL){
      	perror("fopen failed");
				exit(1);
    	}

		// Get file size
  	fseek(fd,0,SEEK_END);
  	size_t file_size_ls = ftell(fd);
  	fseek(fd,0,SEEK_SET);

		// Copy data from the file into a buffer to transmit it to the client
  	size_t fr;

		// Copy data from the file into a buffer to transmit it to the client
		char file_size_str[100];
		sprintf(file_size_str,"%ld", file_size_ls);
		strcpy(data_buffer,file_size_str);
		nbytes = sendto(sock, (const char *)data_buffer, MAXBUFSIZE,
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			remote_length);
		printf("data buffer %s\n", data_buffer);

		printf("Sending file list....\n");

		while(a <= file_size_ls)
		{
			seq += 1;
			a += buff_size;
			int num = a - file_size_ls;
			if (num > 0){
				printf("In while loop...%i\n",(num));
				buff_size = file_size_ls - (a-buff_size);
			}

			fr = fread(data_buffer,buff_size,1,fd);
			if (fr<0){
      		perror("fread failed\n");
					exit(1);
    		}

			msg_struct.sequence = seq;
			memcpy(msg_struct.data,data_buffer,sizeof(data_buffer));
			nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
									        			MSG_CONFIRM, (const struct sockaddr *) &remote,
									            			remote_length);
			// Reliability protocol
			nbytes = recvfrom(sock, &msg_struct_ack, sizeof(msg_struct_ack),
																									MSG_WAITALL, ( struct sockaddr *) &remote,
																									&remote_length);

	    while (nbytes == -1) {
	    	while (nbytes == -1) {
					nbytes = recvfrom(sock, &msg_struct_ack, sizeof(msg_struct_ack),
																											MSG_WAITALL, ( struct sockaddr *) &remote,
																											&remote_length);
	    	}
				if ((msg_struct_ack.sequence != seq) | ( msg_struct_ack.status != RECEIVED))
				{
				nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
																								MSG_CONFIRM, (const struct sockaddr *) &remote,
																										remote_length);
				}
				nbytes = recvfrom(sock, &msg_struct_ack, sizeof(msg_struct_ack),
																										MSG_WAITALL, ( struct sockaddr *) &remote,
																										&remote_length);
	    }
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
		sprintf(data_buffer, "Wrong command: %s",buffer);
		nbytes = sendto(sock, (const char *)data_buffer, MAXBUFSIZE,
						MSG_CONFIRM, (const struct sockaddr *) &remote,
								remote_length);
		printf("Incorrect command entered, do nothing.\n");
	}

	close(sock);
//}
}
