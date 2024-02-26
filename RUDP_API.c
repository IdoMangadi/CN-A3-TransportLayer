#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include "RUDP.h"

// TODO: CREATING HEADER FILE FOR #DEFINES, 

// Static variables:
static struct sockaddr_in *s_receiver_addr = NULL;
static struct sockaddr_in *s_sender_addr = NULL;
static socklen_t s_addr_len = sizeof(struct sockaddr_in);


// RUDP header structure definition:
struct rudp_header {
    int16_t length;
    int16_t checksum;
    int8_t flags;
};


/**
 * Creating a RUDP. returning socket fd.
*/
int rudp_socket(){

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // Creating a standart UDP socket
    if (sockfd < 0) {
        perror("Error creating socket");
        return 1;
    }
    return sockfd;
    // TODO: THREE WAY HANDSHAKE, TIMEOUT.
}

/**
 * For the receiver side to accept a new connection and start receive data. 
 * implementing the three way handshake.
 * return -1 if connection didnt established.
*/
int rudp_accept(int sockfd, struct sockaddr_in *sender_addr, socklen_t addr_len){
    // Creating SYN header:
    struct rudp_header* syn_header;
    syn_header->flags = 0;
    // Creating ACKSYN header:
    struct rudp_header* acksyn_header;
    acksyn_header->flags = ACKSYN;
    // Creating ACK header:
    struct rudp_header* ack_header;
    ack_header->flags = 0;

    // Receiving connection (from sender with connect function):
    while(syn_header->flags != SYN){
        size_t bytes_received = recvfrom(sockfd, syn_header, sizeof(syn_header), 0, (struct sockaddr *)sender_addr, &addr_len);
    }

    // Sending ACKSYN:
    // Setting timeout:
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Defining maximum tries:
    int tries = 0;
    while(ack_header->flags != ACK && tries <= MAX_ACCEPT_TRIES){
        size_t bytes_sent = sendto(sockfd, acksyn_header, sizeof(acksyn_header), 0, (struct sockaddr *)sender_addr, sizeof(struct sockaddr_in));
        size_t bytes_received = recvfrom(sockfd, ack_header, sizeof(ack_header), 0, (struct sockaddr *)sender_addr, &addr_len);
        tries++;
    }
    if(tries > MAX_ACCEPT_TRIES){
        return -1;
    }
    // Updating addresses:
    s_sender_addr = sender_addr;
    return 0;
}

/**
 * For the Sender side to make a handshake with receiver.
 * return -1 if the handshake didnt succeeded.
*/
int rudp_connect(int sockfd, struct sockaddr_in *receiver_addr, socklen_t addr_len){

    // Sending SYN:
    // Init the SYN header:
    struct rudp_header* syn_header;
    syn_header->length = 0;
    syn_header->checksum = 0;
    syn_header->flags = SYN;

    // Setting timeout:
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Creating ACKSYN received packet and set it to 0:
    struct rudp_header* acksyn_packet;
    memset(acksyn_packet, 0, sizeof(acksyn_packet));

    // Creating an ACK header:
    struct rudp_header* ack_header;
    ack_header->length = 0;
    ack_header->checksum = 0;
    ack_header->flags = ACK;

    size_t bytes_sent;
    while(acksyn_packet->flags != ACKSYN){
        // Sending via UDP:
        bytes_sent = sendto(sockfd, syn_header, sizeof(syn_header), 0, (struct sockaddr *)receiver_addr, sizeof(struct sockaddr_in));

        // Receiving packet back:
        size_t bytes_received = recvfrom(sockfd, acksyn_packet, sizeof(acksyn_packet), 0, (struct sockaddr *)receiver_addr, &addr_len);
    }

    while(acksyn_packet->flags == ACKSYN){  // In this case, breaking the loop when stopping to get ACKSYN
        // Sending ACK:
        bytes_sent = sendto(sockfd, ack_header, sizeof(ack_header), 0, (struct sockaddr *)receiver_addr, sizeof(struct sockaddr_in));
        // Waiting for possible ACKSYN (defined by TIMEOUT):
        size_t bytes_received = recvfrom(sockfd, acksyn_packet, sizeof(acksyn_packet), 0, (struct sockaddr *)receiver_addr, &addr_len);
    }

    s_receiver_addr = receiver_addr;
    return 0;
}


/**
 * Sending data to the peer. the function should wait for an acknowledgment packet and if it didn't receive any, retransmits the data.
*/
int rudp_send(int sockfd, char *data, size_t len) {
    // TODO: CREATING PACKET WITH RUDP PROTOCOL, SENDING IT, WAIT FOR ACK, SEND AGAIN AFTER TIMEOUT

    // Init the header:
    struct rudp_header* header;
    header->length = len;
    // Checksum calculation:
    header->checksum = 0;
    for (size_t i=0; i<len; i++){
        header->checksum ^= data[i];
    }
    header->flags = SEND;
    
    // Allocate memory and copying values:
    char* packet_to_send[sizeof(struct rudp_header) + len];
    memcpy(packet_to_send, header, sizeof(struct rudp_header));  // Copying the header.
    memcpy(packet_to_send[sizeof(header)], data, len);  // Copying the data.

    // Setting timeout:
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Creating received packet and set it to 0:
    struct rudp_header* received_packet;
    memset(received_packet, 0, sizeof(received_packet));

    size_t bytes_sent;
    while(received_packet->flags != ACK){
        // Sending via UDP:
        bytes_sent = sendto(sockfd, packet_to_send, sizeof(packet_to_send), 0, (struct sockaddr *)s_receiver_addr, sizeof(struct sockaddr_in));

        // Receiving packet back:
        size_t bytes_received = recvfrom(sockfd, received_packet, sizeof(received_packet), 0, (struct sockaddr *)s_receiver_addr, &s_addr_len);
    }

    return bytes_sent;
}


/**
 * Receive data from a peer.
*/
int rudp_receive(int sockfd, void *buffer, size_t buffer_size) {
    //TODO: RECEIVE DATA, CHECKSUM VALIDATION, SENDING ACK / BROKEN

    char* received_packet[sizeof(struct rudp_header) + buffer_size];  // Space for maximum data size + rudp_header

    size_t bytes_received = recvfrom(sockfd, received_packet, sizeof(received_packet), 0, (struct sockaddr *)s_sender_addr, &s_addr_len);

    // Creating the parts of the received packet:
    struct rudp_header* header = received_packet;
    char* data = received_packet[sizeof(header)];

    // Checksum validation:
    int16_t tmp_checksum = 0;
    for (size_t i=0; i<buffer_size; i++){
        tmp_checksum ^= data[i];
    }

    // Sending response:
    struct rudp_header* ack_header;
    ack_header->length = 0;
    ack_header->checksum = 0;

    if (tmp_checksum == header->checksum){
        ack_header->flags = ACK;
    }
    else{
        ack_header->flags = BROKEN;
    }
    sendto(sockfd, ack_header, sizeof(ack_header), 0, (struct sockaddr *)s_sender_addr, sizeof(struct sockaddr_in));

    // Copy data to the buffer:
    memcpy(data, buffer, buffer_size);

    return bytes_received;
}


/**
 * Closes a connections between peers.
*/
int rudp_close(int sockfd) {
    // Sender side:
    if(s_receiver_addr != NULL){

        // Creating fin header:
        struct rudp_header* fin_header;
        fin_header->flags = FIN;

        // Creating ackfin header:
        struct rudp_header* ackfin_header;
        ackfin_header->flags = 0;

        // Setting timeout:
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        while(ackfin_header->flags != ACKFIN){
            // Sending the fin:
            size_t bytes_sent = sendto(sockfd, fin_header, sizeof(fin_header), 0, (struct sockaddr *)s_receiver_addr, s_addr_len);

            // Wating for ackfin:
            size_t bytes_received = recvfrom(sockfd, ackfin_header, sizeof(ackfin_header), 0, (struct sockaddr *)s_receiver_addr, &s_addr_len);
        }
    }

    // Receiver side:
    if(s_sender_addr != NULL){
        
        // Creating fin header:
        struct rudp_header* fin_header;
        fin_header->flags = 0;

        // Creating ackfin header:
        struct rudp_header* ackfin_header;
        ackfin_header->flags = ACKFIN;

        // Sending the ackfin header:

    }

    close(sockfd);
    return 0;
}
