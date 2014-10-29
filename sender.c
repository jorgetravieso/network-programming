//sudo mn --link tc,loss=10

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>
#include "cbuffer.h"                                            //circular buffer/queue ds + typedef of packets

#define WINDOWS_SIZE 100                                        //windows size of the the GBN
#define LOGGING_MODE 1                                          //if set to true, shows the logging and debugging printfs



void read_packets (FILE * fp, CircularBuffer * cb);             //reads a upto 100 packets from the input file
void syserr(char * msg) { perror(msg); exit(-1); }              //exits the program
unsigned char checksum8(char * buf, int size);                  //checksum 8 algorithm
void send_packets(CircularBuffer * cb);                         //send all packets in the window
void send_packets_from(CircularBuffer * cb, int fromsqno);      //send all packets from a starting sequence number
void read_and_send(FILE *fp, CircularBuffer * packets);         //read and send packet at the time until the buffer is full


int file_size;                                                  
int total_num_of_packets;
int remaining;                  //remaining number of bytes to send
int sq_counter;                 //keeps track of the next squence number for the packet  

int sockfd, portno, n;          // socket, port and container variable
FILE * fp;                      //input stream
struct hostent* server;         //address of receiver
struct sockaddr_in serv_addr; 
socklen_t addrlen;


int main(int argc, char* argv[])
{

  //validation
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

  fp = fopen(argv[3], "rb");  //open file
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

  if(LOGGING_MODE) printf("file_size: %d\n", file_size);

  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    //create socket
  if(sockfd < 0) syserr("can't open socket");           //check for errors
  printf("creating socket...\n");   
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
  serv_addr.sin_port = htons(portno);

  addrlen = sizeof(serv_addr);



  CircularBuffer packets;                                //circular buffer/queue
  cb_init(&packets,WINDOWS_SIZE);                        //init the buffer
  read_packets(fp, &packets);                            //reads upto the first 100 packets
  send_packets(&packets);                                //sends them

  int lastack = 0;
  fd_set readset;
  struct timeval timeout;                    
  timeout.tv_sec = 0;    
  timeout.tv_usec = 10;                                  /*set the timeout to 10 ms*/

  for(;;){                                              //forever

    FD_ZERO(&readset);                                  //do select
    FD_SET(sockfd, &readset);
    n = select(sockfd+1, &readset, NULL, NULL, &timeout);
    if(n < 0) syserr("error on select()"); 
    if (FD_ISSET(sockfd, &readset)){                    //if we get some acks
      AckPacket ack;
      n = recvfrom(sockfd, (void*) &ack, sizeof(ack), 0, (struct sockaddr*)&serv_addr, &addrlen); 
      if(n < 0) syserr("can't receive ack from client"); 
      if(LOGGING_MODE) printf("We received ack#:%d\n", ack.ack);
                                                        //dequeue all the packets with sqno <= ack
      if(ack.ack >= lastack){
        int i;
        for(i = lastack; i <= ack.ack; i++){
          Packet p = peek(&packets);
          if(p.sqno == i){
            dequeue(&packets,&p);
          }  
        }
        lastack = ack.ack;                               //keep track of this last ack
        read_and_send(fp,&packets);                      //read and sent more packets one at the time until buffer is full
        if(lastack == total_num_of_packets - 1)          //if we received the last ack we break 
          break;
      }
      uint16_t temp = -1;
      if(ack.ack == temp){                                  //if the last ack was not received the receiver will keep sending -1
        break;  
      }
    }
    else{                                                 //timeout we send the whole window
      if(LOGGING_MODE) printf("%s\n", "*** timeout occurred ** ");  
      send_packets(&packets);                             
    }

  }


  fclose(fp);                                             //close the socket and input file
  close(sockfd);
  return 0;
}

//reads a upto 100 packets from the input file
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
}

//checksum 8 algorithm
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

//send all packets in the window
void send_packets(CircularBuffer * packets){
  send_packets_from(packets, 0);
}

//send all packets from a starting sequence number
void send_packets_from(CircularBuffer * packets, int fromsqno){

  int driver = packets->start;

  while( driver != packets->end ) {
    Packet p = packets->elements[driver];
    driver = (driver + 1) % packets->size;
    if(p.sqno < fromsqno){continue;}

    n = sendto(sockfd, (void *) &p, sizeof(p), 0, (struct sockaddr*)&serv_addr, addrlen);
    if(n < 0) syserr("can't send to server");
    if(LOGGING_MODE) printf("sent packet #:%d:\n", p.sqno);

  }
  //we need to send the last packet 
  Packet p = packets->elements[driver];
  driver = (driver + 1) % packets->size;
  n = sendto(sockfd, (void *) &p, sizeof(p), 0, (struct sockaddr*)&serv_addr, addrlen);
  if(n < 0) syserr("can't send to server");
  if(LOGGING_MODE) printf("sent packet #:%d:\n", p.sqno);

}



//read and send packet at the time until the buffer is full
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
      if(LOGGING_MODE) printf("sent packet #:%d:\n", p.sqno);
    }



  }

}











