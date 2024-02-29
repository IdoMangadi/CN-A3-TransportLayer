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

int rudp_socket();

int rudp_connect(int sockfd, const char *receiver_ip, int receiver_port);

int rudp_bind(int sockfd, int local_port);

int rudp_send(int sockfd, char *data, size_t len);

int rudp_receive(int sockfd, void *buffer, size_t buffer_size, struct sockaddr_in *sender_addr);

int rudp_close(int sockfd);

#endif /* RUDP_H */
