#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PortNo 8085
#define MAXBUFFSIZE 100
char url[100] = {};
char data_buffer[1024*1024*4] = {};
char data_content[1024] = {};
char *content_type;
char content[50] = {};
int socket_server,accept_var;

void get_request(char request_url[], char version[])
{
	strcpy(url,"/home/vipraja/Documents/Network systems/ECEN5273/PA2/www");
	strcat(url, request_url);
	printf("%s\n", url);
	FILE *fd = fopen(url, "r");
	if (fd == NULL)
	{
		perror("fopen failed");
    close(accept_var);
		return;
	}
	// Get file size
	printf("fopen done!\n");
	fseek(fd,0,SEEK_END);
	int file_size = ftell(fd);
	fseek(fd,0,SEEK_SET);
	printf("\nfile size %d\n",file_size);
	int fr = fread(data_buffer,1,file_size,fd);
	if (fr<0)
	{
		perror("fread failed");
	}
	printf("fread done fr = %d\n",fr);

	content_type = strchr(request_url, '.');
	printf("content_type %s\n", content_type);

  if (strcmp(content_type, ".html") == 0)
		strcpy(content, "text/html");
	else if (strcmp(content_type, ".txt") == 0)
		strcpy(content, "text/plain");
  else if (strcmp(content_type, ".jpg") == 0)
		strcpy(content, "image/jpg");
	else if (strcmp(content_type, ".png") == 0)
		strcpy(content, "image/png");
	else if (strcmp(content_type, ".gif") == 0)
		strcpy(content, "image/gif");
	else if (strcmp(content_type, ".css") == 0)
		strcpy(content, "text/css");
	else if (strcmp(content_type, ".js") == 0)
		strcpy(content, "application/javascript");

printf("content %s\n", content);
	sprintf(data_content,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n",content, file_size);

 int write_var = write(accept_var,data_content,strlen(data_content));
 if (write_var < 0)
{
	 perror("ERROR writing to socket");
}
else if(write_var > 0)
{
	printf("write_var %d content_length %d\n",write_var,strlen(data_content) );
}
else
{
	printf("Write_var is 0\n");
}

printf("%s\n", data_content);
	 write_var = write(accept_var,data_buffer,file_size);
	 if (write_var < 0)
	 {
		 perror("ERROR writing to socket");
	 }
	 else if(write_var > 0)
	 {
		 printf("write_var %d file_size %d\n",write_var,file_size );
	 }
	 else
	 {
		 printf("Write_var = 0\n" );
	 }
fclose(fd);
close(accept_var);

}

int main()
{

  char buffer[256];
  struct sockaddr_in server_addr, client_addr;


	socket_server = socket(AF_INET,SOCK_STREAM,0);
	if(!(socket_server))
	{
		perror("ERROR opening socket");
	}
	else printf("Successfully created server socket\n");

	server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PortNo);

	if (bind(socket_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
	{
		perror("ERROR on binding");
	}
	else printf("Binding successful\n");

while(1){
	//listen
	if(listen(socket_server,5) < 0)
	{
		perror("ERROR listening");
	}
	else printf("Listening success\n");

int addr_length =  sizeof(client_addr);
	//accept
	accept_var = accept(socket_server, (struct sockaddr *) &client_addr, &addr_length);
  if (accept_var<0)
  {
	  perror("ERROR on accept");
		close(accept_var);
  }

	char buff[1024] = {0};

  int read_var = read(accept_var,buff,sizeof(buff));
	if (read_var < 0)
	{
		perror("ERROR reading from socket");
	}
	printf("***************************************\n");
	printf("%s\n", buff);
	printf("***************************************\n");
	   /* get the first token */
	  char* token = strtok(buff, " \n");

		char request_method[100], request_url[100], request_version[100], connection[100];
		int token_count = 0;

	   /* walk through other tokens */
	   while( token != NULL ) {
	      token_count++;

				if (token_count == 1)
					 strcpy(request_method, token);
				else if (token_count == 2)
			   	 strcpy(request_url, token);
				else if (token_count == 3)
					 strcpy(request_version, token);
	      token = strtok(NULL, " \n");
				if (token_count == 3)
					break;
	   }
		 printf("request_method %s\n", request_method);
		 printf("request_url %s\n", request_url);
		 printf("request_version %s\n", request_version);

	get_request(request_url,request_version);

}
  close(socket_server);

	return 0;

}
