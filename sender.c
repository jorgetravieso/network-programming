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



int file_size;
int total_num_of_packets;
int sq_counter;

int sockfd, portno, n;
FILE * fp;
struct hostent* server;
struct sockaddr_in serv_addr;
socklen_t addrlen;






void read_packets (FILE * fp, CircularBuffer * cb);
void syserr(char * msg) { perror(msg); exit(-1); }
unsigned char checksum8(char * buf, int size);
void send_packets(CircularBuffer * cb);
void send_packets_from(CircularBuffer * cb, int fromsqno);
void read_and_send(FILE *fp, CircularBuffer * packets);

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
  fp = fopen("/Users/jtraviesor/Desktop/test.in", "r");
  if(!fp){
    fprintf(stderr, "ERROR: file %s couldn't be opened or not found:\n", argv[3]);
    return 3;
  }

  fseek(fp, 0L, SEEK_END);  //read file size
  file_size = ftell(fp);
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
  //printf("%s\n", "We finished reading");
  //cb_print(&packets);


  //printf("Size: %d\n", cb_size(&packets));
  printf("And is the queue full? %d\n", is_full(&packets)); 

  send_packets(&packets);

  int lastack = 0;
  fd_set readset;
  struct timeval timeout;                    
  timeout.tv_usec = 10;    /*set the timeout to 10 ms*/

  //int count = 0;
  while(1){
    //if(count ==  10000) break;
    //count++;
   
    //cb_print(&packets);




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
        //read_packets(fp,&packets);
        lastack = ack.ack;
        read_and_send(fp,&packets);
        //send_packets_from(&packets, lastack);
        if(lastack == total_num_of_packets - 1)
          break;
      }
    }
    else{
      //timeout
      printf("%s\n", "timeout occurred");
      send_packets(&packets);
    }

    



 
  
}



















/*
n = recvfrom(sockfd, buffer, 255, 0, (struct sockaddr*)&serv_addr, &addrlen);
if(n < 0) syserr("can't receive from server");
printf("CLIENT RECEIVED MESSAGE: %s\n", buffer);

  */

close(sockfd);
return 0;
}

void read_packets(FILE *fp, CircularBuffer * packets)
{

 // printf("trying to read\n");

  //char buffer[PAYLOAD_SIZE];
 // int hey = 0; 


  while(!is_full(packets))
  {   
    Packet p;
    //printf("trying to read\n");
    //printf("size of payload: %d\n",sizeof(p.payload));
    memset(p.payload, 0, sizeof(p.payload));

    if(feof( fp )) break;
    else
    {
      /* Attempt to read in 10 bytes: */
      fread( p.payload, sizeof(p.payload),1, fp);
      //p.payload[nread] = '\0';
      if( ferror( fp ) ) 
      {
       perror( "Read error" );
       break;
     }
     p.sqno = sq_counter++;
     p.num_of_packets = total_num_of_packets;
     p.checksum = 0;
     p.checksum = checksum8((char *) &p, sizeof(p));
      //printf("%s\n", "after creating struct");
      //hey++;
     packet_print(p);
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


  int driver = packets->start;
  while(driver != packets->end){

    Packet p = packets->elements[driver];
    driver = (driver + 1) % packets->size;
    if(p.sqno < fromsqno){continue;}

    n = sendto(sockfd, (void *) &p, sizeof(p), 0, (struct sockaddr*)&serv_addr, addrlen);
    if(n < 0) syserr("can't send to server");
    printf("we sent p#:%d:\n", p.sqno);
    
    //printf("SeqNo: %d, #Packets %d, Checksum: %d\n", p.sqno,p.num_of_packets, p.checksum);
    
  } 
}



void read_and_send(FILE *fp, CircularBuffer * packets)
{

 // printf("trying to read\n");

  //char buffer[PAYLOAD_SIZE];
 // int hey = 0; 


  while(!is_full(packets))
  {   
    Packet p;
    //printf("trying to read\n");
    //printf("size of payload: %d\n",sizeof(p.payload));
    memset(p.payload, 0, sizeof(p.payload));

    if(feof( fp )) break;
    else
    {
      /* Attempt to read in 10 bytes: */
      fread( p.payload, sizeof(p.payload),1, fp);
      //p.payload[nread] = '\0';
      if( ferror( fp ) ) 
      {
       perror( "Read error" );
       break;
     }
     p.sqno = sq_counter++;
     p.num_of_packets = total_num_of_packets;
     p.checksum = 0;
     p.checksum = checksum8((char *) &p, sizeof(p));
     enqueue(packets,p);
     n = sendto(sockfd, (void *) &p, sizeof(p), 0, (struct sockaddr*)&serv_addr, addrlen);
     if(n < 0) syserr("can't send to server");
     printf("we sent p#:%d:\n", p.sqno);
   }



 }

}











