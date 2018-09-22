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
	printf("Enter the appropriate command:\n");
	scanf("%[^\n]s",command);
	getchar();
	//gets(command);
	/******************
	  sendto() sends immediately.
	  it will report an error if the message fails to leave the computer
	  however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	nbytes = sendto(sock, (const char *)command, strlen(command),
        	MSG_CONFIRM, (const struct sockaddr *) &remote,
            	sizeof(remote));

	// Blocks till bytes are received
	struct sockaddr_in from_addr;
	int addr_length = sizeof(struct sockaddr);
	bzero(buffer,sizeof(buffer));

  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));

	if ((strcmp(command, "ls") == 0)|(strncmp(command, "get", 3) == 0)){

		if (strcmp(command, "ls") == 0) {
			sleep(1);
		}

		nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
									MSG_WAITALL, (struct sockaddr *) &from_addr,
									&addr_length);

		file_size = atoi(buffer);
		printf("file size %s\n",buffer);
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
     seq = 0;
		int flag = 0;
		a = 0;
		while(sequence_count >= 0){
			seq += 1;
			flag = 0;
			rec_size =  sizeof(msg_struct);
			if (count_flag  == 0){
				printf("seq count %i\n", sequence_count);
					a += buff_size;
			}

			if (a%file_size < buff_size){
				buff_size = file_size - (a-buff_size);
				rec_size = buff_size+8;
				memset(msg_struct.data, 0, sizeof(msg_struct.data));
			}

			int nbytes1 = 0;

			while(flag == 0)
			{
				nbytes1 = recvfrom(sock,&msg_struct, rec_size,
                		MSG_WAITALL, (struct sockaddr *)&from_addr,
                      		&addr_length);

		    if(nbytes > 0)
			  {
				  if(msg_struct.sequence == seq)
				  {
					  msg_struct_ack.sequence = msg_struct.sequence;
					  msg_struct_ack.status = RECEIVED;
					  nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
										 MSG_CONFIRM, (const struct sockaddr *) &remote,
												sizeof(remote));
		        if(fwrite(msg_struct.data,1,buff_size,fp)<0)
					  {
						  perror("error writting file");
						  exit(1);
					  }
					  else
						{
							printf("\nNo of bytes written to the file %d and nbytes1 is %d\n",buff_size,nbytes1);
							printf("sequence_count %d\n",sequence_count);
						  sequence_count -= 1;
							flag = 1;
					  }
						}
				else
				{
					msg_struct_ack.sequence = msg_struct.sequence;
					msg_struct_ack.status = NOT_RECEIVED;
					printf("sent ack msg %i\n", msg_struct_ack.status);
					nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
										MSG_CONFIRM, (const struct sockaddr *) &remote,
												sizeof(remote));
					printf("\nNACK WRONG PACKET RECEIVED\n");
				}
				}
			else
			{
				/*msg_struct_ack.sequence = msg_struct.sequence;
				msg_struct_ack.status = NOT_RECEIVED;
				count_flag = 1;
				nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
									MSG_CONFIRM, (const struct sockaddr *) &remote,
											sizeof(remote));*/
				printf("\nTIMEOUT TIMEOUT TIMEOUT\n");
			}
		}
		/*	while (nbytes == -1){
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

				if(nbytes >= 0){
					msg_struct_ack.sequence = msg_struct.sequence;
					msg_struct_ack.status = RECEIVED;
					nbytes = sendto(sock,&msg_struct_ack, sizeof(msg_struct_ack),
										MSG_CONFIRM, (const struct sockaddr *) &remote,
												sizeof(remote));
				}

			if (msg_struct_ack.status == RECEIVED){
				count_flag = 0;
				if(fwrite(msg_struct.data,1,buff_size,fp)<0){
      					perror("error writting file");
								exit(1);
    				}
						else sequence_count -= 1;
			}*/
		}
		printf("File received...\n");
		fclose(fp);
	}
	else if (strncmp(command, "put", 3) == 0)
	{
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
		nbytes = sendto(sock, (const char *)data_buffer, strlen(data_buffer),
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
			int num = a - file_size_put;
			if (num > 0){
				buff_size = file_size_put - (a-buff_size);
				printf("Almost done!\n");
			}

			fr = fread(data_buffer,buff_size,1,fd);
			if (fr<0){
      		perror("fread failed\n");
					exit(1);
				}

				while(ack_flag == 0)
				{
					printf("\nsending file %d\n",seq);
					//store sequence number, status and data in a struct and send it.
					msg_struct.sequence = seq;
		      memcpy(msg_struct.data,data_buffer,sizeof(data_buffer));
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
						}
					}
			       }

			//store sequence number, status and data in a struct and send it.
			/*msg_struct.sequence = seq;
			memcpy(msg_struct.data,data_buffer,sizeof(data_buffer));
			nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
        			MSG_CONFIRM, (const struct sockaddr *) &remote,
            			sizeof(remote));

			// Reliability protocol
			nbytes = recvfrom(sock, &msg_struct_ack, sizeof(msg_struct_ack),
					                MSG_WAITALL, (struct sockaddr *)&from_addr,
													&addr_length);

			while (nbytes == -1) {
				while (nbytes == -1) {
					nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
												MSG_WAITALL, (struct sockaddr *) &from_addr,
												&addr_length);
					printf("nbytes after timeout %i\n", nbytes);
				}
				if ((msg_struct_ack.sequence!= seq) | (msg_struct_ack.status != RECEIVED)) {
					nbytes = sendto(sock, &msg_struct, sizeof(msg_struct),
												MSG_CONFIRM, (const struct sockaddr *) &remote,
													sizeof(remote));
				}
				nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE,
											MSG_WAITALL, (struct sockaddr *) &from_addr,
											&addr_length);
			}*/
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
}
	close(sock);

}
