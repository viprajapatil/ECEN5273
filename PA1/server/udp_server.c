/*
 * file : udp_server.c
 * author : Vipraja Patil
 * description: Server receives commands from client. Reliability
 * protocol(Stop and wait) has been implemented for efficient transfer of file
 * between the two nodes.
*/

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

// Enum defining acknowledgments
enum status{
	RECEIVED = 0,
	NOT_RECEIVED = 1
};

// Struct which contains sequence number of the packet and data
typedef struct message
{
	long int sequence;
	char data[1024];
}msg;

// Struct which contains sequence numebr of the packet and acknowledgment
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

	seq = 0;
	count_flag = 0;
	buff_size = 1024;
	a = 0;
	nbytes = -1;
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

	int cnt = 0;
while(1){
	seq = 0;
	count_flag = 0;
	buff_size = 1024;
	a = 0;
	nbytes = -1;
	printf("entered....\n");
	//waits for an incoming message
	memset(buffer,0,sizeof(buffer));
	nbytes = -1;
	sleep(1);

	// Sets a timeout of 1sec
	setsockopt(sock, SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

	 // Receive command from client
	msg_struct.sequence = 0;
	while(nbytes == -1 || msg_struct.sequence != 3)
	{
		nbytes = recvfrom(sock, &msg_struct, sizeof(msg_struct),
									MSG_WAITALL, ( struct sockaddr *) &remote,
									&remote_length);
	}
	if (msg_struct.sequence == 3)
	{
		memcpy(buffer,msg_struct.data,MAXBUFSIZE);
	}

	printf("The client says %s number of bytes received is %d\n", buffer,nbytes);


	// Buffer to store data to be sent to the client
	char data_buffer[1024*1024*4];

	/*************************************************************************
	Server will send file to the client
	*****************t********************************************************/
	//According to the command entered by the user, perform the appropriate task
	if (strncmp(buffer, "get", 3) == 0){

		char *ret = strchr(buffer, ' ');
		FILE *fd = fopen(ret+1,"r");
  	if(fd==NULL){
				// if fopen fails sends message to client, then exits
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
		bzero(file_size_str,100);
		bzero(data_buffer,MAXBUFSIZE);
		sprintf(file_size_str,"%ld", file_size);
		printf("File size is: %s\n",file_size_str);
		strcpy(data_buffer,file_size_str);

		//send file size to client
		msg_struct.sequence = 5;
		memcpy(msg_struct.data,data_buffer,MAXBUFSIZE);
		nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
						MSG_CONFIRM, (const struct sockaddr *) &remote,
								remote_length);
		printf("Sending file...\n");

		int ack_flag = 0;
		buff_size = 1024;
		while(a <= file_size){
			ack_flag = 0;
			seq += 1;
			a += buff_size;
			int num = a - file_size;
			if (num > 0){
				buff_size = file_size - (a-buff_size);
				printf("Almost done!\n");
			}
			printf("a %i\n", a);

			fr = fread(data_buffer,buff_size,1,fd);
			if (fr<0){
      		perror("fread failed\n");
					exit(1);
    		}

		/*********************************************************************
		Whenever server receives a positive acknowledgment it updates ack_flag
		as 0 and the continues receiving packets otherwise sets it as 0 and
		resends the packet and waits for acknowledgement
		*********************************************************************/
		while(ack_flag == 0)
		{
			printf("\nsending file %d\n",seq);
			//store sequence number, status and data in a struct and send it.
			msg_struct.sequence = seq;
      memcpy(msg_struct.data,data_buffer,buff_size);
			nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			remote_length);
			nbytes = recvfrom(sock, &msg_struct_ack, sizeof(msg_struct_ack),
		                MSG_WAITALL, ( struct sockaddr *) &remote,
		                &remote_length);

			/*******************************************************************
			 if nbytes > 0, that means recvfrom has received something otherwise
			 timeout will occur and it will again send the packet and wait for ack
			 *****************************************************************/
			if(nbytes > 0)
			{
				if((msg_struct_ack.status == RECEIVED) && (msg_struct_ack.sequence == seq) )
				{
				 	ack_flag = 1;
					printf("pack...\n");
				}
				else
				{
					if(msg_struct_ack.status == NOT_RECEIVED)
					{
						if(msg_struct_ack.sequence == (seq+1))
						{
							printf("\nack\nack loss\n");
							ack_flag = 1;
						}
						else
						{
							printf("\npacket loss\n");
						}
					}
				}
			}
			else
			{
				printf("\nTIMEOUT TIMEOUT TIMEOUT\n");
			}
	  }

		}
		printf("File sent...\n");

		fclose(fd);

	}
	/*************************************************************************
	Server will receive file from client
	*************************************************************************/
	else if (strncmp(buffer, "put", 3) == 0){
		int timeout_count = 0;
		int ack_count = 0;
		buff_size = 1024;
		seq = 1;
		int flag = 0;
		a = 0;
		count_flag = 0;

		// Receive file size from client
		while(nbytes == -1 || msg_struct.sequence !=4)
		{
			nbytes = recvfrom(sock, &msg_struct, sizeof(msg_struct),
										MSG_WAITALL, ( struct sockaddr *) &remote,
										&remote_length);
	 	}
		memcpy(buffer,msg_struct.data,MAXBUFSIZE);
		file_size = atoi(buffer);

		if (file_size == 0){
			printf("File does not exist...\n");
			exit(0);
		}

		int sequence_count = (file_size/buff_size);
		int nack = 0;
		int num_put;
		FILE *fp;
		fp = fopen("client_received_file","w+");

		// loop will run until the whole file is received
		while(sequence_count >= 0){

			flag = 0;
			rec_size =  sizeof(msg_struct);

			if (count_flag == 0){
						printf("seq count %i\n", sequence_count);
				a += buff_size;
				}

			num_put = a-file_size;

			// Only receive the remaining bytes from the client
			if (num_put > 0){
				buff_size = file_size-(a-buff_size);
				rec_size = buff_size+8;
				memset(msg_struct.data, 0, sizeof(msg_struct.data));
			}
			int nbytes1 = 0;

			/********************************************************************
			flag will be set to 0 when the server receives the data from client
			but, if timeout occurs while receiving flag will be set to 1 and
		  loop will be exceuted again
			********************************************************************/
			while (flag == 0)
			{

				nbytes1 = recvfrom(sock, &msg_struct, sizeof(msg_struct),
			                MSG_WAITALL, ( struct sockaddr *) &remote,
			                &remote_length);

				if (nbytes1 > 0)
				{
					if (msg_struct.sequence == seq)
					{
						// Send PACK
					 msg_struct_ack.sequence = msg_struct.sequence;
					 msg_struct_ack.status = RECEIVED;
					 nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
 														        	MSG_CONFIRM, (const struct sockaddr *) &remote,
 												            			remote_length);
					 printf("\n\nPACKET RECEIVED\n");
					 printf("seq count after pack %ld\n", msg_struct.sequence);
					 printf("seq count %i\n", seq);
					 if(fwrite(msg_struct.data,1,buff_size,fp)<0)
					 {
						 perror("error writting file");
						 exit(1);
					 }
					 else
					 {
						 printf("No of bytes written to the file %d and nbytes1 is %d\n",buff_size,nbytes1);
						 //printf("msg struct sequence %d seq %i\n",msg_struct.sequence, seq);
						 sequence_count -= 1;
						 flag = 1;
						 seq ++;
					 }
					}
				  else
				  {
						// Send NACK
						msg_struct_ack.sequence = seq;//msg_struct.sequence;
						msg_struct_ack.status = NOT_RECEIVED;
						//printf("sent ack msg %i\n", msg_struct_ack.status);
						nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
																		 MSG_CONFIRM, (const struct sockaddr *) &remote,
																				 remote_length);
						printf("\n\nNACK WRONG PACKET RECEIVED\n");
						printf("seq count after nack %ld\n", msg_struct.sequence);
						printf("seq count %i\n", seq);
						ack_count++;
				//	exit(0);
					}
				}
				else
				{
					// If timeout occurs send NACK to the client
					printf("\nTIMEOUT TIMEOUT TIMEOUT\n");
					msg_struct_ack.sequence = seq;//msg_struct.sequence;
					msg_struct_ack.status = NOT_RECEIVED;
					//printf("sent ack msg %i\n", msg_struct_ack.status);
					nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
																	 MSG_CONFIRM, (const struct sockaddr *) &remote,
																			 remote_length);
					printf("\n\nNACK WRONG PACKET RECEIVED\n");
					printf("seq count after nack %ld\n", msg_struct.sequence);
					printf("seq count %i\n", seq);
					ack_count++;

					timeout_count++;
				}

		}
	}
		printf("File received...\n");
		printf("timeout %i ack %i\n", timeout_count, ack_count);
		fclose(fp);

}
/*************************************************************************
Server will delete the file specified by the client and sends a message
in response
*************************************************************************/
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
/*************************************************************************
Server will send a list of files present in the server's local folder to
the client
*************************************************************************/
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
			// If timeout occurs re-receive the acknowledgment
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
	/*************************************************************************
	When the client sends an exit command, server will send a message to the
	client regarding the exit and Server will gracefully exit
	*************************************************************************/
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
}
	close(sock);

}
