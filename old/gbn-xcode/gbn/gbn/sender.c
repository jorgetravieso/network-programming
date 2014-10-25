#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//#define READ_ONLY = "r";

#define WINDOWS_SIZE 100
#include "cbuffer.c"



int file_size;
int total_num_of_packets;
int sq_counter;




void read_packets (FILE * fp, CircularBuffer cb, int from);
void syserr(char * msg) { perror(msg); exit(-1); }

int main(int argc, char* argv[])
{
  int sockfd, portno, n;
  FILE * fp;
  struct hostent* server;
  struct sockaddr_in serv_addr;
  socklen_t addrlen;

  char buffer[256];

  if(argc != 4) {
    fprintf(stderr, "Usage: %s <hostname> <port> <file_name>\n", argv[0]);
    return 1;
  }

  server = gethostbyname(argv[1]);   //gethost 
  portno = atoi(argv[2]);           
  if(!server) {
    fprintf(stderr, "ERROR: no such host: %s\n", argv[1]);
    return 2;
  }

 // fp = fopen(argv[3], "r");  //open file
    fp = fopen("/Users/jtraviesor/Desktop/test.in", "r");
    if(!fp){
    fprintf(stderr, "ERROR: file %s couldn't be opened or not found:\n", argv[3]);
    return 3;
  }
    
  fseek(fp, 0L, SEEK_END);  //read file size
  file_size = ftell(fp);
  rewind(fp);
  total_num_of_packets = file_size / PAYLOAD_SIZE;
  sq_counter = 0;


  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    //create socket
  if(sockfd < 0) syserr("can't open socket");
  printf("create socket...\n");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
  serv_addr.sin_port = htons(portno);


    
  CircularBuffer packets;
  cb_init(packets,WINDOWS_SIZE);
  read_packets(fp, packets, 0);
  cb_print(packets);
    
    
    
    
    
  /*
    
    


  printf("PLEASE ENTER MESSAGE: ");
  fgets(buffer, 255, stdin);
  n = strlen(buffer); if(n>0 && buffer[n-1] == '\n') buffer[n-1] = '\0';

  addrlen = sizeof(serv_addr);
  n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&serv_addr, addrlen);
  if(n < 0) syserr("can't send to server");
  printf("send...\n");

  n = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr*)&serv_addr, &addrlen);
  if(n < 0) syserr("can't receive from server");
  printf("CLIENT RECEIVED MESSAGE: %s\n", buffer);

  close(sockfd);
   
  */
   
  return 0;
}

void read_packets(FILE *fp, CircularBuffer packets, int from)
{
    
    char buffer[PAYLOAD_SIZE];
    
    while(!is_empty(packets))
    {
        if(!fread(buffer, sizeof(buffer), 1, fp))
        {
            printf("Could not read 1k from file");
        }
        Packet p= {sq_counter++,total_num_of_packets,buffer};
        enqueue(packets,p);
    }
    
    
}





