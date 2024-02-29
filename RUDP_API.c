#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include "RUDP.h"


// Static variables:
static struct sockaddr_in *stat_receiver_addr = NULL;
static struct sockaddr_in *stat_sender_addr = NULL;
static socklen_t s_addr_len = sizeof(struct sockaddr_in);
static uint8_t recv_seq_number = 0;  // Representing the number of packets the receiver received in this session of communication.
static uint8_t send_seq_number = 0;  // Representing the number of packets the sender sent in this session of communication.


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
        return -1;
    }

    // Set default receiver and sender addresses to NULL
    stat_receiver_addr = NULL;
    stat_sender_addr = NULL;

    return sockfd;
}

/**
 * Establishes a connection with a specified receiver.
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
    struct rudp_header header;
    header.length = 0;
    header.checksum = 0;
    header.flags = SYN;
    header.seq_number = 0;

    // Creating the ACKSYN packet:
    struct rudp_header ack_packet;
    ack_packet.length = 0;
    ack_packet.checksum = 0;
    ack_packet.flags = 0;
    ack_packet.seq_number = 0;

    // Setting timeout:
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    ssize_t tries = 0;

    // Handshake:
    while(ack_packet.flags != ACKSYN && tries < MAX_TRIES){ // Means handshake was not performed yet.
        // Sending SYN:
        ssize_t bytes_sent = sendto(sockfd, &header, sizeof(header), 0, (struct sockaddr *)&receiver_addr, sizeof(struct sockaddr_in));
        if(bytes_sent < 0){
            return -1;
        }

        // Receiving packet back (timeout defined):
        ssize_t bytes_received = recvfrom(sockfd, &ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)&receiver_addr, &s_addr_len);
        if(bytes_sent < 0){
            return -1;
        }
        
        tries++;
    }

    // Limitting the SYN tries:
    if(tries = MAX_TRIES && ack_packet.flags != ACKSYN){
        return -1;
    }

    // Store the receiver address for future use
    memcpy(stat_receiver_addr, &receiver_addr, sizeof(receiver_addr));
    send_seq_number = 0;

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
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // IP

    // Bind socket to local address
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error occured whlie binding");
        return -1;
    }
    // printf("RUDP Receiver started.\nWating for connections...\n");

    // First handshake try:
    // Creating SYN header:
    struct rudp_header syn_header;
    syn_header.checksum = 0;
    syn_header.length = 0;
    syn_header.seq_number = 0;
    syn_header.flags = 0;

    // Creating ACKSYN header:
    struct rudp_header acksyn_header;
    acksyn_header.checksum = 0;
    acksyn_header.length = 0;
    acksyn_header.seq_number = 0;
    acksyn_header.flags = ACKSYN;

    // Setting timeout to connections receiving (in seconds):
    struct timeval timeout;
    timeout.tv_sec = INCOMMING_CONNECTIONS_TIMEOUT;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Note: in order to handle sender trying to send DATA instead of SYN, the timeout resets for any incomming message. 
    //       This way it will have a chance to commit a handshake before receiver closes.

    while(syn_header.flags != SYN){
        // Receiving SYN:
        ssize_t bytes_received = recvfrom(sockfd, &syn_header, sizeof(syn_header), 0, (struct sockaddr *)stat_sender_addr, &s_addr_len);
        if(bytes_received < 0){
            return -2;
        }
    }

    // Sending ACKSYN: (Sending only once, handling lost in recv function)
    if(sendto(sockfd, &acksyn_header, sizeof(acksyn_header), 0, (struct sockaddr *)stat_sender_addr, sizeof(struct sockaddr_in)) < 0){
        return -3;
    }
    recv_seq_number = 0;
    return 0;
}


/**
 * Sending data to the peer. the function should wait for an acknowledgment packet and if it didn't receive any, retransmits the data.
 * The function only trying to send DATA - MAX_TRIES times.
*/
int rudp_send(int sockfd, const void *data, size_t len) {  //  struct sockaddr_in *receiver_addr (i deleted it)

    // Handshake validation:
    if(stat_receiver_addr == NULL){
        return -1;
    }

    // Variables:
    size_t total_sent = 0;
    size_t bytes_to_send = MAX_SEG_SIZE-sizeof(struct rudp_header);
    char* data_ptr = data;

    // Setting timeout:
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Creating the DATA's header:
    struct rudp_header header;
    header.length = 0;
    header.checksum = 0;
    header.flags = DATA;
    header.seq_number = send_seq_number;  // In the first segment: this assisgnment is 0 but will actually send 1.

    while(total_sent < len){  // Means there is more data to send.

        // Creating ACK's packet:
        struct rudp_header ack_packet;
        ack_packet.length = 0;
        ack_packet.checksum = 0;
        ack_packet.flags = NACK;
        ack_packet.seq_number = 0;

        if((len - total_sent) < bytes_to_send){  // Modifying the actual bytes to send if needed.
            bytes_to_send = len - total_sent;
        }
        header.length = bytes_to_send;  // Setting data len.
        for (size_t i=0; i<bytes_to_send; i++){  // Setting checksum.
            header.checksum ^= data_ptr[i];
        }
        header.seq_number = (header.seq_number == 0) ? 1 : 0;  // Setting sequence number in header
        send_seq_number = header.seq_number;  // Updating sender's seuence number.

        // Building the packet to send:
        char to_send_packet[MAX_SEG_SIZE];
        memcpy(to_send_packet, &header, sizeof(header));  // Copying the header.
        memcpy(to_send_packet[sizeof(header)], data_ptr, bytes_to_send);  // Copying the data.

        // Sending proccess:
        ssize_t bytes_sent = 0;
        ssize_t bytes_received = 0;
        ssize_t tries = 0;
        while((ack_packet.flags != ACK || (ack_packet.flags == ACK && ack_packet.seq_number != send_seq_number)) && tries < MAX_TRIES){
            // Sending the DATA packet:
            bytes_sent = sendto(sockfd, to_send_packet, sizeof(header) + bytes_to_send, 0, (struct sockaddr *)stat_receiver_addr, sizeof(struct sockaddr_in));

            // Receiving the ACK packet:
            if(bytes_sent > 0){
                bytes_received = recvfrom(sockfd, &ack_packet, sizeof(ack_packet), 0, (struct sockaddr *)stat_receiver_addr, &s_addr_len);
            }
        }

        // Limitting the DATA sending tries:
        if(tries > MAX_TRIES && ack_packet.flags != ACK){  // Means unsuccessful sending.
            send_seq_number = (send_seq_number == 0) ? 1 : 0;
            return -1; 
        }

        // Updating:
        total_sent += bytes_to_send;
        data_ptr += bytes_to_send;
    }

    return total_sent;
} 

/**
 * Receive data from a peer.
 * Returning -1 if nothing received from sender 
 * Returning -2 if sender sent FIN.
 * Returning -3 for errors.
*/
int rudp_receive(int sockfd, void *buffer, size_t buffer_size, struct sockaddr_in *sender_addr) {

    // Setting timeout to data receiving:
    struct timeval timeout;
    timeout.tv_sec = INCOMMING_DATA_TIMEOUT;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // Getting data from stat_sender. handling: resent SYN, DATA and FIN, inside:
    int got_data = 0;
    ssize_t bytes_received;
    while(got_data == 0){

        // Building the receiving packet:
        char* packet_received[MAX_SEG_SIZE];
        bytes_received = recvfrom(sockfd, packet_received, MAX_SEG_SIZE, 0, (struct sockaddr *)stat_sender_addr, sizeof(struct sockaddr_in));

        // Handling not communicating sender (T.O):
        if(bytes_received < 0){
            stat_sender_addr = NULL;  // Means next time the sender will have to handshake.
            return -1;
        }

        // Splitting the header:
        struct rudp_header* header = (struct rudp_header*) packet_received;
        char* data = packet_received[sizeof(header)];

        // Handling resent SYN:
        if(header->flags == SYN){
            // Creating ACKSYN header:
            struct rudp_header acksyn_header;
            acksyn_header.checksum = 0;
            acksyn_header.length = 0;
            acksyn_header.seq_number = 0;
            acksyn_header.flags = ACKSYN;

            // Sending ACKSYN:
            ssize_t bytes_sent = sendto(sockfd, &acksyn_header, sizeof(acksyn_header), 0, (struct sockaddr *)stat_sender_addr, sizeof(struct sockaddr_in));
            if(bytes_sent < 0){
                return -3;
            } 
            recv_seq_number = 0;
            continue;
        }

        // Handling DATA receiving:
        if(header->flags == DATA){ 

            // Building ACK packet:
            struct rudp_header ack_header;
            ack_header.checksum = 0;
            ack_header.length = 0;
            ack_header.seq_number = 0;
            ack_header.flags = ACK;

            // Sequence number validation:
            if(header->seq_number == recv_seq_number){  // Means already have the packet, sender didnt receive the ACK
                ack_header.seq_number = recv_seq_number;
                size_t bytes_sent = sendto(sockfd, &ack_header, sizeof(ack_header), 0, (struct sockaddr *)stat_sender_addr, sizeof(struct sockaddr_in));
                if(bytes_sent < 0){
                    return -3;
                }
                continue;  //Continue to wait the correct packet in sequence.
            }

            // Length validation:
            if(header->length != bytes_received - sizeof(struct rudp_header)){  // Means the length is incorrect.
                ack_header.flags = NACK; 
            }
            else{
                // Checksum validation:
                int16_t tmp_checksum = 0;
                for (size_t i=0; i<buffer_size; i++){
                    tmp_checksum ^= data[i];
                }
                // Updating NACK for incorrect checksum:
                if(tmp_checksum != header->checksum){
                    ack_header.flags = NACK;
                }
                else{ // Means checksum correct:
                    // Copy data to the buffer:
                    size_t size_to_copy = (buffer_size < header->length) ? buffer_size : header->length;
                    memcpy(data, buffer, size_to_copy);
                    got_data = 1;
                    recv_seq_number = header->seq_number;  // Updaating self seq_number.
                }
            }
            ack_header.seq_number = header->seq_number;  // Updating seq_number of ack packet. 

            // Sending the response packet:
            size_t bytes_sent = sendto(sockfd, &ack_header, sizeof(ack_header), 0, (struct sockaddr *)stat_sender_addr, sizeof(struct sockaddr_in));
            if(bytes_sent < 0){
                return -3;
            }
        }

        // Handling FIN:
        if(header->flags == FIN){
            // Creating ACKFIN header:
            struct rudp_header ackfin_header;
            ackfin_header.checksum = 0;
            ackfin_header.length = 0;
            ackfin_header.seq_number = 0;
            ackfin_header.flags = ACKFIN;

            // Sending ACKFIN:
            size_t bytes_sent = sendto(sockfd, &ackfin_header, sizeof(ackfin_header), 0, (struct sockaddr *)stat_sender_addr, sizeof(struct sockaddr_in));
            if(bytes_sent < 0){
                return -3;
            }
            stat_sender_addr = NULL; 
            recv_seq_number = 0;
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
    if(stat_receiver_addr != NULL){  // Means there was a handshake from the sender:

        // Creating FIN header:
        struct rudp_header fin_header;
        fin_header.length = 0;
        fin_header.checksum = 0;
        fin_header.seq_number = 0;
        fin_header.flags = FIN;

        // Creating ackfin header:
        struct rudp_header ackfin_header;
        ackfin_header.length = 0;
        ackfin_header.checksum = 0;
        ackfin_header.seq_number = 0;
        ackfin_header.flags = 0;

        // Setting timeout:
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = TIMEOUT;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        //Variables:
        int tries = 0;

        while(ackfin_header.flags != ACKFIN && tries < MAX_TRIES){
            // Sending the FIN:
            ssize_t bytes_sent = sendto(sockfd, &fin_header, sizeof(fin_header), 0, (struct sockaddr *)stat_receiver_addr, s_addr_len);
            if(bytes_sent < 0){
                return -3;
            }

            // Wating for ACKFIN:
            ssize_t bytes_received = recvfrom(sockfd, &ackfin_header, sizeof(ackfin_header), 0, (struct sockaddr *)stat_receiver_addr, &s_addr_len);
            if(bytes_received < 0){
                return -3;
            }
            tries++;
        }
        stat_receiver_addr = NULL;
        send_seq_number = 0;  // Init sequence number for more connections to the receiver.

    }

    // Receiver side:
    if(stat_sender_addr != NULL){
        stat_sender_addr = NULL;
        recv_seq_number = 0;
    }

    close(sockfd);
    return 0;
}
