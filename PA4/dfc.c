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
char subfolder[20];
char conf[50];
int index_value[8];
int i = 0;
int status[4];

void get_conf_info()
{
  FILE *fr = fopen(conf,"r");
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
             status[i] = server_connect;
			       close(sockfd[i]);
			       return;
	      }

        char authen[100];
        sprintf(authen,"%s %s %s",username,password,subfolder);
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
Calculate md5sum of the file
*/
int md5sum(char *filename)
{
    char md5value[100];
    char md5_command[100];
    sprintf(md5_command,"md5sum %s",filename);
    FILE *f = popen(md5_command, "r");
    while (fgets(md5value, 100, f) != NULL) {
        strtok(md5value,"  \t\n");
    }
    pclose(f);
    return atoi(md5value);
}

/*
According to the index value send files to fifferent servers
*/
void hashtable(int index)
{
    if (index == 0)
    {
        int filename_index[8] = {1,2,2,3,3,4,4,1};
        for(int i=0; i<8; i++)
          index_value[i] = filename_index[i];
    }
    else if (index == 1)
    {
        int filename_index[8] = {4,1,1,2,2,3,3,4};
        for(int i=0; i<8; i++)
          index_value[i] = filename_index[i];
    }
    else if (index == 2)
    {
        int filename_index[8] = {3,4,4,1,1,2,2,3};
        for(int i=0; i<8; i++)
          index_value[i] = filename_index[i];
    }
    else
    {
        int filename_index[8] = {2,3,3,4,4,1,1,2};
        for(int i=0; i<8; i++)
          index_value[i] = filename_index[i];
    }
}

/*
Put a file into 4 dfs servers
*/
void put_file(char *filename)
{
  char d;
  for(int i=0;i<strlen(password)-1;i++)
      d = password[i]^password[i+1];
  FILE *f = fopen(filename, "rb");
  printf("filename-> %s\n", filename);
  if(f==NULL)
      perror("error on file open:\n");
    int modvalue = md5sum(filename);
    modvalue = modvalue%4;
    hashtable(modvalue);
    printf("mod-> %d\n", modvalue);
    char cmd[100];
    fseek(f, 0, SEEK_END); // seek to end of file
    int size = ftell(f); // get current file pointer
    fseek(f, 0, SEEK_SET); // seek back to beginning of file

    // divide in 4 parts
    int send_size = size/4;
    int last_size = size - (send_size*3);
    printf("peice ->%d, last ->%d\n", send_size, last_size);

    // Divide files in 4 parts and store in buffer
    fseek(f,0,SEEK_SET); /*set file pointer to start of file*/
    char buffer0[send_size];
    memset(buffer0,'\0',send_size);
    fread(buffer0,send_size,1,f);
    for(int i=0; i<strlen(buffer0);i++)
        buffer0[i] = buffer0[i]^1;

    fseek(f,send_size,SEEK_SET); /*set file pointer to start of file*/
    char buffer1[send_size];
    memset(buffer1,'\0',send_size);
    fread(buffer1,send_size,1,f);
    for(int i=0; i<strlen(buffer1);i++)
        buffer1[i] = buffer1[i]^1;

    fseek(f,send_size*2,SEEK_SET); /*set file pointer to start of file*/
    char buffer2[send_size];
    memset(buffer2,'\0',send_size);
    fread(buffer2,send_size,1,f);
    for(int i=0; i<strlen(buffer2);i++)
        buffer2[i] = buffer2[i]^1;

    fseek(f,send_size*3,SEEK_SET); /*set file pointer to start of file*/
    char buffer3[last_size];
    memset(buffer3,'\0',last_size);
    fread(buffer3,last_size,1,f);
    for(int i=0; i<strlen(buffer3);i++)
        buffer3[i] = buffer3[i]^1;
    int c =0;
    // send files to dfs servers
  /*  for (int i=0; i<4; i++)
    {

    }*/
    int nbytes;
    char filename_ext[20];
    char buffer_num[100];
    int index = 0;
    int count = 1;
    for(int i=0; i<4; i++)
    {
        if(status[i] < 0)
        {
          index = index+2;
          continue;
        }

        while(count<3)
        {
            bzero(filename_ext,sizeof(filename_ext));
            sprintf(filename_ext,".%s.%d",filename,index_value[index]);
            filename_ext[strlen(filename_ext)] = '\0';
            char sample[5];
            bzero(sample,sizeof(sample));
            nbytes= recv(sockfd[i],sample,sizeof(sample),0);
            nbytes = send(sockfd[i], filename_ext, strlen(filename_ext), 0);
            //sleep(2);
            recv(sockfd[i],sample,sizeof(sample),0);
            if(index_value[index] == 1)
            {
                nbytes = send(sockfd[i], buffer0, send_size, 0);
            }
            else if (index_value[index] == 2)
            {
              nbytes = send(sockfd[i], buffer1, send_size, 0);
            }
            else if (index_value[index] == 3)
            {
                nbytes = send(sockfd[i], buffer2, send_size, 0);
            }
            else if (index_value[index] == 4)
            {
                nbytes = send(sockfd[i], buffer3, last_size, 0);
            }
            printf("\nnbytes->%d\n",nbytes);
            printf("server->%d, file->%d\n\n", i,index_value[index]);
            index++;
            count++;


        }
        count = 1;
        //printf("server->%d, file->%d\n", i,index_value[index]);
    }

}


void get_file()
{
  char d;
  for(int i=0;i<strlen(password)-1;i++)
      d = password[i]^password[i+1];
  int nbytes, h=0;
    long long int n;
    char ser[10] = "random";
    char filename_get[100];
    char file_name[100];
    char p[4][100];
    char server_data[1000000];
    int c = 0;
    for(int i=0; i<4; i++)
    {
      if(status[i] < 0)
          continue;
        for(int j=0; j<2; j++)
        {
          bzero(filename_get,sizeof(filename_get));
          bzero(server_data,sizeof(server_data));
          sleep(1);
          nbytes = send(sockfd[i], "hello", 5, 0); //for sync
          nbytes = recv(sockfd[i], filename_get, sizeof(filename_get), 0);
          printf("\nfileext->%s ,n->%d\n", filename_get,nbytes);
          if (strcmp(filename_get,"ERROR") == 0)
              continue;
          sprintf(file_name,"%s",filename_get);
          //printf("filename->%s\n\n", file_name);
          FILE *f = fopen(filename_get,"wb");

              send(sockfd[i], "hello", 5, 0); //for sync
              nbytes = 1;
              while(nbytes > 0)
              {
              nbytes = recv(sockfd[i], server_data, sizeof(server_data), 0);
              printf("nbytes->%d\n", nbytes);
              for(int i=0; i<strlen(server_data);i++)
                  server_data[i] = server_data[i]^1;
              if (nbytes > 0)
              {
              fwrite(server_data,1,nbytes,f);
              }
              if(nbytes < sizeof(server_data))
              {
                break;
              }
            }
          fclose(f);
          c++;
        }

    }

    char file_with_ext[100];
    file_name[strlen(file_name)-2] = '\0';
    for (int i=1; i<=4; i++)
    {
      bzero(file_with_ext,sizeof(file_with_ext));
      //printf("filename->%s\n",file_name);
      sprintf(file_with_ext,"%s.%d",file_name,i);
      //printf("file->%s\n", file_with_ext);
      strcpy(p[i-1],file_with_ext);
    }
    char cmd_merge[200];
    sprintf(cmd_merge,"cat %s %s %s %s > %s",p[0],p[1],p[2],p[3],file_name+1);
    printf("merge->%s\n", cmd_merge);
    system(cmd_merge);

}


void list_func()
{
      char server_data[1000];
      int c =0;
      char line[100];
      char file_name_list[100];
      char file[100];
      bzero(file,sizeof(file));
      bzero(server_data,sizeof(server_data));
      for(int i=0;i<4;i++)
      {
        if(status[i] < 0)
            continue;

          bzero(file_name_list,sizeof(file_name_list));
          sprintf(file_name_list,"list%d.txt",i);
          int nbytes = recv(sockfd[i], server_data, sizeof(server_data), 0);
          FILE *f = fopen(file_name_list,"w");
          fwrite(server_data,1,nbytes,f);
          fclose(f);
          system("sort -u list* > list.txt");
      }
      FILE *f = fopen("list.txt","r");
      if(f==NULL)
          perror("fopen error");
      while(fgets(line,sizeof(line),f) != NULL)
      {
          if(strstr(line,".1") != NULL)
          {
            printf("%s\n", line);
            char *fl = line;
            *(fl+strlen(fl)-2)='\0';

            fgets(line,sizeof(line),f);
            if (strstr(line,".2") == NULL){
                printf("%s2 missing\n",fl);
              }
            else {printf("%s\n", line);
            fgets(line,sizeof(line),f);
          }

            if (strstr(line,".3") == NULL){
                printf("%s3 missing\n",fl);}
            else {printf("%s\n", line);
            fgets(line,sizeof(line),f);
            }

            if (strstr(line,".4") == NULL){
                printf("%s4 missing\n",fl);}
            else printf("%s\n", line);
          }
          else if(strstr(line,".2") != NULL)
          {
            char *fl = line;
            *(fl+strlen(fl)-2)='\0';
            printf("%s1 missing\n", fl);
            printf("%s\n", line);

            fgets(line,sizeof(line),f);
            if (strstr(line,".3") == NULL)
                printf("%s3 missing\n",fl);
            else{ printf("%s\n", line);
            fgets(line,sizeof(line),f);}

            if (strstr(line,".4") == NULL)
                printf("%s4 missing\n",fl);
            else printf("%s\n", line);
          }
          else if(strstr(line,".3") != NULL)
          {
            char *fl = line;
            *(fl+strlen(fl)-2)='\0';
            printf("%s1 missing\n", fl);
            printf("%s2 missing\n", fl);
            printf("%s\n", line);
            fgets(line,sizeof(line),f);
            if (strstr(line,".4") == NULL)
                printf("%s4 missing\n",fl);
            else printf("%s\n", line);
          }
          else if(strstr(line,".4") != NULL)
          {
            char *fl = line;
            *(fl+strlen(fl)-2)='\0';
            printf("%s1 missing\n", fl);
            printf("%s2 missing\n", fl);
            printf("%s3 missing\n", fl);
            printf("%s\n", line);
          }
      }
}


int main(int argc, char **argv)
{
    char command[100];
    char *filename, *sub_folder;
    if (argc < 3)
    {
        printf("Less args <dfc config file> <subfolder>");
        exit(-1);
    }
    printf("********** Welcome to DFC (client)! ***********\n");
    sprintf(conf,"%s",argv[1]);
    sprintf(subfolder,"%s",argv[2]);
    get_conf_info();

    //connect to all the 4 servers
    //connect_to_servers();
    while(1)
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
             status[i] = server_connect;
			       close(sockfd[i]);
			       //return;
	      }

        char authen[100];
        sprintf(authen,"%s %s %s",username,password,subfolder);
        int nbytes = send(sockfd[i], authen, strlen(authen), 0);
        if (nbytes < 0){
          printf("nbytes-> %d\n", nbytes);
        }
        else printf("nbytes-> %d\n", nbytes);


        printf("Sucessful connection for port %d \n", dfs_server_port[i]);
	      //setsockopt (sockfd[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout);
      }

    //take command type as input
    memset(command,0,sizeof(command));
    printf("Enter command: ");
    scanf("%[^\t\n]s", command);
    getchar();
    for (int i=0; i<4; i++)
    { if (status[i] < 0)
          continue;
      int nbytes = send(sockfd[i], command, strlen(command), 0);
      if (nbytes < 0){
        printf("nbytes-> %d\n", nbytes);
      }
    }

    if (strstr(command,"get") != NULL)
    {
      filename = strtok(command," ");
      filename = strtok(NULL," ");
      sub_folder = strtok(NULL, " ");
      //filename = strstr(command, " ");
      //filename++;
      get_file();
    }
    else if (strstr(command,"put") != NULL)
    {
      filename = strtok(command," ");
      filename = strtok(NULL," ");
      sub_folder = strtok(NULL, " ");
        //filename = strstr(command, " ");
        //filename++;
        put_file(filename);
    }
    else if (strstr(command, "list") != NULL)
    {
        //call list function
        list_func();
    }
    else printf("Entered wrong command, enter -> list, get or put\n");

}
    return 0;
}
