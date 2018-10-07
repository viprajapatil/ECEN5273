#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define PortNo 8088
#define MAXBUFFSIZE 100
int socket_server,accept_var[100];
int i = 0;


void get_request(int accept_var, char request_url[], char version[])
{
	char url[100] = {};
	char data_buffer[1024*1024*4] = {};
	char data_content[1024] = {};
	char *content_type;
	char content[50] = {};
	char error_header[] =
	"HTTP/1.1 500 Internal Server Error\r\n"
	"Content-Type: text/html; charset = UTF-8\r\n\r\n"
	"<!DOCTYPE html>\r\n"
	"<body><center><h1>ERROR 500: Internal Server Error</h1><br>\r\n";

	strcpy(url,"/home/vipraja/Documents/Network systems/ECEN5273/PA2/www");
	strcat(url, request_url);
	printf("%s\n", url);
	FILE *fd = fopen(url, "r");
	if (fd == NULL)
	{
		perror("fopen failed");
		//strcpy(data_content, "HTTP/1.1 500 Internal Server Error");
		printf("header value is: %s\n\n",error_header);
		int write_var = send(accept_var,error_header,strlen(error_header),0);
	  if (write_var < 0)
	  {
	 	  perror("ERROR writing to socket");
	  }
		shutdown(accept_var,SHUT_RDWR);
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
	else if (strcmp(content_type, ".js") == 0)
		strcpy(content, "application/javascript");
	else strcpy(content, "text/css");

printf("content %s\n", content);
	sprintf(data_content,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n",content, file_size);

 int write_var = write(accept_var,data_content,strlen(data_content));
 if (write_var < 0)
{
	 perror("ERROR writing to socket");
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
shutdown(accept_var,SHUT_RDWR);
close(accept_var);
printf("\nComplete\n");
}

int main(int argc, char * argv[])
{

  char buffer[256];
  struct sockaddr_in server_addr, client_addr;
  pid_t child_thread;

	socket_server = socket(AF_INET,SOCK_STREAM,0);
	if(!(socket_server))
	{
		perror("ERROR opening socket");
	}
	else printf("Successfully created server socket\n");

	server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(atoi(argv[1]));

	if (bind(socket_server, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
	{
		perror("ERROR on binding");
	}
	else printf("Binding successful\n");


	//listen
	if(listen(socket_server,5) < 0)
	{
		perror("ERROR listening");
	}
	else printf("Listening success\n");
while(1){
int addr_length =  sizeof(client_addr);
	//accept
	accept_var[i] = accept(socket_server, (struct sockaddr *) &client_addr, &addr_length);
  if (accept_var[i]<0)
  {
	  perror("ERROR on accept");
		close(accept_var[i]);
  }

	char buff[1024] = {0};

  int read_var = read(accept_var[i],buff,sizeof(buff));
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

  child_thread = fork();
	if (child_thread == 0)
	{
		get_request(accept_var[i],request_url,request_version);
		exit(1);
	}
  i++;
	i = i%99;
}
  close(socket_server);

	return 0;

}
