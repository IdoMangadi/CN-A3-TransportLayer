FLAGS = -Wall -g

.PHONY: all clean

all: TCP_Receiver TCP_Sender

# Creating Receiver and Sender excutable files:
TCP_Receiver: TCP_Receiver.c
	gcc $(FLAGS) TCP_Receiver.c -o TCP_Receiver

TCP_Sender: TCP_Sender.c
	gcc $(FLAGS) TCP_Sender.c -o TCP_Sender

# Cleaning:
clean:
	rm -f TCP_Sender TCP_Receiver *.o