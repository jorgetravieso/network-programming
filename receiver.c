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

  fp = fopen(argv[2], "wb");  //open file
  //fp = fopen("/Users/jtraviesor/Desktop/test.out", "wb");
  if(!fp)
  {
    fprintf(stderr, "ERROR: file %s couldn't be opened or created:\n", argv[3]);
    return 3;
  }
  int expected = 0;
  
  for(;;) 
  {

    Packet p;
    addrlen = sizeof(clt_addr); 

    n = recvfrom(sockfd, (void*) &p, sizeof(p), 0, (struct sockaddr*)&clt_addr, &addrlen); 
    if(n < 0) syserr("can't receive from client"); 
    inet_ntoa(clt_addr.sin_addr), ntohs(clt_addr.sin_port); 
    if(p.sqno == expected && !is_corrupt(p)){

      printf("processing packet #:%d\n", p.sqno);
      expected++;
      write_packet(p, fp);
      AckPacket ack;
      ack.ack = p.sqno;
      n = sendto(sockfd, (void * )&ack, sizeof(ack), 0, (struct sockaddr*)&clt_addr, addrlen);
      if(n < 0) syserr("can't send ack to server"); 
      if(p.sqno + 1 == p.num_of_packets){
        printf("%s\n","the last packet was received" );
        fd_set readset;
        struct timeval timeout;                    
          timeout.tv_sec = 60;    /*set the timeout to 10 ms*/
        timeout.tv_usec = 0;
        FD_ZERO(&readset);
        FD_SET(sockfd, &readset);
        n = select(sockfd+1, &readset, NULL, NULL, &timeout);
        if(n < 0) syserr("can't receive from client"); 
        if (!FD_ISSET(sockfd, &readset)){
          break;
        }

      }
    }
    if(expected == p.num_of_packets){
        //we are on the las packet
      break;
    }



  }


  fclose(fp);
  close(sockfd); 
  return 0;
}

unsigned char checksum8(char * buf, int size)
{

  unsigned int sum = 0;
  int i;
  for(i = 0; i < size; i++)
  {
    sum += buf[i] & 0xff;
    sum = (sum >> 8) + (sum & 0xff);
  }
  return ~sum;
}

void write_packet(Packet p, FILE * stream)
{
  fwrite(p.payload,p.payload_size, 1,stream);
}

int is_corrupt(Packet p)
{
  int rec_checksum = p.checksum;
  p.checksum = 0;
  int new_checksum = checksum8((char *)&p, sizeof(p));

  if(new_checksum != rec_checksum){
    return 1;
  }
  return 0;
}

