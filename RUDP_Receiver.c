#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "RUDP.h"

int arguments_error(){
    perror("Error occured whlie getting arguments");
    return 1;
}

int main(int argc, char* argv[]) {

    // Handling arguments:
    if(argc != 3){
        return arguments_error();
    }
    // PORT:
    int port = 9998;
    if(strcmp("-p", argv[1]) == 0){
        port = atoi(argv[2]);
    }
    else {return arguments_error();}

    // Create a socket:
    int sockfd = rudp_socket();
    if(sockfd < 0){
        perror("Error occured whlie creating socket");
        return 1;
    }

    // Bind to local IP and port and accepting connection:
    int bind_res = rudp_bind(sockfd, port);
    if (bind_res == -1) {
        perror("Error occured whlie binding.\n");
        return 1;
    }
    if (bind_res == -2) {
        perror("No connection accepted... closing Receiver.\n");
        return 0;
    }
    if (bind_res == -3) {
        perror("Error occured whlie handshake.\n");
        return 0;
    }
    printf("Sender connected... waiting for data\n");

    // Creating space in memory to save the measured times:
    double *time_taken_array = NULL;
    size_t num_times = 0;


    // Enter a loop of receiving
    while (1) {

        // Creating buffer for receiving data:
        char buffer[BUFFER_SIZE];

        // Sender address struct for returning value from rudp_recv:
        struct sockaddr_in sender_addr;

        printf("Receiving MSG from Sender.\n");

        // Starting time measuring:
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

        ssize_t bytes_received = rudp_receive(sockfd, buffer, sizeof(buffer), &sender_addr);
        if (bytes_received == -1) {
            fprintf(stderr, "Failed to receive data.\n");
            break;
        }
        if (bytes_received == -2) {
            fprintf(stderr, "Sender end connection.\n");  // Means the sender sent a FIN 
            break;
        }

        // End of time measuring:
        gettimeofday(&end_time, NULL);

        // Times handling: (*1000.0): second -> milliseconds , (/1000.0): microseconds -> milliseconds
        double time_taken = (double)(end_time.tv_sec - start_time.tv_sec)*1000.0 + (double)(end_time.tv_usec - start_time.tv_usec) / 1000.0;

        num_times++;
        time_taken_array = realloc(time_taken_array, num_times * sizeof(double));
        if (time_taken_array == NULL) {
            perror("Error occured while reallocating memory.\n");
            return 1;
        }
        time_taken_array[num_times - 1] = time_taken;  // Saving the time taken to receive the current file.

        bytes_received = rudp_receive(sockfd, buffer, 1, &sender_addr);
        if(buffer[0] == 'E'){
            print("Sender sent exit message.\n");  // The next msg from sender will be FIN 
        }
    }

    // Close the socket:
    rudp_close(sockfd);

    // Printing:
    printf("----------------------------------------------------------------------------------------\n");
    printf("-                                   *Statistics*                                       -\n");
    double times_sum = 0;
    double speeds_sum = 0;
    for (int i=0; i<num_times; i++){
        double speed = (3.0 * 1024 * 1024) / (time_taken_array[i] / 1000.0);  // Converting to seconds
        speeds_sum += speed;
        printf("- Run #%d:    File Size: %d MB;    Time: %.*f ms;    Speed: %.*f MB/s\n", i, (BUFFER_SIZE)/(1024*1024), 2, time_taken_array[i], 2, speed);
        times_sum += time_taken_array[i];
    }

    printf("- Average time: %.*f ms\n", 2, times_sum/num_times);
    printf("- Average bandwidth: %.*f MB/s\n", 2, speeds_sum/num_times);
    printf("----------------------------------------------------------------------------------------\n");
    printf("Receiver end.\n");

    free(time_taken_array);

    return 0;
}
 
