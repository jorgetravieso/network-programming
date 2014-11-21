#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream> 
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>

#define DEFAULT_PORT 5555
#define BUF_SIZE 256
#define END_OF_TRANS "\0"
#define END_OF_TRANS_CHAR '\0'

using namespace std;

void syserr(string msg) { perror(msg.c_str()); exit(-1); }
const char * read_current_dir ();
string reaf_file(char *);
void process_request(int);
void replace_char(char * buffer, char oldc, char newc, int n);



int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno;
  struct sockaddr_in serv_addr, clt_addr;
  socklen_t addrlen;

  int pid;

  if(argc < 1 || argc > 2) { 
    fprintf(stderr,"Usage: %s <port>\n", argv[0]);
    return 1;
  } 
  else if(argc == 2)
  {
    portno = atoi(argv[1]);
  }
  else //missing port number
  {
    portno = DEFAULT_PORT;

  }

  printf("portno: %d\n", portno);

  sockfd = socket(AF_INET, SOCK_STREAM, 0); 
  if(sockfd < 0) syserr("can't open socket"); 
  printf("create socket...\n");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
    syserr("can't bind");
  printf("bind socket to port %d...\n", portno);

  listen(sockfd, 5); 


  while(1) {
    printf("wait on port %d...\n", portno);
    addrlen = sizeof(clt_addr); 
    newsockfd = accept(sockfd, (struct sockaddr*)&clt_addr, &addrlen);

    if(newsockfd < 0) 
      syserr("can't accept"); 

    if ( (pid=fork()) == 0) {
      close(sockfd);   //child does not need the listening socket
      process_request(newsockfd);
      exit(0);         //exit child       
    }  
  }


  //close sockets
  close(newsockfd); 
  close(sockfd); 
  return 0;
}

void process_request(int newsockfd)
{  int n;
  char buffer[BUF_SIZE];

  do//while(true)
  {
    bzero(buffer, BUF_SIZE);
    printf("new incoming connection, block on receive\n");
    n = recv(newsockfd, buffer, sizeof(buffer), 0); 
    if(n < 0) 
      syserr("can't receive from client"); 
    else buffer[n] = '\0';

    if(strncmp("ls-remote",buffer,8)==0)
    {

      const char * buf = read_current_dir();
      n = send(newsockfd, buf, strlen(buf) + 1, 0);

      if(n < 0) 
        syserr("can't send to server"); 

    }
    else if(strncmp("put",buffer,3)==0)
    {   
      printf("We start put:\n");
      //get header
      n = recv(newsockfd, buffer, sizeof(buffer), 0);
      if(n < 0) syserr("can't receive from server");
      //replace_char(buffer, END_OF_TRANS_CHAR, '\0', n);
      string header(buffer);
      int separator_index = header.find("|");
      int file_size = 0;
      string name;

      if (separator_index!=string::npos){
        name = header.substr(0, separator_index);
        file_size = atoi(header.substr(separator_index + 1).c_str());
        cout << "size " << file_size << endl;
      }
      if(file_size == 0){
         continue;
      }
      //write the file

      FILE * fp = fopen(name.c_str(), "w");
      if(!fp){
        fprintf(stderr, "%s\n", "There is error writing to file");
        break;
      }

      int counter = 0;
      int total = file_size/BUF_SIZE;

      while(true)
      {
        n = recv(newsockfd, buffer, sizeof(buffer), 0);
        if(n < 0) syserr("can't receive from server");
        for(int i = 0; i < n; i++)
        {
          fputc(buffer[i],fp);
        }
        counter+= n;
        if(counter == file_size) break;
      }
      fclose(fp);
      printf("%s\n", "finished");
      //continue;
    }
    else if (strncmp("get",buffer,3)==0)
    {
        printf("We start get:\n"); 
        n = recv(newsockfd, buffer, sizeof(buffer), 0);  //get filename
        if(n < 0) syserr("can't receive from server");
        buffer[n] = '\0'; 
        string file_as_string = reaf_file(buffer);      //read file

        string file_size = to_string( file_as_string.length()); 
        file_size.append(END_OF_TRANS);
        n = send(newsockfd, file_size.c_str(), file_size.length(), 0);  //send file size
        if(n < 0) syserr("can't send to server");
        //fflush(stdout);
        n = send(newsockfd, file_as_string.c_str(), file_as_string.length(), 0);  //send file 
        if(n < 0) syserr("can't send to server");
    }


  }while(strncmp("exit",buffer,4)!=0);
  cout << "we finished and child is going to die" << endl;

  close(newsockfd);
}


const char * read_current_dir ()
{

  string buf;  
      //char * buf = NULL;
  DIR *dir;
  struct dirent *ent;

  if((dir = opendir(".")) == NULL) {
    printf("Could not open the directory\n");
  }
  else

    while((ent = readdir (dir)) != NULL){
      if(strcmp(ent->d_name, "..") == 0) continue;
      if(strcmp(ent->d_name, ".") == 0) continue;
      buf.append(ent->d_name);
      buf.append("\n");
    }

    buf.append(END_OF_TRANS);

    closedir(dir);



    return buf.c_str();
  }

   string reaf_file(char * file_name)
  {
      ifstream t(file_name);
      stringstream buffer;
      buffer << t.rdbuf();
      
      string str =  buffer.str();
      t.close();
      //cout << str << endl;
      return str;


}
//replaces first instance of a char oldc with a new char newc 
void replace_char(char * buffer, char oldc, char newc, int n)
{
    for(int i = 0; i < n; i++)
    {
        if(buffer[i]==oldc)
        {
            buffer[i]=newc;
            break;
        }
    }
}

