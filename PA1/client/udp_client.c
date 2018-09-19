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

int main (int argc, char * argv[])
{
	// For timeout


	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
	long int a = 0;
	long int count_flag = 0;
	struct sockaddr_in remote;              //"Internet socket address structure"
	int buff_size = 1024;
 	int file_size;
	char data_buffer[1024*1024*4];

	struct timeval tv;
tv.tv_sec = 1;
tv.tv_usec = 000000;

	int rec_size;

	msg msg_struct;
	ack_struct msg_struct_ack;

	int seq = 0;

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

  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

	if ((strcmp(command, "ls") == 0)|(strncmp(command, "get", 3) == 0))
	{
		nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
									MSG_WAITALL, (struct sockaddr *) &from_addr,
									&addr_length);

		file_size = atoi(buffer);

		// Calculate count for sequence numbers
		int sequence_count = (file_size/buff_size);

		FILE *fp;
		fp = fopen("server_received_file","w+");

		while(sequence_count >= 0)
		{

			seq += 1;
			printf("enter loop...\n");

		printf("seq %i seq num %ld ack seq num %ld\n", seq, msg_struct.sequence, msg_struct_ack.sequence);
			rec_size =  sizeof(msg_struct);
			if (count_flag  == 0)
			{
				printf("count %i\n", sequence_count);
					a += buff_size;
			}

								printf("a %ld t %ld\n", a, count_flag);
			if (a%file_size < buff_size)
			{
				buff_size = file_size - (a-buff_size);
				rec_size = buff_size+8;
				memset(msg_struct.data, 0, sizeof(msg_struct.data));
			}

			nbytes = recvfrom(sock,&msg_struct, rec_size,
                		MSG_WAITALL, (struct sockaddr *)&from_addr,
                		&addr_length);
		  printf("nbytes recived %i\n", nbytes);
			while (nbytes == -1)
			{
				printf("send bytes not recived...\n");
				msg_struct_ack.sequence = msg_struct.sequence;
				msg_struct_ack.status = NOT_RECEIVED;
				printf("sent ack msg %i\n", msg_struct_ack.status);
				count_flag = 1;
				nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
									MSG_CONFIRM, (const struct sockaddr *) &remote,
											sizeof(remote));

				nbytes = recvfrom(sock,&msg_struct, rec_size,
								                		MSG_WAITALL, (struct sockaddr *)&from_addr,
								                		&addr_length);
				}

				if(nbytes >= 0)
				{
					msg_struct_ack.sequence = msg_struct.sequence;
					msg_struct_ack.status = RECEIVED;
					count_flag = 0;
					nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
										MSG_CONFIRM, (const struct sockaddr *) &remote,
												sizeof(remote));
				}
				else printf("entered NACK loop...\n");

			if (msg_struct_ack.status == RECEIVED)
			{
				count_flag = 0;
				printf("entered fwrite loop...\n");
				if(fwrite(msg_struct.data,1,buff_size,fp)<0)
    				{
      					perror("error writting file");
    				}
						else sequence_count -= 1;
				printf("sequence number %ld\n", msg_struct.sequence);

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
			seq += 1;
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

			//store sequence number, status and data in a struct and send it.
			msg_struct.sequence = seq;
			memcpy(msg_struct.data,data_buffer,sizeof(data_buffer));
			nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			sizeof(remote));
			printf("send sequence number %ld\n", msg_struct.sequence);
			printf("send nbytes %i\n", nbytes);

			// Reliability protocol
			nbytes = recvfrom(sock, &msg_struct_ack, sizeof(msg_struct_ack),
					                MSG_WAITALL, (struct sockaddr *)&from_addr,
													&addr_length);
			printf("received ack seq %ld\n", msg_struct_ack.sequence);
			printf("received ack msg %i\n", msg_struct_ack.status);
			if ((msg_struct_ack.sequence != seq) | ( msg_struct_ack.status != RECEIVED))
			{
				nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
				        			MSG_CONFIRM, (const struct sockaddr *) &remote,
				            		sizeof(remote));
			}


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
