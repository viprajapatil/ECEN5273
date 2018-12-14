/*
@file dfc.c
@author Vipraja Patil
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

int dfs_server_port[4];
int sockfd[100];
struct sockaddr_in server_addr[100];
char username[20];
char password[20];
int i = 0;

void get_conf_info()
{
  FILE *fr = fopen("dfc.conf","r");
  char line[1000];
  if (fr == NULL)
  {
      perror("Unable to open conf file:");
      exit(-1);
  }
  while(fgets(line, 200, fr) !=  NULL)
  {
      line[strlen(line)-1] = '\0';
      if (strstr(line,"Server") != NULL)
      {
          char *s = strstr(line, ":");
          char *st = strtok(s,":");
          dfs_server_port[i] = atoi(st);
        //  printf("DFS is running at port-> %d\n", dfs_server_port[i]);
          i++;
      }
      if (strstr(line, "Username") != NULL)
      {
          char *s = strstr(line, " ");
          char *st = strtok(s," ");
          strcpy(username,st);
      }
      if (strstr(line, "Password") != NULL)
      {
          char *s = strstr(line, " ");
          char *st = strtok(s," ");
          strcpy(password,st);
      }
  }
  fclose(fr);
}

void connect_to_servers()
{
    for (int i=0; i<4; i++)
    {
        //set timeout for 1 sec for connection;
        struct timeval timeout;
	      timeout.tv_sec = 1;
	      timeout.tv_usec = 0;
      //  printf("port number->  %d\n", dfs_server_port[i]);
        bzero(&server_addr[i],sizeof(server_addr[i]));               //zero the struct
        server_addr[i].sin_family = AF_INET;                 //address family
        server_addr[i].sin_port = htons(dfs_server_port[i]);      //sets port to network byte order
        server_addr[i].sin_addr.s_addr = inet_addr("127.0.0.1"); //sets remote IP address

        sockfd[i] = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd[i] < 0)
        {
            perror("Unable to open socket");
        }
        int server_connect = connect(sockfd[i], (struct sockaddr *) &server_addr[i], sizeof(server_addr[i]));
	      if (server_connect < 0)
	      {
			       perror("Failed to connect to host");
			       close(sockfd[i]);
			       return;
	      }

        char authen[100];
        sprintf(authen,"%s %s",username,password);
        int nbytes = send(sockfd[i], authen, strlen(authen), 0);
        if (nbytes < 0){
          printf("nbytes-> %d\n", nbytes);
        }
        else printf("nbytes-> %d\n", nbytes);


        printf("Sucessful connection for port %d \n", dfs_server_port[i]);
	      //setsockopt (sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout);
      }
    }

/*
Put a file into 4 dfs servers
*/
void put_file()
{
    //
}

void get_file()
{
    //
}

int main(int argc, char **argv)
{
    char command[100];
    if (argc < 2)
    {
        printf("Less args <dfc config file>");
        exit(-1);
    }
    printf("********** Welcome to DFC (client)! ***********\n");
    get_conf_info();

    //connect to all the 4 servers
    connect_to_servers();


    //take command type as input
    printf("Enter command: ");
    scanf("%[^\t\n]s", command);
    for (int i=0; i<4; i++)
    {
      int nbytes = send(sockfd[i], command, strlen(command), 0);
      if (nbytes < 0){
        printf("nbytes-> %d\n", nbytes);
      }
    }

    if (strstr(command,"get") != NULL)
    {
      printf("s1-> %s",command);

        //call get function
        printf("get called!/n\n");
    }
    else if (strstr(command,"put") != NULL)
    {
        //call put function
        printf("put called!/n\n");
    }
    else if (strstr(command, "list") != NULL)
    {
        //call list function
        printf("list called!/n\n");
    }
    else printf("Entered wrong command, enter -> list, get or put\n");

    return 0;
}
