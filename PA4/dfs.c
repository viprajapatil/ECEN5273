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
char encpt_password[20];



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
  sprintf(encpt_password,"%s",password);
  char *sub;
  sub = strtok(subb," ");
  sub = strtok(NULL, " ");
  sub = strtok(NULL, " ");
  char cmdsub[100];
  bzero(cmdsub,sizeof(cmdsub));
  sprintf(cmdsub,"mkdir -p %s/%s/%s",directory,username,sub);
  sprintf(directory,"%s/%s",directory,username);
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


void put_file(int socket_connection_id, char *sub)
{

    char file_ext[20], file_path[100];
    char directory_put[100];
    bzero(directory_put, sizeof(directory_put));
    sprintf(directory_put,"%s/%s",directory,sub);
    char buffer[1000000];
    int n;
    for(int i=0; i<2; i++)
    {
    //receive 2 files from the client and store it in subfolder
    memset(file_ext,'\0', sizeof(file_ext));
    memset(file_path,'\0' ,sizeof(file_path));
    // Receive command from client
    send(socket_connection_id,"hello",5,0);
    n = recv(socket_connection_id, file_ext, sizeof(file_ext), 0);
    printf("file ext rec-> %s, bytes->%d\n", file_ext,n);
    bzero(buffer, sizeof(buffer));
    //sleep(1);
    // Receive command from client
    send(socket_connection_id,"jello",5,0);//for sync
    n = 1;
    while(n > 0){
      n = recv(socket_connection_id, buffer, sizeof(buffer), 0);
      printf("rec n->%d\n", n);
      if(n == 0)
      {
        break;
      }
      //printf("\ncontent-> %s   n-> %lld\n", buffer, n);
      sleep(1);
      sprintf(file_path,"./%s/%s",directory_put,file_ext);
      if(n > 0)
      {
        FILE *f1 = fopen(file_path,"wb");
        fwrite(buffer,1,n,f1);
        fclose(f1);
      }
      bzero(file_ext,sizeof(file_ext));
    if(n < sizeof(buffer))
      break;
    }
}
shutdown(socket_connection_id,SHUT_RDWR);
close(socket_connection_id);
}



void get_file(char *filename, int socket_connection_id, char *sub)
{
    int c = 0;
    char sample[10];
    char cmd[100];
    char line[100];
    char directory_get[100];
    bzero(directory_get,sizeof(directory_get));
    sprintf(directory_get,"%s/%s",directory,sub);
    //send all files to client
    char file[100];
    sprintf(cmd,"cd %s;ls -a",directory_get);
    FILE *f = popen(cmd, "r");

    while (fgets(line, 100, f) != NULL) {
        bzero(file,sizeof(file));
        sprintf(file,".%s",filename);
        char *fs = strstr(line,file);
        if (fs != NULL)
        {
          char *fw = strstr(line,file);
          fw = strtok(fw,"\n");
          if (fw != NULL)
          {   printf("fw->%s\n", fw);
              bzero(file,sizeof(file));
              sprintf(file,"./%s/%s",directory_get,fw);
              FILE *fs = fopen(file,"rb");
              if (fs == NULL)
              {
                perror("fopen error:");
              }
              char filename_get[100];
              bzero(filename_get,sizeof(filename_get));
              sprintf(filename_get,"%s",fw);
              filename_get[strlen(filename_get)] = '\0';
              recv(socket_connection_id, sample, 5, 0); //for sync
              //printf("recv sample\n");
              int n = send(socket_connection_id, filename_get, strlen(filename_get), 0);
              printf("\nfile_ext bytes->%d\n", n);
              sleep(1);
              fseek(fs, 0, SEEK_END); // seek to end of file
              int size = ftell(fs); // get current file pointer
              fseek(fs, 0, SEEK_SET); // seek back to beginning of file
              char buffer[size];
              memset(buffer,'\0',sizeof(buffer));
              fread(buffer,size,1,fs);
              fclose(fs);
              //printf("\n%s\n", buffer);

              recv(socket_connection_id, sample, 5, 0);  //for sync
              printf("recv sample content\n");
              n = send(socket_connection_id, buffer, size, 0);
              printf("n send->%d\n", n);
              sleep(1);
              c++;
              if(c > 2)
              {
                break;
              }
          }

        }

    }
    if (c==1)
    {
      recv(socket_connection_id, sample, 5, 0); //for sync
      printf("recv sample\n");
      char error_file[100] = "ERROR";
      int n = send(socket_connection_id, error_file, strlen(error_file), 0);
      printf("only 1 file");
      // recv(socket_connection_id, sample, 5, 0);  //for sync
      // printf("recv sample content\n");
      // n = send(socket_connection_id, buffer, size, 0);
    }
    pclose(f);
    shutdown(socket_connection_id,SHUT_RDWR);
    close(socket_connection_id);

}


void list_func(int socket_connection_id,char *sub)
{
    char directory_list[100];
    char line[100];
    bzero(directory_list,sizeof(directory_list));
    sprintf(directory_list,"%s/%s",directory,sub);
    char cmd[100];
    bzero(cmd,sizeof(cmd));
    sprintf(cmd,"cd %s;ls -a > list_file.txt",directory_list);
    system(cmd);
    char some[100];bzero(some,sizeof(some));
    sprintf(some,"./%s/list_file.txt",directory_list);
    printf("d->%s\n", some);
    FILE *fs = fopen(some,"rb");
    if (fs==NULL)
        perror("fopen error");

        fseek(fs, 0, SEEK_END); // seek to end of file
        int size = ftell(fs); // get current file pointer
        fseek(fs, 0, SEEK_SET); // seek back to beginning of file
        char buffer[size];
        memset(buffer,'\0',sizeof(buffer));
        fread(buffer,size,1,fs);
        fclose(fs);
    int n = send(socket_connection_id, buffer, size, 0);
    shutdown(socket_connection_id,SHUT_RDWR);
    close(socket_connection_id);
}



void handler(int socket_connection_id)
{
  char buffer[100];
  char *filename;
  int n = recv(socket_connection_id, buffer, sizeof(buffer), 0);
  buffer[n] = '\0';
  printf("%s\n", buffer);
  // Authenticate credentials
  int a = authenticate_credentials(buffer);
  if (a == 1)
  {
      printf("Authentication failed\n");
      shutdown(socket_connection_id,SHUT_RDWR);
      close(socket_connection_id);
  }

  bzero(buffer, sizeof(buffer));
  // Receive command from client
  n = recv(socket_connection_id, buffer, sizeof(buffer), 0);
  printf("command-> %s\n", buffer);


/*  n = recv(accept_var[i], buffer, sizeof(buffer), 0);
  printf("command-> %s", buffer);*/

  if (strstr(buffer,"get") != NULL)
  {
    filename = strtok(buffer, " ");
    filename = strtok(NULL, " ");
    char *sub = strtok(NULL, " ");
    printf("entered get loop..\n");
    get_file(filename, socket_connection_id, sub);
  }
  else if (strstr(buffer,"put") != NULL)
  {
      // filename = strstr(buffer, " ");
      // filename++;
      filename = strtok(buffer, " ");
      filename = strtok(NULL, " ");
      char *sub = strtok(NULL, " ");
      printf("file->%s, sub->%s\n", filename,sub);
      put_file(socket_connection_id, sub);
  }
  else if (strstr(buffer, "list") != NULL)
  {
      //call list function
      char *sub = strtok(buffer, " ");
      sub = strtok(NULL, " ");
      list_func(socket_connection_id,sub);
  }
  else printf("Entered wrong command\n");
  shutdown(socket_connection_id,SHUT_RDWR);
  close(socket_connection_id);
}


int main(int argc, char **argv)
{
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int socket_server;
    pid_t child;

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
    if (!socket_server)
    {
        perror("Error creating the socket");
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


    while(1){
      int addr_length = sizeof(client_addr);
      printf("****************************\n");
    accept_var[i] = accept(socket_server, (struct sockaddr *) &client_addr, &addr_length);
    if (accept_var[i]<0)
    {
	      perror("ERROR on accept");
      //  shutdown(accept_var[i],SHUT_RDWR);
		    close(accept_var[i]);
    }
    else printf("Accept complete\n");

    child = fork();
    if (child == 0)
    {
      handler(accept_var[i]);
      close(socket_server);
      exit(0);
    }
    else {
      close(accept_var[i]);
    }
    i++;
	   i = i%99;
}
    return 0;
}
