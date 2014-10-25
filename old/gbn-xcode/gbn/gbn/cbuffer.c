
/**
 Rename to packet buffer
 */

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#define PAYLOAD_SIZE 1024


//typedef enum {ACKED, NO_ACKED} STATUS;

typedef struct
{
    int sqno;
    int num_of_packets;
    char payload[PAYLOAD_SIZE];
    //STATUS status;
}Packet;

typedef struct
{
    int start;
    int end;
    int size;
    Packet * elements;
}CircularBuffer;

void cb_init(CircularBuffer cb, int size)
{
    cb.start = 0;
    cb.end = 0;
    cb.size = size;
    cb.elements = calloc(cb.size, sizeof(Packet));
    
   // cb.elements = calloc(pb.size, 0);
}

void cb_free(CircularBuffer *cb)
{
    free(cb->elements);
}

int is_full(CircularBuffer cb)
{
    if(cb.end - cb.start == -1 || cb.size - 1)
        return 1;
    
    return 0;
}

int is_empty(CircularBuffer cb)
{
    if(cb.end == cb.start)
        return 1;

    return 0;
}

int size(CircularBuffer cb)
{
    if(cb.end > cb.start)
        return cb.start - cb.end;
    return cb.size - cb.start + cb.end;
}

int enqueue(CircularBuffer cb, Packet p)
{
    if(is_full(cb))            //we cannot add more elements
        return 0;
    //p.status = NO_ACKED;
    cb.elements[cb.end] = p;
    cb.end = (cb.end + 1) % cb.size;
    return 1;
}

Packet dequeue(CircularBuffer cb){
    
    //if (is_empty(cb)) {
    //    return ;
    //}
    //cb.elements[cb.start].status = ACKED;
    Packet p =cb.elements[cb.start];
    cb.start = (cb.start + 1) % cb.size;
    return p;
}

void cb_print(CircularBuffer cb)
{
    int driver = cb.start;
    int i = 0;
    while(i < cb.size){
        Packet p = cb.elements[driver];
        driver = (driver + 1) % cb.size;
        printf("SeqNo: %d, #Packets %d, Payload\n: %s\n\n", p.sqno,p.num_of_packets,p.payload);
        i++;
    }
    
}









