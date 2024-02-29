FLAGS = -Wall -g

.PHONY: all clean

all: TCP_Receiver TCP_Sender RUDP_Receiver RUDP_Sender

# Creating TCP Receiver and Sender excutable files:
TCP_Receiver: TCP_Receiver.c
	gcc $(FLAGS) TCP_Receiver.c -o TCP_Receiver

TCP_Sender: TCP_Sender.c
	gcc $(FLAGS) TCP_Sender.c -o TCP_Sender

# Creating RUDP Receiver and Sender excutable files:
RUDP_Receiver: RUDP_Receiver.o RUDP_API.o
	gcc $(FLAGS) RUDP_Receiver.o RUDP_API.o -o RUDP_Receiver

RUDP_Sender: RUDP_Sender.o RUDP_API.o
	gcc $(FLAGS) RUDP_Sender.o RUDP_API.o -o RUDP_Sender

#Creating OBJ files:
RUDP_API.o: RUDP_API.c RUDP.h
	gcc -c $(FLAGS) RUDP_API.c -o RUDP_API.o

RUDP_Receiver.o: RUDP_Receiver.c RUDP.h
	gcc -c $(FLAGS) RUDP_Receiver.c -o RUDP_Receiver.o

RUDP_Sender.o: RUDP_Sender.c RUDP.h
	gcc -c $(FLAGS) RUDP_Sender.c -o RUDP_Sender.o

# Cleaning:
clean:
	rm -f TCP_Sender TCP_Receiver RUDP_Receiver RUDP_Sender *.o