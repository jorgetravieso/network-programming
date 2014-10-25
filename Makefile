CC=gcc
CFLAGS=-c -Wall
LDFLAGS=

SRC=sender.c cbuffer.c
OBJ=$(SRC:.c=.o)
HDR=cbuffer.h
EXEC=sender

all: $(SRC) $(EXEC)
	
$(EXEC): $(OBJ) 
		$(CC) $(LDFLAGS) $(OBJ) -o $@

%.o:	%.c $(HDR)
		$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
