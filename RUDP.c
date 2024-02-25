#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

// TODO: CREATING HEADER FILE FOR #DEFINES, 

// RUDP header structure definition:
struct rudp_header {
    uint16_t length;
    uint16_t checksum;
    uint8_t flags;
    void* payload;
};

/**
 * Creating a RUDP and creating a handshake between two peers.
*/
int rudp_socket(const char *server_ip, int server_port){

    int sock = socket(AF_INET, SOCK_DGRAM, 0);  // Creating a standart UDP socket
    if (sock < 0) {
        perror("Error creating socket");
        return 1;
    }
    // TODO: THREE WAY HANDSHAKE, TIMEOUT.
}


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
