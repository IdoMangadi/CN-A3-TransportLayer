#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "RUDP.h"

#define FILE_SIZE 3 * 1024 *1024

int arguments_error(){
    perror("Error occured while getting arguments");
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
        perror("Error occured while creating socket");
        return 1;
    }

    // Bind to local IP and port and accepting connection:
    int bind_res = rudp_bind(sockfd,"127.0.0.1", port);
    if (bind_res == -1) { 
        perror("Error occured while binding.\n");
        return 1;
    }
    if (bind_res == -2) {
        perror("No connection accepted... closing Receiver.\n");
        return 0;
    }
    if (bind_res == -3) {
        perror("Error occured while handshake.\n");
        return 0;
    }
    printf("Sender connected... waiting for data\nFirst receiving       : ");

    // Creating space in memory to save the measured times:
    double *time_taken_array = NULL;
    size_t num_times = 0;

    // Creating buffer for receiving data:
    char buffer[MAX_SEG_SIZE] = {0};  // Its more than actually get due to the header (created originaly for the RUDP_API).
    char continuing_msg = 'C';


    // Enter a loop of receiving
    while (1) { 

        // Sender address struct for returning value from rudp_recv:
        struct sockaddr_in sender_addr;
        if(continuing_msg == 'C'){
            printf("Receiving DATA from Sender.\n");
        }

        ssize_t total_received = 0;
        int sender_exited = 0;

        // Starting time measuring:
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

        while(total_received < FILE_SIZE){
            ssize_t bytes_received = rudp_receive(sockfd, buffer, sizeof(buffer), &sender_addr);
            if (bytes_received == -1 || bytes_received == -3) {
                printf("Failed to receive data.\n");
                if(time_taken_array != NULL){free(time_taken_array);}
                return 1;
            }
            if(bytes_received == -2){
                printf("Sender end communication.\n\n");
                sender_exited = 1;
                break;
            }
            total_received += bytes_received;
            // ---- Debug print: ----
            // printf("total bytes: %zd ( this time: %zd), first char: %c\n", total_received, bytes_received, buffer[0]);
        }
        if(sender_exited){ break; }
        

        // End of time measuring:
        gettimeofday(&end_time, NULL);

        printf("File transfer completed. File size: %ld Bytes\n", total_received);

        // Times handling: (*1000.0): second -> milliseconds , (/1000.0): microseconds -> milliseconds
        double time_taken = (double)(end_time.tv_sec - start_time.tv_sec)*1000.0 + (double)(end_time.tv_usec - start_time.tv_usec) / 1000.0;

        num_times++;
        time_taken_array = realloc(time_taken_array, num_times * sizeof(double));
        if (time_taken_array == NULL) {
            perror("Error occured while reallocating memory.\n");
            if(time_taken_array != NULL){free(time_taken_array);}
            return 1;
        }
        time_taken_array[num_times - 1] = time_taken;  // Saving the time taken to receive the current file.

        // Receiving continuing message:
        ssize_t bytes_received = rudp_receive(sockfd, buffer, sizeof(buffer), &sender_addr);
        if (bytes_received == -1 || bytes_received == -3) {
            printf("Failed to receive data.\n");
            if(time_taken_array != NULL){free(time_taken_array);}
            return 1;
        }
        if(bytes_received == -2){  // Not suppose to happend.
            printf("Sender end communication.\n\n");
            break;
        }
        continuing_msg = buffer[0];
        printf("Continuing message: %c : ", continuing_msg);
        if(continuing_msg == 'E'){
            printf("Sender sent exit message.\n");  // The next msg from sender will be FIN 
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
        double speed = ((FILE_SIZE)/(1024*1024)) / (time_taken_array[i] / 1000.0);  // Converting to seconds
        speeds_sum += speed;
        printf("- Run #%d:    File Size: %.*f MB;    Time: %.*f ms;    Speed: %.*f MB/s\n", i, 2, (FILE_SIZE)/(1024.0*1024.0), 2, time_taken_array[i], 2, speed);
        times_sum += time_taken_array[i];
    }

    printf("- Average time: %.*f ms\n", 2, times_sum/num_times);
    printf("- Average bandwidth: %.*f MB/s\n", 2, speeds_sum/num_times);
    printf("----------------------------------------------------------------------------------------\n");
    printf("\nReceiver ends.\n");

    free(time_taken_array);

    return 0;
}
 
