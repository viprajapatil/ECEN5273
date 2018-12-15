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

enum DFS_server{
  DFS1 = 1,
  DFS2 = 2,
  DFS3 = 3,
  DFS4 = 4,
};

int server_num;
int socket_server,accept_var[100], socket_server_proxy;
struct sockaddr_in server_addr, client_addr;
int i = 0;
char directory[100];

int authenticate_credentials(char buffer[])
{
  char subb[100];
  sprintf(subb,"%s",buffer);
  char *username_c;
  char *password_c;
  char *username = strtok(buffer, " ");
  char *password = strstr(buffer, " ");
  password = strtok(NULL, "");
  password = strtok(password, " ");

  char *sub;
  sub = strtok(subb," ");
  sub = strtok(NULL, " ");
  sub = strtok(NULL, " ");
  char cmdsub[100];
  bzero(cmdsub,sizeof(cmdsub));
  sprintf(cmdsub,"mkdir -p %s/%s/%s",directory,username,sub);
  sprintf(directory,"%s/%s/%s",directory,username,sub);
  FILE *fr = fopen("dfs.conf","r");
  if (fr == NULL)
  {
      perror("Unable to open conf file:");
      exit(-1);
  }
  char line[1000];
  while(fgets(line, 200, fr) !=  NULL)
  {
      line[strlen(line)-1] = '\0';
      username_c = strtok(line, " ");
      password_c = strstr(line, " ");
      password_c = strtok(NULL, "");
      password_c = strtok(password_c, " ");
    //  printf("%s %s %s\n", username_c, password_c, subfolder);
      if (strcmp(username,username_c) == 0 && strcmp(password, password_c) == 0)
      {
        printf("Authentication completed!\n");
        system(cmdsub);
        return 0;
      }
  }
  exit(-1);
  return 1;
}
void put_file()
{
    char file_ext[100], file_path[100];
    char buffer[1000];
    int n;
    for(int i=0; i<2; i++)
    {
    //receive 2 files from the client and store it in subfolder
    bzero(file_ext, sizeof(file_ext));
    bzero(file_path, sizeof(file_path));
    // Receive command from client
    n = recv(accept_var[0], file_ext, sizeof(file_ext), 0);
    printf("file ext rec-> %s\n", file_ext);
    bzero(buffer, sizeof(buffer));
    sleep(1);
    // Receive command from client
    n = recv(accept_var[0], buffer, sizeof(buffer), 0);
    printf("\ncontent-> %s   n-> %d\n", buffer, n);

    sprintf(file_path,"./%s/%s",directory,file_ext);
    printf("file ext-> %s\n", file_path);
    FILE *f1 = fopen(file_path,"w");
    fwrite(buffer,1,n,f1);
    fclose(f1);
    }
}




void get_file()
{
    char sample[100];
    //send all files to client
    int n = recv(accept_var[0], sample, sizeof(sample), 0);
    n = send(accept_var[0], sample, sizeof(sample), 0);

}





int main(int argc, char **argv)
{
    struct timeval tv;
    int socket_server;
    char *filename;
    int port = atoi(argv[2]);
    printf("port %d\n", port);
    if (argc < 3)
    {
        printf("Less args <directory> <portno>\n");
        exit(-1);
    }
    sprintf(directory,"%s",argv[1]);
    char cmd[100];
    sprintf(cmd,"mkdir -p %s", argv[1]);
    system(cmd);
    socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_server < 0)
    {
        perror("Error creating the socket");
        exit(-1);
    }
    setsockopt(socket_server, SOL_SOCKET,SO_REUSEADDR,&tv,sizeof(tv));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(socket_server, (struct sockaddr *) &server_addr,sizeof(server_addr)) < 0)
    {
        perror("Error on binding");
    }
    else printf("Bind Sucessful\n");

    if(listen(socket_server ,5) < 0)
	  {
		    perror("ERROR listening");
	  }
	  else printf("Listen success\n");
    int addr_length = sizeof(client_addr);
    accept_var[i] = accept(socket_server, (struct sockaddr *) &client_addr, &addr_length);
    if (accept_var[i]<0)
    {
	       perror("ERROR on accept");
		     close(accept_var[i]);
    }
    else printf("Accept complete\n");
    char buffer[100];
    int n = recv(accept_var[i], buffer, sizeof(buffer), 0);
    printf("%s\n", buffer);
    // Authenticate credentials
    int a = authenticate_credentials(buffer);
    if (a == 1)
    {
        printf("Authentication failed\n");
        close(accept_var[i]);
    }

    bzero(buffer, sizeof(buffer));
    // Receive command from client
    n = recv(accept_var[i], buffer, sizeof(buffer), 0);
    printf("command-> %s\n", buffer);


  /*  n = recv(accept_var[i], buffer, sizeof(buffer), 0);
    printf("command-> %s", buffer);*/

    if (strstr(buffer,"get") != NULL)
    {
      filename = strstr(buffer, " ");
      filename++;
      get_file();
    }
    else if (strstr(buffer,"put") != NULL)
    {
        filename = strstr(buffer, " ");
        filename++;
        put_file();
    }
    else if (strstr(buffer, "list") != NULL)
    {
        //call list function
        printf("list called!/n\n");
    }
    else printf("Entered wrong command\n");

    return 0;
}
