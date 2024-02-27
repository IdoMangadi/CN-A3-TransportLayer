#ifndef RUDP_H
#define RUDP_H

#include <stdint.h>
#include <netinet/in.h>

#define MAX_TRIES 3
#define TIMEOUT 3000 // 3000 milliseconds
#define INCOMMING_CONNECTIONS_TIMEOUT 60 // In seconds
#define INCOMMING_DATA_TIMEOUT 60 // In seconds
#define BUFFER_SIZE 3 * 1024 * 1024

// RUDP header flags
#define SYN 0x01
#define ACK 0x02
#define DATA 0x04
#define FIN 0x08
#define ACKSYN (SYN | ACK)
#define ACKFIN (FIN | ACK)
#define NACK 0x10

extern struct sockaddr_in *s_receiver_addr;
extern struct sockaddr_in *s_sender_addr;

int rudp_socket();
int rudp_connect(int sockfd, const char *receiver_ip, int receiver_port);
int rudp_bind(int sockfd, int local_port);
int rudp_send(int sockfd, char *data, size_t len, struct sockaddr_in *receiver_addr);
int rudp_receive(int sockfd, void *buffer, size_t buffer_size, struct sockaddr_in *sender_addr);
int rudp_close(int sockfd);

#endif /* RUDP_H */
