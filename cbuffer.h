
#define PAYLOAD_SIZE 1024
#define WINDOWS_SIZE 100

typedef struct
{
    int sqno;
    int num_of_packets;
    int checksum;
    char payload [PAYLOAD_SIZE];
}Packet;

typedef struct
{	
	int ack;
}AckPacket;


typedef struct
{
    int start;
    int end;
    int size;
    Packet elements[WINDOWS_SIZE];
    int current_size;
}CircularBuffer;


void cb_init(CircularBuffer * cb, int size);
void cb_free(CircularBuffer *cb);
int is_full(CircularBuffer * cb);
int is_empty(CircularBuffer * cb);
int cb_size(CircularBuffer * cb);
int enqueue(CircularBuffer * cb, Packet p);
int dequeue(CircularBuffer * cb, Packet * p);
Packet peek(CircularBuffer * cb);
void cb_print(CircularBuffer * cb);
void packet_print(Packet p);