/*
 * file : udp_client.c
 * author : Vipraja Patil
 * description: Client sends an appropriate command to the server. Reliability
 * protocol(Stop and wait) has been implemented for efficient transfer of file
 * between the two nodes.
*/

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

int main (int argc, char * argv[])
{
	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
	long int a = 0;
	long int count_flag = 0;
	struct sockaddr_in remote;              //"Internet socket address structure"
	int buff_size;
 	int file_size;
	char data_buffer[1024*1024*4];
	char command[100];

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	int rec_size;

	msg msg_struct;
	ack_struct msg_struct_ack;

	int seq;

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}


	seq = 0;
	count_flag = 0;
	buff_size = 1024;
	a = 0;
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
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("unable to create socket");
	}

	while(1){
	//Ask the user to enter the command
	memset(msg_struct.data, 0, sizeof(msg_struct.data));
	memset(command,0,sizeof(command));
	printf("Enter the appropriate command:\n");
	scanf("%[^\n]s",command);
	getchar();
	//gets(command);

	printf("\ncommand received is %s\n",command);

	// Send sequence number so that if garbage value is received it can be discarded and client waits for the command again
	msg_struct.sequence = 3;
  memcpy(msg_struct.data,command,MAXBUFSIZE);
	printf("%s\n", msg_struct.data);
	/******************
	  sendto() sends immediately.
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	 nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
	 					MSG_CONFIRM, (const struct sockaddr *) &remote,
	 							sizeof(remote));


	// Blocks till bytes are received
	struct sockaddr_in from_addr;
	int addr_length = sizeof(struct sockaddr);
	bzero(buffer,sizeof(buffer));

	// Sets a timeout of 1sec
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

/***************************************************************************
When the user enters "ls", client receives a list of files from the server's
local directory and when "get" is received, client receives the specified
file from the server
***************************************************************************/
	if ((strcmp(command, "ls") == 0)|(strncmp(command, "get", 3) == 0)){

		int timeout_count = 0;
		int ack_count = 0;

		if (strcmp(command, "ls") == 0) {
			sleep(1);
		}

    char file_buffer[MAXBUFSIZE];
		bzero(file_buffer,MAXBUFSIZE);
		while(nbytes == -1 || msg_struct.sequence !=5)
		{
			nbytes = recvfrom(sock,&msg_struct, sizeof(msg_struct)/*rec_size*/,
									MSG_WAITALL, (struct sockaddr *)&from_addr,
												&addr_length);
		}
		memcpy(file_buffer,msg_struct.data,MAXBUFSIZE);

		file_size = atoi(file_buffer);
		printf("file size %s\n",file_buffer);
		if (file_size == 0){
			printf("File does not exist...\n");
			exit(0);
		}

		buff_size = 1024;
		// Calculate count for sequence numbers
		int sequence_count = (file_size/buff_size);

		FILE *fp;
		fp = fopen("server_received_file","w+");
		printf("Sequence count value is %d Receiving file...\n",sequence_count);
     seq = 1;
		int flag = 0;
		a = 0;
		while(sequence_count >= 0){
		//	seq += 1;
			flag = 0;
			rec_size =  sizeof(msg_struct);
			if (count_flag  == 0){
				//printf("seq count %i\n", sequence_count);
					a += buff_size;
			}
			int num = a-file_size;
			if (num > 0){
				buff_size = file_size-(a-buff_size);
				rec_size = buff_size+8;
				memset(msg_struct.data, 0, sizeof(msg_struct.data));
			}

			int nbytes1 = 0;
			/********************************************************************
			flag will be set to 0 when the client receives the data from server
			but, if timeout occurs while receiving flag will be set to 1 and
		  loop will be exceuted again
			********************************************************************/
			while(flag == 0)
			{
				nbytes1 = recvfrom(sock,&msg_struct, sizeof(msg_struct)/*rec_size*/,
                		MSG_WAITALL, (struct sockaddr *)&from_addr,
                      		&addr_length);

		    if(nbytes1 > 0)
			  {
				  if(msg_struct.sequence == seq)
				  {
					  msg_struct_ack.sequence = msg_struct.sequence;
					  msg_struct_ack.status = RECEIVED;
					  nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
										 MSG_CONFIRM, (const struct sockaddr *) &remote,
												sizeof(remote));
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
							printf("\nNo of bytes written to the file %d and nbytes1 is %d\n",buff_size,nbytes1);
							//printf("seq is  %d\n",seq);
						  sequence_count -= 1;
							flag = 1;
							seq++;
					  }
					}
				else
				{
					msg_struct_ack.sequence = seq;//msg_struct.sequence;
					msg_struct_ack.status = NOT_RECEIVED;
				//	printf("sent ack msg %i and packet number is %d and seq is %d\n", msg_struct_ack.status,msg_struct_ack.sequence,seq);
					nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
										MSG_CONFIRM, (const struct sockaddr *) &remote,
												sizeof(remote));
					printf("\nNACK WRONG PACKET RECEIVED\n");
					printf("seq count after nack %ld\n", msg_struct.sequence);
					printf("seq count %i\n", seq);
					ack_count++;
					//exit(0);
				}
				}
			else
			{

				printf("\nTIMEOUT TIMEOUT TIMEOUT\n");
				msg_struct_ack.sequence = seq;//msg_struct.sequence;
				msg_struct_ack.status = NOT_RECEIVED;
			//	printf("sent ack msg %i and packet number is %d and seq is %d\n", msg_struct_ack.status,msg_struct_ack.sequence,seq);
				nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
									MSG_CONFIRM, (const struct sockaddr *) &remote,
											sizeof(remote));
				printf("\nNACK WRONG PACKET RECEIVED\n");
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
	/***************************************************************************
	When the user enters "put", the client sends the specified file to the
	server
	***************************************************************************/
	else if (strncmp(command, "put", 3) == 0)
	{
		int timeout_count = 0;
		int ack_count = 0;
		char *ret = strchr(command, ' ');
		FILE *fd = fopen(ret+1,"r");
  	if(fd==NULL){
      	perror("fopen failed\n");
				strcpy(data_buffer, "File does not exist");
				nbytes = sendto(sock, (const char *)data_buffer, strlen(data_buffer),
				        	MSG_CONFIRM, (const struct sockaddr *) &remote,
				            	sizeof(remote));
				exit(1);
    	}

		// Get file size
  	fseek(fd,0,SEEK_END);
  	size_t file_size_put = ftell(fd);
  	fseek(fd,0,SEEK_SET);

		// Copy data from the file into a buffer to transmit it to the client
  	size_t fr;
		char file_size_str[100];
		sprintf(file_size_str,"%ld", file_size_put);
		strcpy(data_buffer,file_size_str);

		msg_struct.sequence = 4;
		memcpy(msg_struct.data,data_buffer,MAXBUFSIZE);
		nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
							MSG_CONFIRM, (const struct sockaddr *) &remote,
									sizeof(remote));
		printf("Sending file...\n");
		a = 0;
		seq = 0;
		buff_size = 1024;
		int ack_flag = 0;
		while(a <= file_size_put){
			ack_flag = 0;
			seq += 1;
			a += buff_size;
			printf("a %ld\n", a);
			int num = a - file_size_put;
			if (num > 0){
				buff_size = file_size_put - (a-buff_size);
				printf("Almost done! last packet size is %d\n",buff_size);
			}

			fr = fread(data_buffer,buff_size,1,fd);
			if (fr<0){
      		perror("fread failed\n");
					exit(1);
				}

				/*********************************************************************
				Whenever client receives a positive acknowledgment it updates ack_flag
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
					            	sizeof(remote));
			    nbytes = recvfrom(sock, &msg_struct_ack, sizeof(msg_struct_ack),
																						MSG_WAITALL, (struct sockaddr *)&from_addr,
																						&addr_length);

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
																	ack_count++;
								if(msg_struct_ack.sequence == (seq + 1))
								{
									ack_flag = 1;
									printf("nack losss....\n");
								}
								else
								{

									printf("packet loss\n");
								}
							}
						}
					}
					else
					{
						printf("TIMEOUT \n");
						timeout_count++;
					}
			       }


			printf("File sent...\n");
			printf("timeout %i ack %i\n", timeout_count, ack_count);
		}
	}
	else
	/***************************************************************************
	When the user enters some other commands or enters a wrong command,
	appropriate message is received from the server and the message is printed
	***************************************************************************/
	{
		nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
									MSG_WAITALL, (struct sockaddr *) &from_addr,
									&addr_length);
									printf("received nbytes %i\n", nbytes);
		printf("Server says %s\n", buffer);
	}
}
	close(sock);

}
