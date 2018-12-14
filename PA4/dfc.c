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
          printf("DFS is running at port-> %d\n", dfs_server_port[i]);
          i++;
      }
      if (strstr(line, "Username") != NULL)
      {
          char *s = strstr(line, " ");
          char *st = strtok(s," ");
          printf("Username-> %s\n", st);
      }
      if (strstr(line, "Password") != NULL)
      {
          char *s = strstr(line, " ");
          char *st = strtok(s," ");
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
        printf("port number->  %d\n", dfs_server_port[i]);
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
        printf("Sucessful connection! %d\n", dfs_server_port[i]);
	      //setsockopt (sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout);
      }
    }



int main(int argc, char **argv)
{
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
    return 0;
}
