#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "cbuffer.h"

#define WINDOWS_SIZE 100

void syserr(char *msg) { perror(msg); exit(-1); }
unsigned char checksum8(char * buf, int size);
void write_packet(Packet p, FILE * stream);


int main(int argc, char *argv[])
{
  int sockfd, portno, n;
  FILE * fp;
  struct sockaddr_in serv_addr, clt_addr; 
  socklen_t addrlen;
  //char buffer[256];

  if(argc != 3) { 
    fprintf(stderr,"Usage: %s <port> <file_name>\n", argv[0]);
    return 1;
  } 
  portno = atoi(argv[1]);

  sockfd = socket(AF_INET, SOCK_DGRAM, 0); 
  if(sockfd < 0) syserr("can't open socket"); 
  printf("create socket...\n");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
    syserr("can't bind");
  printf("bind socket to port %d...\n", portno);

  // fp = fopen(argv[3], "r");  //open file
  fp = fopen("/Users/jtraviesor/Desktop/test.out", "w");
  if(!fp){
    fprintf(stderr, "ERROR: file %s couldn't be opened or created:\n", argv[3]);
    return 3;
  }


for(;;) {

  printf("wait on port %d...\n", portno);
  addrlen = sizeof(clt_addr); 

  n = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr*)&clt_addr, &addrlen); 
  if(n < 0) syserr("can't receive from client"); 
  else buffer[n] = '\0';

  //printf("SERVER GOT MESSAGE: %s from client %s at port %d\n", buffer,
	inet_ntoa(clt_addr.sin_addr), ntohs(clt_addr.sin_port)); 


  //n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clt_addr, addrlen);
  //if(n < 0) syserr("can't send to server"); 
  // printf("send message...\n");

}

  close(sockfd); 
  return 0;
}

unsigned char checksum8(char * buf, int size)
{
  unsigned int sum = 0;
  for(int i = 0; i < size; i++)
  {
    sum += buf[i] & 0xff;
    sum = (sum >> 8) + (sum & 0xff);
  }
  return ~sum;
}

void write_packet(Packet p, FILE * stream)
{
    char c = 0;
    int index = 0;
    while(index < PAYLOAD_SIZE && (c = p.payload[index++])!= 0)
    {
       fputc(c,stream);
    }
}

