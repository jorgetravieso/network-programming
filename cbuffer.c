

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cbuffer.h" 


void cb_init(CircularBuffer * cb, int size)
{
    cb->start = 0;
    cb->end = 0;
    cb->size = size + 1;
}

void cb_free(CircularBuffer *cb)
{
    free(cb->elements);
}

int is_full(CircularBuffer * cb)
{
    return (cb->end + 1) % cb->size == cb->start;
}

int is_empty(CircularBuffer * cb)
{
    return (cb->end == cb->start);
}

int cb_size(CircularBuffer * cb)
{
    if(cb->end > cb->start)
        return abs(cb->start - cb->end);
    return abs(cb->size - cb->start + cb->end);
}

int enqueue(CircularBuffer * cb, Packet  p)
{
    if(is_full(cb)) //we cannot add more elements
    {           
        printf("%s\n", "The queue is full");
        return 0;

    }
    cb->elements[cb->end] = p;
    cb->end = (cb->end + 1) % cb->size;
    return 1;
}

Packet dequeue(CircularBuffer * cb){

    Packet p =cb->elements[cb->start];
    cb->start = (cb->start + 1) % cb->size;
    return p;
}

Packet peek(CircularBuffer * cb){

    Packet p =cb->elements[cb->start];
    return p;
}


void cb_print(CircularBuffer * cb)
{
    int driver = cb->start;
    //int i = 0;
    
    while(driver != cb->end)
    {
        Packet p = cb->elements[driver];
        driver = (driver + 1) % cb->size;
        packet_print(p);
       // i++;   
    }
}

void packet_print(Packet p)
{
    printf("SeqNo: %d, #Packets %d, Checksum: %d", p.sqno,p.num_of_packets, p.checksum);
    char c = 0;
    int index = 0;
    printf("%s\n", "Payload: ");
    while(index < PAYLOAD_SIZE && (c = p.payload[index++])!= 0)
    {
     fputc(c,stdout);
    }
    printf("\n\n");
}









