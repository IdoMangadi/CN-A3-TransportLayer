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
static uint8_t r_seq_number = 0;  // Representing the number of packets the receiver received in this session of communication.
static uint8_t s_seq_number = 0;  // Representing the number of packets the sender sent in this session of communication.


// RUDP header structure definition:
struct rudp_header {
    uint16_t length;
    uint16_t checksum;
    uint8_t flags;
    uint8_t seq_number;
};


/**
 * Creating a RUDP. returning socket fd.
*/
int rudp_socket() {
    int sockfd;

    // Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Error occured while creating socket");
        return -1;
    }

    // Set default receiver and sender addresses to NULL
    s_receiver_addr = NULL;
    s_sender_addr = NULL;

    return sockfd;
}

/**
 * Establishes a connection to the specified receiver.
 * Returns 0 on success, -1 on failure.
 */
int rudp_connect(int sockfd, const char *receiver_ip, int receiver_port) {
    // Create receiver address structure
    struct sockaddr_in receiver_addr;

    memset(&receiver_addr, 0, sizeof(receiver_addr));

    receiver_addr.sin_family = AF_INET;  // Connection type
    receiver_addr.sin_port = htons(receiver_port);  // Port
    if (inet_pton(AF_INET, receiver_ip, &receiver_addr.sin_addr) <= 0) {  // IP
        perror("inet_pton");
        return -1;
    }

    // Handshake performing:
    // Creating the header to send:
    struct rudp_header* header;
    header->length = 0;
    header->checksum = 0;
    header->flags = SYN;
    header->seq_number = 0;

    // Creating the ACKSYN packet:
    struct rudp_header* ack_packet;
    ack_packet->length = 0;
    ack_packet->checksum = 0;
    ack_packet->flags = 0;
    ack_packet->seq_number = 0;

    // Setting timeout:
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ssize_t tries = 0;

    // Handshake:
    while( ack_packet->flags != ACKSYN && tries < MAX_TRIES){ // Means handshake was not performed yet.
        // Sending SYN:
        ssize_t bytes_sent = sendto(sockfd, header, sizeof(header), 0, (struct sockaddr *)&receiver_addr, sizeof(struct sockaddr_in));

        // Receiving packet back (timeout defined):
        ssize_t bytes_received = recvfrom(sockfd, ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&receiver_addr, &s_addr_len);
        
        tries++;
    }

    // Limitting the SYN tries:
    if(tries = MAX_TRIES && ack_packet->flags != ACKSYN){
        return -1;
    }

    // Store the receiver address for future use
    memcpy(s_receiver_addr, &receiver_addr, sizeof(receiver_addr));

    return 0;
}


/**
 * Binds the socket to the specified local IP and port and wait for connection.
 * Returns 0 on success, -1 on failure, -2 for no incomming connections and -3 on sending ACKSYN failure.
 */
int rudp_bind(int sockfd, int port) { // I removed: const char *local_ip
    // Create server address structure
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); 
    serverAddr.sin_family = AF_INET;  // Connection type
    serverAddr.sin_port = htons(port);  // Port
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind socket to local address
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error occured whlie binding");
        return -1;
    }
    printf("RUDP Receiver started.\nWating for connections...\n");

    // First handshake try:
    // Creating SYN header:
    struct rudp_header* syn_header;
    syn_header->checksum = 0;
    syn_header->length = 0;
    syn_header->seq_number = 0;
    syn_header->flags = 0;

    // Creating ACKSYN header:
    struct rudp_header* acksyn_header;
    acksyn_header->checksum = 0;
    acksyn_header->length = 0;
    acksyn_header->seq_number = 0;
    acksyn_header->flags = ACKSYN;

    // Setting timeout to connections receiving:
    struct timeval timeout;
    timeout.tv_sec = INCOMMING_CONNECTIONS_TIMEOUT;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while(syn_header->flags != SYN){
        // Receiving SYN:
        ssize_t bytes_received = recvfrom(sockfd, syn_header, sizeof(syn_header), 0, (struct sockaddr *)s_sender_addr, &s_addr_len);
        if(bytes_received < 0){
            return -2;
        }
    }

    // Sending ACKSYN: (Sending only once, handling lost in recv function)
    if(sendto(sockfd, acksyn_header, sizeof(acksyn_header), 0, (struct sockaddr *)s_sender_addr, sizeof(struct sockaddr_in)) < 0){
        return -3;
    }

    return 0;
}


/**
 * Sending data to the peer. the function should wait for an acknowledgment packet and if it didn't receive any, retransmits the data.
 * The function only trying to send DATA - MAX_TRIES times.
*/
int rudp_send(int sockfd, char *data, size_t len) {  //  struct sockaddr_in *receiver_addr (i deleted it)

    // Handshake validation:
    if(s_receiver_addr == NULL){
        return -1;
    }

    // Creating the header to send:
    struct rudp_header* header;
    header->length = 0;
    header->checksum = 0;
    header->flags = SYN;
    header->seq_number = 0;

    // Creating ACK's packet:
    struct rudp_header* ack_packet;
    ack_packet->length = 0;
    ack_packet->checksum = 0;
    ack_packet->flags = 0;
    ack_packet->seq_number = 0;

    // Setting timeout:
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Variables:
    ssize_t bytes_sent;
    ssize_t bytes_received;
    ssize_t tries = 0;

    // Creating DATA packet:
    header->flags = DATA;
    header->seq_number = ++s_seq_number;
    char tos_packet[sizeof(header) + len];
    memcpy(tos_packet, header, sizeof(struct rudp_header));  // Copying the header.
    memcpy(tos_packet[sizeof(header)], data, len);  // Copying the data.

    ack_packet->flags = 0;

    // Sending proccess:
    while(ack_packet->flags != ACK && tries < MAX_TRIES){
        // Sending the DATA packet:
        bytes_sent = sendto(sockfd, tos_packet, sizeof(tos_packet), 0, (struct sockaddr *)s_receiver_addr, sizeof(struct sockaddr_in));

        // Receiving the ACK packet:
        bytes_received = recvfrom(sockfd, ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)s_receiver_addr, &s_addr_len);
    }

    // Limitting the DATA tries:
    if(tries > MAX_TRIES && ack_packet->flags != ACK){
        s_seq_number--;  // Updating sequence number to the privious sent packet.
        return -1; 
    }

    return bytes_sent; 
}



/**
 * Receive data from a peer.
 * Returning -1 if nothing received from sender 
 * Returning -2 if sender sent FIN.
 * Returning -3 for errors.
*/
int rudp_receive(int sockfd, void *buffer, size_t buffer_size, struct sockaddr_in *sender_addr) {

    // Setting the buffer size to receive all data at one recvfrom command:
    int recvBufferSize = BUFFER_SIZE + sizeof(struct rudp_header); // Set to maximum buffer size
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvBufferSize, sizeof(recvBufferSize)) == -1) {
        perror("Error using setsockopt");
        return -3;
    }

    // Getting data from s_sender. handling: resent SYN, DATA and FIN inside:
    int got_data = 0;
    ssize_t bytes_received;
    while(got_data == 0){

        // Setting timeout to data receiving:
        struct timeval timeout;
        timeout.tv_sec = INCOMMING_DATA_TIMEOUT;
        timeout.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        // Receiving packet:
        char* packet_received[sizeof(struct rudp_header) + buffer_size]; // Space for maximum data size + rudp_header
        bytes_received = recvfrom(sockfd, packet_received, sizeof(packet_received), 0, (struct sockaddr *)s_sender_addr, sizeof(struct sockaddr_in));
        // Handling not communicating sender:
        if(bytes_received < 0){
            s_sender_addr = NULL;  // Means next time the sender will have to handshake.
            r_seq_number = 0;
            return -1;
        }
        // Splitting the header:
        struct rudp_header* header = packet_received;
        char* data = packet_received[sizeof(header)];

        // Handling resent SYN:
        if(header->flags == SYN){
            // Creating ACKSYN header:
            struct rudp_header* acksyn_header;
            acksyn_header->checksum = 0;
            acksyn_header->length = 0;
            acksyn_header->seq_number = 0;
            acksyn_header->flags = ACKSYN;

            // Sending ACKSYN:
            ssize_t bytes_sent = sendto(sockfd, acksyn_header, sizeof(acksyn_header), 0, (struct sockaddr *)sender_addr, sizeof(struct sockaddr_in));
            if(bytes_received < 0){
                perror("Error occured while handshake.\n");
                return -3;
            }
            continue;
        }

        // Handling DATA receiving:
        if(header->flags == DATA){
            // Handling bug of receiving in pieces: 
            // I handled this by setting a BUFFER_SIZE with setsockopt 

            // Building response packet:
            struct rudp_header* res_header;
            res_header->checksum = 0;
            res_header->length = 0;
            res_header->seq_number = 0;
            res_header->flags = ACK;

            // Sequence number validation:
            if(header->seq_number <= r_seq_number){  // Means already have the packet, sender didnt receive the ACK
                res_header->seq_number = s_seq_number;
                size_t bytes_sent = sendto(sockfd, res_header, sizeof(res_header), 0, (struct sockaddr *)s_sender_addr, sizeof(struct sockaddr_in));
                if(bytes_sent < 0){
                    perror("Error occured while ACK sending.\n");
                    return -3;
                }
                continue;  //Continue to wait the correct packet in sequence.
            }

            // Checksum validation:
            int16_t tmp_checksum = 0;
            for (size_t i=0; i<buffer_size; i++){
                tmp_checksum ^= data[i];
            }
            // Updating NACK for incorrect checksum:
            if(tmp_checksum != header->checksum){
                res_header->flags = NACK;
            }
            else{ // Means checksum correct:
                // Copy data to the buffer:
                memcpy(data, buffer, buffer_size);
                r_seq_number++;  // Updaing receiver sequence number. 
                res_header->seq_number = r_seq_number;
                got_data = 1;
            }

            // Sending the response packet:
            size_t bytes_sent = sendto(sockfd, res_header, sizeof(res_header), 0, (struct sockaddr *)s_sender_addr, sizeof(struct sockaddr_in));
            if(bytes_sent < 0){
                perror("Error occured while N/ACK sending.\n");
                return -3;
            }
        }

        // Handling FIN:
        if(header->flags == FIN){
            // Creating ACKFIN header:
            struct rudp_header* ackfin_header;
            ackfin_header->checksum = 0;
            ackfin_header->length = 0;
            ackfin_header->seq_number = 0;
            ackfin_header->flags = ACKFIN;

            // Sending ACKFIN:
            size_t bytes_sent = sendto(sockfd, ackfin_header, sizeof(ackfin_header), 0, (struct sockaddr *)s_sender_addr, sizeof(struct sockaddr_in));
            s_sender_addr = NULL;
            r_seq_number = 0;
            return -2;
            
        }

    }

    return bytes_received - sizeof(struct rudp_header);
}




/**
 * Closes a connections between peers.
*/
int rudp_close(int sockfd) {
    // Sender side:
    if(s_receiver_addr != NULL){  // Means there was a handshake from the sender:

        // Creating FIN header:
        struct rudp_header* fin_header;
        fin_header->length = 0;
        fin_header->checksum = 0;
        fin_header->seq_number = 0;
        fin_header->flags = FIN;

        // Creating ackfin header:
        struct rudp_header* ackfin_header;
        ackfin_header->length = 0;
        ackfin_header->checksum = 0;
        ackfin_header->seq_number = 0;
        ackfin_header->flags = 0;

        // Setting timeout:
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        //Variables:
        int tries = 0;

        while(ackfin_header->flags != ACKFIN && tries < MAX_TRIES){
            // Sending the fin:
            ssize_t bytes_sent = sendto(sockfd, fin_header, sizeof(fin_header), 0, (struct sockaddr *)s_receiver_addr, s_addr_len);

            // Wating for ackfin:
            ssize_t bytes_received = recvfrom(sockfd, ackfin_header, sizeof(ackfin_header), 0, (struct sockaddr *)s_receiver_addr, &s_addr_len);
            tries++;
        }
        s_receiver_addr = NULL;
        s_seq_number = 0;  // Init sequence number for more connections to the receiver.

    }

    // Receiver side:
    if(s_sender_addr != NULL){
        s_sender_addr = NULL;
        r_seq_number = 0;
    }

    close(sockfd);
    return 0;
}
