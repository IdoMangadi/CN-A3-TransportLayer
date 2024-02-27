#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "RUDP.h"

#define RECEIVER_IP "127.0.0.1"
#define RECEIVER_PORT 12345

int main() {
    // Create a socket
    int sockfd = rudp_socket();

    // Connect to the receiver
    if (rudp_connect(sockfd, RECEIVER_IP, RECEIVER_PORT) == -1) {
        fprintf(stderr, "Failed to connect to receiver\n");
        exit(EXIT_FAILURE);
    }

    printf("UDP Sender started.\n");

    // Send data to the receiver
    int bytes_sent = rudp_send(sockfd, "Hello, receiver!", strlen("Hello, receiver!"), s_receiver_addr);
    if (bytes_sent == -1) {
        fprintf(stderr, "Failed to send data\n");
        exit(EXIT_FAILURE);
    }

    printf("Sent %d bytes of data to receiver\n", bytes_sent);

    // Close the socket
    rudp_close(sockfd);

    return 0;
}
