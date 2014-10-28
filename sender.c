#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>
//#define READ_ONLY = "r";

#define WINDOWS_SIZE 100
#include "cbuffer.h"


void read_packets (FILE * fp, CircularBuffer * cb);
void syserr(char * msg) { perror(msg); exit(-1); }
unsigned char checksum8(char * buf, int size);
void send_packets(CircularBuffer * cb);
void send_packets_from(CircularBuffer * cb, int fromsqno);
void read_and_send(FILE *fp, CircularBuffer * packets);


int file_size;
int total_num_of_packets;
int remaining;
int sq_counter;

int sockfd, portno, n;
FILE * fp;
struct hostent* server;
struct sockaddr_in serv_addr;
socklen_t addrlen;


int main(int argc, char* argv[])
{


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
  fp = fopen("/Users/jtraviesor/Desktop/test.in", "rb");
  if(!fp){
    fprintf(stderr, "ERROR: file %s couldn't be opened or not found:\n", argv[3]);
    return 3;
  }

  fseek(fp, 0L, SEEK_END);  //read file size
  file_size = ftell(fp);
  remaining = file_size;
  rewind(fp);
  total_num_of_packets =  ceil((file_size * 1.0 )/ PAYLOAD_SIZE);
  sq_counter = 0;

  printf("file_size: %d\n", file_size);

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    //create socket
  if(sockfd < 0) syserr("can't open socket");
  printf("create socket...\n");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
  serv_addr.sin_port = htons(portno);

  addrlen = sizeof(serv_addr);



  CircularBuffer packets;
  cb_init(&packets,WINDOWS_SIZE);
  read_packets(fp, &packets);





  send_packets(&packets);

  int lastack = 0;
  fd_set readset;
  struct timeval timeout;                    
  timeout.tv_sec = 0;    
  timeout.tv_usec = 10;    /*set the timeout to 10 ms*/

  while(1){

   // printf("lastack: %d total_num_of_packets: %d \n", lastack, total_num_of_packets);


    FD_ZERO(&readset);
    FD_SET(sockfd, &readset);
    n = select(sockfd+1, &readset, NULL, NULL, &timeout);
    if(n < 0) syserr("can't receive from client"); 
    if (FD_ISSET(sockfd, &readset)){
      AckPacket ack;
      n = recvfrom(sockfd, (void*) &ack, sizeof(ack), 0, (struct sockaddr*)&serv_addr, &addrlen); 
      if(n < 0) syserr("can't receive ack from client"); 
      printf("We received ack#:%d\n", ack.ack);

      if(ack.ack >= lastack){
        int i;
        for(i = lastack; i <= ack.ack; i++){
          Packet p = peek(&packets);
          if(p.sqno == i){
            dequeue(&packets,&p);
          }  
        }
        lastack = ack.ack;
        read_and_send(fp,&packets);
        if(lastack == total_num_of_packets - 1)
          break;
      }
    }
    else{
     // printf("%s\n", "timeout occurred");
      send_packets(&packets);
    }

    





  }


  fclose(fp);
  close(sockfd);
  return 0;
}

void read_packets(FILE *fp, CircularBuffer * packets)
{



  while(!is_full(packets))
  {   
    Packet p;
    memset(p.payload, 0, sizeof(p.payload));

    if(feof( fp )) break;
    else
    {

      fread( p.payload, sizeof(p.payload),1, fp);
      if( ferror( fp ) ) 
      {
       perror( "Read error" );
       break;
     }
     p.sqno = sq_counter++;
     p.num_of_packets = total_num_of_packets;

     if(remaining > PAYLOAD_SIZE) p.payload_size = PAYLOAD_SIZE;
     else  p.payload_size = remaining;
     remaining -= PAYLOAD_SIZE;
     p.checksum = 0;
     p.checksum = checksum8((char *) &p, sizeof(p));
     enqueue(packets,p);
   }



 }



  // printf("We read packets: %d\n", hey);


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

void send_packets(CircularBuffer * packets){
  send_packets_from(packets, 0);
}

void send_packets_from(CircularBuffer * packets, int fromsqno){

  int i = 0;
  int driver = packets->start;

  if(packets->current_size == 1){
    i++;
    Packet p = packets->elements[driver];
    driver = (driver + 1) % packets->size;
    n = sendto(sockfd, (void *) &p, sizeof(p), 0, (struct sockaddr*)&serv_addr, addrlen);
    if(n < 0) syserr("can't send to server");
    printf("we sent p#:%d:\n", p.sqno);

  }
  else{
    while( driver != packets->end ) {
      i++;
      Packet p = packets->elements[driver];
      driver = (driver + 1) % packets->size;
      if(p.sqno < fromsqno){continue;}

      n = sendto(sockfd, (void *) &p, sizeof(p), 0, (struct sockaddr*)&serv_addr, addrlen);
      if(n < 0) syserr("can't send to server");
      printf("we sent p#:%d:\n", p.sqno);

    }
    
  }
  printf("We sent %d packets\n", i);

}



void read_and_send(FILE *fp, CircularBuffer * packets)
{


  while(!is_full(packets))
  {   
    Packet p;
    memset(p.payload, 0, sizeof(p.payload));

    if(feof( fp )) break;
    else
    {
      fread( p.payload, sizeof(p.payload),1, fp);
      if( ferror( fp ) ) syserr("can't send to server");
      p.sqno = sq_counter++;
      p.num_of_packets = total_num_of_packets;
      
      if(remaining > PAYLOAD_SIZE) p.payload_size = PAYLOAD_SIZE;
      else  p.payload_size = remaining;
      remaining -= PAYLOAD_SIZE;
      p.checksum = 0;
      p.checksum = checksum8((char *) &p, sizeof(p));
      enqueue(packets,p);

      n = sendto(sockfd, (void *) &p, sizeof(p), 0, (struct sockaddr*)&serv_addr, addrlen);
      if(n < 0) syserr("can't send to server");
      printf("we sent p#:%d:\n", p.sqno);
    }



  }

}











