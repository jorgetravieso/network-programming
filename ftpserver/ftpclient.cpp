/*

Jorge Travieso
PI 4857521
Professor Liu



*/



#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <sstream> 
#include <fstream>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BUF_SIZE 256
#define END_OF_TRANS "\0"
#define END_OF_TRANS_CHAR '\0'

using namespace std;

const char * read_current_dir ();
const string reaf_file(string);
void syserr(string msg) { perror(msg.c_str()); exit(-1); }
void replace_char(char * buffer, char oldc, char newc, int n);



int main(int argc, char ** argv)
{

    int sockfd, portno, n;
    struct hostent* server;
    struct sockaddr_in serv_addr;
    char buffer[BUF_SIZE];

    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
        return 1;
    }

    portno = atoi(argv[2]);
    server = gethostbyname(argv[1]);
    if(!server) {
        fprintf(stderr, "ERROR: no such host: %s\n", argv[1]);
        return 2;
    }
    
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if(sockfd < 0) 
        syserr("can't open socket");
    
    printf("create socket...\n");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
    serv_addr.sin_port = htons(portno);

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        syserr("can't connect to server");
    
    printf("Connecting...\n");

    while(true){

        memset(buffer, 0 , sizeof(buffer));
        printf("%s:%d> ", argv[1], portno);
        fgets(buffer, 255, stdin);
        n = strlen(buffer); 
        if(n>0 && buffer[n-1] == '\n') buffer[n-1] = '\0';

        //process the the user commands

        if(strncmp("exit",buffer,4)==0)
        {   
                    //SENDING
            n = send(sockfd, buffer, strlen(buffer), 0);    
            if(n < 0) syserr("can't send to server");
            printf("Connection to server %s:%d terminated. Bye now!\n", argv[1], portno);
            break;
        }
        else if(strncmp("ls-local",buffer,8)==0)
        {   
            cout << "Files at the client:" << endl;
            cout << read_current_dir();
           
        }
        else if(strncmp("ls-remote",buffer,9)==0)
        {   
            printf("Files at the server (%s):\n", argv[1]);    
            bool finished = false;
            n = send(sockfd, buffer, strlen(buffer), 0);    
            if(n < 0) syserr("can't send to server");
            //int counter = 0;
            while(!finished)
            {
                n = recv(sockfd, buffer, sizeof(buffer), 0);
                if(n < 0) syserr("can't receive from server");
               
                
                for( int i = 0; i < n; i++)
                {
                    
                    if(buffer[i] == END_OF_TRANS_CHAR) {
                        finished = true;
                        break;
                    }
                    printf("%c", buffer[i]);
                }


            }

        }
        else if(strncmp("put",buffer,3)==0)
        {  

            n = send(sockfd, "put", 3, 0);    
            if(n < 0) syserr("can't send to server");
            string str(buffer);
            string file_name = str.substr(4);
            cout << "Upload "<< file_name << " to remote server: ";
            string file_as_string = reaf_file(file_name);
            string header = file_name + "|" + to_string(file_as_string.length());
            header.append(END_OF_TRANS);
            //cout << "file header: " << header << endl;
            n = send(sockfd, header.c_str(), header.length(), 0);    
            if(n < 0) syserr("can't send to server");

            fflush(stdout);
            //cout << file_as_string << endl;
            if(file_as_string.length() == 0){
                cout << " failed." << endl;
                continue;

            }

            n = send(sockfd, file_as_string.c_str(), file_as_string.length(), 0);    
            if(n < 0) syserr("can't send to server");
            cout << " successful." << endl;



        }

        else if(strncmp("get",buffer,3)==0)
        {  
            n = send(sockfd, "get", 3, 0);              //send command
            if(n < 0) syserr("can't send to server");
            //fflush(stdout);
            string str(buffer);
            string file_name = str.substr(4);           //send filename
            cout << "Retrieve "<< file_name << " from remote server: ";
            n = send(sockfd, file_name.c_str(), file_name.length(), 0);    
            if(n < 0) syserr("can't send to server");

            cout << "attempt" << endl;
            //fflush(stdout);
                                                            //receive size
            n = recv(sockfd, buffer, sizeof(buffer), 0);
            if(n < 0) syserr("can't receive from server");
            //buffer[n] = '\0';
            replace_char(buffer, END_OF_TRANS_CHAR, '\0', n);

            int size = atoi(buffer);
            cout << "file_size: " << size << endl;
            int total = size / BUF_SIZE;
            int counter = 0;


            FILE * fp = fopen(file_name.c_str(), "w");
            if(!fp){
                fprintf(stderr, "%s\n", "There is error writing to file");
                cout << " failed" << endl;
                continue;
                //break;
            }

            while(true)
            {
                memset(buffer, 0, sizeof(buffer));
                n = recv(sockfd, buffer, sizeof(buffer), 0);
                if(n < 0) syserr("can't receive from server");
                for(int i = 0; i < n; i++)
                {
                    fputc(buffer[i],fp);
                }
                //bzero(buffer, sizeof(buffer));
                counter+=n;
                if(counter == size) break;
            }
            fclose(fp);
            printf("%s\n", "finished");
            //continue;
        }
        else{
            cout << "Invalid command, try again"  << endl;
        }
    }

    close(sockfd);
    return 0;


}

const char * read_current_dir ()
{

  string buf;  
      //char * buf = NULL;
  DIR *dir;
  struct dirent *ent;

  if((dir = opendir(".")) == NULL) 
    printf("Could not open the directory\n");
else
    while((ent = readdir (dir)) != NULL)
    {
      if(strcmp(ent->d_name, "..") == 0) continue;
      if(strcmp(ent->d_name, ".") == 0) continue;
      buf.append(ent->d_name);
      buf.append("\n");
    }



  closedir (dir);



  return buf.c_str();
}

const string reaf_file(string file_name)
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



