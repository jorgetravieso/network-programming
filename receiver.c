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

int is_corrupt(Packet p);
void syserr(char *msg) { perror(msg); exit(-1); }
unsigned char checksum8(unsigned char * buf, int size);
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
  if(!fp)
  {
    fprintf(stderr, "ERROR: file %s couldn't be opened or created:\n", argv[3]);
    return 3;
  }



int expected = 0;
int x = 0;

for(;;) 
{

  printf("call #%d\n", ++x);



  Packet p;
  //printf("wait on port %d...\n", portno);
  addrlen = sizeof(clt_addr); 

  n = recvfrom(sockfd, (void*) &p, sizeof(p), 0, (struct sockaddr*)&clt_addr, &addrlen); 
  if(n < 0) syserr("can't receive from client"); 

  //printf("SERVER GOT MESSAGE: %s from client %s at port %d\n", buffer,
  //printf("%s\n", "received p:");
 

  //packet_print(p);
  //printf("SeqNo: %d, #Packets %d, Checksum: %d\n", p.sqno,p.num_of_packets, p.checksum);



	inet_ntoa(clt_addr.sin_addr), ntohs(clt_addr.sin_port); 
 if(!is_corrupt(p)){
    //printf("%s\n","we received a good packet" );
    if(p.sqno == expected){
      expected++;
      write_packet(p, fp);
      AckPacket ack;
      ack.ack = p.sqno;
      n = sendto(sockfd, (void * )&ack, sizeof(ack), 0, (struct sockaddr*)&clt_addr, addrlen);
      if(n < 0) syserr("can't send ack to server"); 
      printf("we sent ack:%d\n",ack.ack);
      if(p.sqno + 1 == p.num_of_packets){
            fd_set readset;
            struct timeval timeout;                    
            timeout.tv_sec = 1;    /*set the timeout to 10 ms*/
            FD_ZERO(&readset);
            FD_SET(sockfd, &readset);
            n = select(sockfd+1, &readset, NULL, NULL, &timeout);
            if(n < 0) syserr("can't receive from client"); 
            if (FD_ISSET(sockfd, &readset)){
              continue;
            }
            break;
      }
    }



  }
  else{
    printf("Bad checksum %d\n", p.sqno);
 }




//printf("SeqNo: %d, #Packets %d, Checksum: %d\n", p.sqno,p.num_of_packets, p.checksum);

    //packet_print(p);

  

  //n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&clt_addr, addrlen);
  //if(n < 0) syserr("can't send to server"); 
  // printf("send message...\n");

}
  fclose(fp);
  close(sockfd); 
  return 0;
}

unsigned char checksum8(unsigned char * buf, int size)
{
  

  unsigned char sum = 0;
  for(int i = 0; i < size; i++)
  {
    sum += buf[i];
  }
  sum = ~sum;
  sum = sum + 1;
  return sum;
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

int is_corrupt(Packet p)
{
  unsigned char rec_checksum = p.checksum;
  //printf("rec_checksum :%d\n", rec_checksum);

  p.checksum = 0;
  unsigned char new_checksum = checksum8((unsigned char *)&p, sizeof(p));
  //printf("new_checksum :%d\n", new_checksum);

  if(new_checksum != rec_checksum){
    printf("received %d\n", rec_checksum);
    printf("received %d\n", new_checksum);

    return 1;
  }
  return 0;
}

