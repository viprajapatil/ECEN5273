/*
@file webproxy.c
@author Vipraja Patil
Creates a webproxy to handle requests from client.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <netdb.h>
#include <openssl/md5.h>
#include <time.h>
#include <arpa/inet.h>

#define PortNo 8088
#define MAXBUFFSIZE 100
#define BufferSize 1024
int socket_server,accept_var[100], socket_server_proxy;
int i = 0;
int connection_flag = 0;
int timeout_cache =0;
long double timestamp_req_func;
time_t timestamp_req;
int req_count = 0;

// Define error header
char error_header[] =
"HTTP/1.1 500 Internal Server Error\r\n"
"Content-Type: text/html; charset = UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<body><center><h1>ERROR 403 FORBIDDEN</h1><br>\r\n";
// Error header for blocked websites
char error_header_block[] =
"HTTP/1.1 500 Internal Server Error\r\n"
"Content-Type: text/html; charset = UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<body><center><h1>ERROR 403 FORBIDDEN</h1><br>\r\n";

// Calculates MD5 hash value for caching and searching
char* md5sum_calculate(char *name, int name_size)
{
	unsigned char digest[16];
	MD5_CTX context;
	MD5_Init(&context);
	MD5_Update(&context, name, name_size);
	MD5_Final(digest, &context);

	char* md5string = (char*)malloc(33);
	for(int i = 0; i < 16; ++i)
  sprintf(&md5string[i*2], "%02x", (unsigned int)digest[i]);
	return md5string;
}

// Clears cache after a certain timeout value
void cache_timeout(int t)
{
	time_t timestamp;
	time(&timestamp);
	if ((timestamp-t) >= timeout_cache)
	{
		char *cmd = "cd ~/Documents/Network\\ systems/ECEN5273/PA3/cache;rm *.html";
		system(cmd);
	}
}

/*
@brief Completes GET request
*/
void get_request(int accept_var, char request_url[], char version[], char connection[])
{
	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	char url[100] = {};
	char data_buffer[1024*1024*4] = {};
	char data_content[1024] = {};
	char *content_type;
	char complete_path[1000];
	char cache_file_path[10000];
	char content[50] = {};
	char* addr;

//	printf("requested url is: %s\n", request_url);
	request_url = strstr(request_url,"//");
	request_url = request_url + 2;
	strcpy(complete_path,request_url);
	strcpy(cache_file_path,complete_path);

	char cache[1000];
	strcpy(cache, complete_path);
//	printf("requested website is: %s\n\n",complete_path);
	request_url = strtok(complete_path,"/");
//	printf("\n\nREQUEST URL IS:%s\n\n",request_url);
	struct sockaddr_in server_addr_proxy;
	struct hostent *lh = gethostbyname(request_url);
	memcpy(&server_addr_proxy.sin_addr,lh->h_addr,lh->h_length);
	addr = inet_ntoa(server_addr_proxy.sin_addr);
	printf("addr --->  %s\n", addr);
	char *request_url_ptr = request_url;

	//struct hostent *lh_s = gethostbyname(request_url);

	//For blocking
	char* ret;
	FILE *block_fd  = fopen("blocked.txt","r");
	char line_block[1000];
	while(!feof(block_fd)){
		fgets(line_block,200,block_fd);
		line_block[strlen(line_block)-1] = '\0';
		ret = strstr(request_url_ptr,line_block);
		if (ret != NULL)
		{
			printf("\nERROR: Blocked Website\n");
			write(accept_var, error_header_block, strlen(error_header_block));
			return;
		}
	}
	fclose(block_fd);

	content_type = strchr(request_url, '.');
// parsing the requested data for getting content type
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

	char cache_read_buffer[1024*1024];
	memset(cache_read_buffer,'\0',sizeof(cache_read_buffer));

// open cache_data
FILE* fd_read;
fd_read = fopen("cache.txt","r");

fseek (fd_read, 0, SEEK_END);
int size = ftell(fd_read);
fseek(fd_read,0,SEEK_SET);  /*reset the file pointer to start of file*/
if (size != 0)
{
char line[1000];

while(!feof(fd_read)){
	fgets(line,200,fd_read);
}

char *line_value, *line_site;
line_value = line;
line_site = line;
char* token_time = strstr(line_value,"time:");
token_time = token_time + 5;
char* token_ip = strstr(line_value,"ip:");
strcpy(strchr(token_ip,' '),"\0");
token_ip = token_ip + 3;
char* token_site = strstr(line_site,"site:");
strcpy(strchr(token_site,' '),"\0");
token_site = token_site + 5;
fclose(fd_read);
cache_timeout(atoi(token_time));
}
	time(&timestamp_req);
	timestamp_req_func = timestamp_req;

	char msg_from_proxy[100];
	char *req_url;
	req_url = strtok(NULL, " ");

	char u[1000];
	char* ptr = complete_path;
	char* md = md5sum_calculate(ptr, strlen(complete_path));
	if(req_url == 0)
	{
		sprintf(u,"./cache/%s.html",md);
	}
	else
	{
		ptr = req_url;
		char* md_req = md5sum_calculate(ptr, strlen(req_url));
		sprintf(u,"./cache/%s%s.html",md,md_req);
	}

	// Saves url, ip address of the url and request time in a file 
	FILE *fd_cache = fopen("cache.txt", "a");
	char cache_data[100];
	bzero( cache_data, sizeof(cache_data));
	if(u != NULL && timestamp_req_func != 0)
	{
		sprintf(cache_data,"site:%s ip:%s time:%ld\n",u,addr,timestamp_req);
		int cache_write_bytes = fwrite(cache_data , 1 , strlen(cache_data) , fd_cache );
	}
	fclose(fd_cache);

	int num_bytes;
	FILE* fp = fopen(u,"r");
	if(fp == NULL)
	{
		//fclose(fp);
		printf("\n\n\n--------------- FILE DOES NOT EXIST! %s ----------------\n\n\n", u);
	}
	else
	{
		printf("\n\n\n--------------- FILE EXISTS! %s -----------------\n", u);
		printf("----------------------------- RETRIEVING IP %s FROM CACHE MEMORY ------------------------------\n", addr);
		do
		{
			memset(data_buffer,'\0',sizeof(data_buffer));
			num_bytes = fread(data_buffer, 1, sizeof(data_buffer), fp);
			if(num_bytes< 0)
				perror("FREAD error");
		//	printf("numbytes value is: %d\n\n",num_bytes);
			int nbytes_send = send(accept_var,data_buffer,num_bytes,0);
		//	printf("nbytes send value is: %d\n\n",nbytes_send);
		}while(num_bytes);
		fclose(fp);
		shutdown(accept_var,SHUT_RDWR);
		return;
	}


	if (req_url != 0)
	{
		sprintf(msg_from_proxy,"GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",req_url,version,request_url);
	}
	else
	{
		sprintf(msg_from_proxy,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",version, request_url);
	}
	printf("message--> %s\n", msg_from_proxy);
  printf("\n\nCreating socket\n\n");
	//struct sockaddr_in server_addr_proxy;
	socket_server_proxy = socket(AF_INET,SOCK_STREAM,0);
	if (socket_server_proxy < 0)
	{
		perror("Socket creation error...");
	}
	printf("\n\nsocket created\n\n");
	setsockopt(socket_server_proxy, SOL_SOCKET,SO_REUSEADDR,&tv,sizeof(tv));
	// Defining attributes for so socket
	server_addr_proxy.sin_family = AF_INET;
  server_addr_proxy.sin_port = htons(80);

	//connect
	int server_connect = connect(socket_server_proxy, (struct sockaddr *) &server_addr_proxy, sizeof(server_addr_proxy));
	if (server_connect < 0)
	{
			perror("Failed to connect to host");
			close(socket_server_proxy);
			return;
	}
	printf("sending %s\n", msg_from_proxy);
	//send
 	int bytesSend = send(socket_server_proxy, msg_from_proxy, strlen(msg_from_proxy), 0);
 
	int readBuffer[BufferSize];
	bzero( readBuffer, sizeof(readBuffer));
	//RECEIVE
	int bytesReceived = 0;
	FILE *fd;
	char url_cache[100];
		strcpy(url_cache ,complete_path);

	char* ptr_cache = url_cache;
	char* md_cache = md5sum_calculate(ptr_cache, strlen(url_cache));
	char* md_cache_req;
	if(req_url == 0)
	{
		sprintf(url_cache,"./cache/%s.html",md_cache);
	}
	else
	{
		char* ptr_cache_req = req_url;
		md_cache_req = md5sum_calculate(ptr_cache_req, strlen(req_url));
		sprintf(url_cache,"./cache/%s%s.html",md_cache,md_cache_req);
	}

	printf("url cache --> %s\n", url_cache);
  fd=fopen(url_cache,"ab");
  memset(readBuffer,'\0',sizeof(readBuffer));
	bytesReceived = recv(socket_server_proxy, readBuffer, sizeof(readBuffer), 0 );
	//printf("NUmber of bytes recieved is: %d\n", bytesReceived);
	while(bytesReceived > 0)
	{
 		fwrite(readBuffer , 1 , bytesReceived , fd );
    send(accept_var,readBuffer,bytesReceived,0);
		bzero( readBuffer, sizeof(readBuffer));
		bytesReceived = recv(socket_server_proxy, readBuffer, sizeof(readBuffer), 0 );
 		printf("NUmber of bytes recieved is: %d\n", bytesReceived);
	}
   fclose(fd);
	 close(socket_server_proxy);
}

/*
@brief Receives request from the client. Checks if the connecttion is alive,
			if it isn't and timeout occurs then it will close the connection.
@param socket_connection_id - socket id
@return void
*/
void webserver_handler(int socket_connection_id)
{

		struct timeval tv;
		tv.tv_sec = 10;
		tv.tv_usec = 0;

	  char buffer[256];
		char received_buffer[1024];

		do
		{
			// Sets a timeout of 1sec
				//setsockopt(socket_connection_id, SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

	 			printf("\nWAITING TO RECEIVE A COMMAND\n");
				char buff[1024] = {0};
				int read_var = recv(socket_connection_id, buff, sizeof(buff), 0);
				if (read_var < 0)
				{
					printf("\nConnection timed out\n");
					//	shutdown(accept_var[i],SHUT_RDWR);
					close(socket_connection_id);
					return;
				}

				char new_buffer[1024] = {};
				int j = 0;
				for(int i = 0; i<read_var; i++)
				{
					if(buff[i]!='\0')
					{
						new_buffer[j] = buff[i];
						j++;
					}
				}

				printf("***************************************\n");
				printf("%s\n", new_buffer);
				printf("***************************************\n");

				/* get the first token */
				char* token = strtok(new_buffer, " \n");

				char request_method[100], request_url[100], request_version[100], connection[100];
				int token_count = 0;

	  		/* walk through other tokens */
				while( token != NULL )
				{
		   		token_count++;

		   		if (token_count == 1)
        	{
						strcpy(request_method, token);
					}
					else if (token_count == 2)
					{
				  	strcpy(request_url, token);
					}
			  	else if (token_count == 3)
			  	{
			 			strcpy(request_version, token);
						request_version[8]='\0';
			  	}
			  	else if (strcmp(token, "Connection:") == 0)
			  	{
				 		token = strtok(NULL, " \n");
			   		strcpy(connection, token);
				 		if ((strncmp(connection, "Keepalive",strlen("Keepalive")) == 0))
				 		{
							connection_flag = 1;
				 		}
						else if ((strncmp(connection, "close",strlen("close")) == 0))
						{
							strcpy(connection, "close");
							connection_flag = 0;
						}
						else if(strcmp(connection, "")==0)
						{
							 connection_flag = 1;
						 }
				 		if (strcmp(request_method, "POST") == 0)
				 		{
					 		token = strtok(NULL, " \n");
					 		token = strtok(NULL, " \n");
					 		strcpy(received_buffer, token);

					 		while(token !=  "\0")
					 		{
								token = strtok(NULL, " ");
								if(token == NULL)
								{
									break;
								}
								strcat(received_buffer, " ");
						 		strcat(received_buffer, token);
					 	  }

					 		printf("data---> %s\n", received_buffer);
				 	  }
			 		}
			 		token = strtok(NULL, " \n");
		  	}
				if (strcmp(connection, "") == 0)
				{
					connection_flag = 1;
				}

				if (strcmp(request_method,"GET") == 0)
				{
					get_request(socket_connection_id,request_url,request_version,connection);
				}
	    	else
				{
					printf("entered Else loop......\n");
					int write_bytes = write(socket_connection_id, error_header, strlen(error_header));
					printf("write bytes -- > %d\n", write_bytes);
					break;
				}
		}while(connection_flag);
		shutdown(socket_connection_id,SHUT_RDWR);
		close(socket_connection_id);
		printf("Shutting down socket\n");
		//exit(1);
}

int main(int argc, char * argv[])
{
  char buffer[256];
	char received_buffer[1024];
  struct sockaddr_in server_addr,client_addr;
  pid_t child_thread;
	timeout_cache = atoi(argv[2]);
	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	// clear cache file
	system("rm cache.txt");
	FILE *fr = fopen("cache.txt","w");
	fclose(fr);

	// Create socket server
	socket_server = socket(AF_INET,SOCK_STREAM,0);
	if(!(socket_server))
	{
		perror("ERROR opening socket");
	}
	else printf("\n********WELCOME TO PROXY SERVER********\nSuccessfully created server socket\n");

  setsockopt(socket_server, SOL_SOCKET,SO_REUSEADDR,&tv,sizeof(tv));
	// Defining attributes for so socket
	server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(atoi(argv[1]));

	// Binding address
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

while(1)
{
	printf("entered while loop...\n");
	int addr_length =  sizeof(client_addr);
	//accept
	accept_var[i] = accept(socket_server, (struct sockaddr *) &client_addr, &addr_length);
  if (accept_var[i]<0)
  {
	  perror("ERROR on accept");
		close(accept_var[i]);
  }

	// After receiving a request, create a thread using fork. This is done for
	// handling multiple requests from the client.
  child_thread = fork();

	if (child_thread == 0)
	{
		webserver_handler(accept_var[i]);
		printf("exited handler...\n");
		close(socket_server);
		exit(0);
	}
	else
	{
		printf("\nIn the else loop\n");
		close(accept_var[i]);
	}

	i++;
	i = i%99;
}

	return 0;
}
