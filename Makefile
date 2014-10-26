CC=gcc
CFLAGS=-c -O -Wall
LDFLAGS=

SRC=sender.c cbuffer.c
OBJ=$(SRC:.c=.o)
HDR=cbuffer.h
EXEC=sender

SRC2=receiver.c
OBJ2=$(SRC2:.c=.o)
EXEC2=receiver


all: $(SRC) $(EXEC) $(EXEC2)
	
$(EXEC): $(OBJ) 
		$(CC) $(LDFLAGS) $(OBJ) -o $@

$(EXEC2): $(OBJ2) 
		$(CC) $(LDFLAGS) $(OBJ2) -o $@

%.o:	%.c $(HDR)
		$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJ) $(EXEC) $(OBJ2) $(EXEC2) cbuffer.o
