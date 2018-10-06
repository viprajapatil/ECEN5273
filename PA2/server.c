#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct{
	char string[50];
	int string_length;
	int USR_Led_Control;
}message;

int main()
{
	int socket_server,accept_var;
    char buffer[256];
    struct sockaddr_in server_addr, client_addr;
	int portno = 3000;
	message *ptr;
    message mesg_struct;
	
	socket_server = socket(AF_INET,SOCK_STREAM,0);
	if(!(socket_server))
		printf("ERROR opening socket\n");
	else printf("Successfully created server socket\n");

	server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(portno);
	
	if (bind(socket_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) 
        printf("ERROR on binding\n");
	else printf("Binding successful\n");
	
	//listen
	if(listen(socket_server,5) < 0)
		printf("ERROR listening\n");
	else printf("Listening success\n");
	
	//accept
	accept_var = accept(socket_server, (struct sockaddr *) &client_addr, 0);
     if (accept_var<0) 
          printf("ERROR on accept, %i\n", accept_var);
	  
	char buff[sizeof(message)] = {0};
	
    int read_var = read(accept_var,buff,sizeof(message));
     
	if (read_var < 0) 
		printf("ERROR reading from socket\n");
	ptr = (message*)(buff);
    printf("Server read string: %s, String length = %d, USR led status: %d\n", ptr->string, ptr->string_length, ptr->USR_Led_Control);

	ptr = &mesg_struct;
    strcpy(mesg_struct.string, "Server write message\n");
    mesg_struct.string_length = strlen(mesg_struct.string);
    mesg_struct.USR_Led_Control = 1;
    int write_var = write(accept_var,ptr,sizeof(message));
	if (write_var < 0)
		printf("ERROR writing to socket\n");

    
     
	close(accept_var);
    close(socket_server);
     return 0;   
	

	return 0;
	
}
