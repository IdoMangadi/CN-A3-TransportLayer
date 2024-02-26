#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

// RUDP Conventions:
#define SYN 7
#define ACKSYN 8
#define ACK 3
#define FIN 98
#define ACKFIN 99
#define SEND 10
#define RECV 11
#define BROKEN 101

// RUDP Elements:
#define TIMEOUT 50000 // in microseconds, => 50 in miliseconds => 0.05 in seconds
#define MAX_ACCEPT_TRIES 10  // Number of tries to accept a connection.

// TODO: CREATING HEADER FILE FOR #DEFINES, 

// RUDP header structure definition:
struct rudp_header {
    uint16_t length;
    uint16_t checksum;
    uint8_t flags;
};

/**
 * Creating a RUDP and creating a handshake between two peers.
*/
int rudp_socket(const char *server_ip, int server_port){}


/**
 * Sending data to the peer. the function should wait for an acknowledgment packet and if it didn't receive any, retransmits the data.
*/
int rudp_send(int sockfd, const void *data, size_t len, struct sockaddr_in *receiver_addr) {
    // TODO: CREATING PACKET WITH RUDP PROTOCOL, SENDING IT, WAIT FOR ACK, SEND AGAIN AFTER TIMEOUT
}


/**
 * Receive data from a peer.
*/
int rudp_receive(int sockfd, void *buffer, size_t buffer_size, struct sockaddr_in *sender_addr) {
    //TODO: RECEIVE DATA, CHECKSUM VALIDATION, SENDING N/ACK

}


/**
 * Closes a connections between peers.
*/
int rudp_close(int sockfd, struct sockaddr_in *receiver_addr) {
    // TODO: SEND FIN, 
}
