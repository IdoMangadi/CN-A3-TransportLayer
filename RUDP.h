#ifndef RUDP_H
#define RUDP_H

#include <stdint.h>
#include <netinet/in.h>

#define MAX_TRIES 5
#define TIMEOUT 50000 // in microseconds, => 50 in miliseconds => 0.05 in seconds
#define INCOMMING_CONNECTIONS_TIMEOUT 90 // In seconds
#define INCOMMING_DATA_TIMEOUT 90 // In seconds
#define MAX_SEG_SIZE 65482  // The *UDP* maximum segment size.


// RUDP header flags
#define SYN 0x01
#define ACK 0x02
#define DATA 0x04
#define FIN 0x08
#define ACKSYN (SYN | ACK)
#define ACKFIN (FIN | ACK)
#define NACK 0x10

// extern struct sockaddr_in *s_receiver_addr;

// extern struct sockaddr_in *s_sender_addr;

/**
 * Creating a RUDP socket.
 * return: socket fd.
*/
int rudp_socket();

/**
 * Establishes connection with a specified receiver.
 * Returns: 0 for success, -1 for error, -2 for receiver not respond.
 */
int rudp_connect(int sockfd, const char *receiver_ip, int receiver_port);

/**
 * Binds the socket to the a local IP and given port and wait for connection.
 * Returns 0 ofor success, -1 for error, -2 for no incomming connections and -3 for handshake failure.
 */
int rudp_bind(int sockfd, const char *local_ip, int port);

/**
 * Sending data to the connected peer.
 * returns bytes sent or -1 for error.
*/
int rudp_send(int sockfd, char *data, size_t len);

/**
 * Receive data from a peer.
 * Returns byte received or -1 for nothing received from sender, -2 for sender finished communication, -3 for errors. 
*/
int rudp_receive(int sockfd, void *buffer, size_t buffer_size, struct sockaddr_in *sender_addr);

/**
 * Closes a connections between peers.
 * returns 0 for success, -1 for errors.
*/
int rudp_close(int sockfd);

#endif /* RUDP_H */
