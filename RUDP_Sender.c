#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include "RUDP.h"

#define FILE_SIZE 3 * 1024 *1024

char *util_generate_random_data(unsigned int size){
    char *buffer = NULL;

    //Argument check:
    if (size == 0){
        return NULL;
    }

    buffer = (char *)calloc(size, sizeof(char));

    //Error checking:
    if (buffer == NULL){
        return NULL;
    }

    //Randomize the seed of the random number generator.
    srand(time(NULL));

    for(unsigned int i=0; i<size; i++){
        *(buffer + i) = ((unsigned int)rand() % 256);
    }

    return buffer;
}


int arguments_error(){
    perror("Error occurred while getting arguments");
    return 1;
}

int main(int argc, char* argv[]) {

    // Handling arguments:
    if(argc != 5){
        return arguments_error();
    }
    // IP:
    char* ip = NULL;
    if(strcmp("-ip", argv[1]) == 0){
        ip = argv[2];
    }
    else {return arguments_error();}
    // PORT:
    int port = 9998;
    if(strcmp("-p", argv[3]) == 0){
        port = atoi(argv[4]);
    }
    else {return arguments_error();}


    // Generating the file to send:
    unsigned int size = FILE_SIZE;
    char *random_data = util_generate_random_data(size);

    if (random_data != NULL){

        // Create a socket
        int sockfd = rudp_socket();
        if(sockfd < 0){
            perror("Error occurred while creating socket\n");
            return 1;
        }

        printf("RUDP Sender started.\n Connecting Receiver...\n");

        // Connect to the receiver
        int con_res = rudp_connect(sockfd, ip, port);
        if ( con_res == -1) {
            perror("Error occurred while connecting\n");
            return 1;
        }
        if ( con_res == -2) {
            perror("Receiver not responding.\n");
            return 0;
        }
        
        printf("Connection established.\n");

        // Sending the file for the first time:
        if(rudp_send(sockfd, random_data, FILE_SIZE) < 0){
            perror("Error occurred while sending random data.\n");
            return 1;
        }

        int user_choice = 1;
        do{
            printf("Do you want to send the file again?\nYes -> Enter 1\nNo -> Enter 0\nYour answer: ");
            scanf(" %d", &user_choice);
            if(user_choice == 1){
                // Sending continuing message:
                char continue_message = 'C';
                if (rudp_send(sockfd, &continue_message, sizeof(continue_message)) < 0){
                    perror("Error occurred while sending message to receiver.\n");
                    return 1;
                }
                // Sending the file:
                if(rudp_send(sockfd, random_data, FILE_SIZE) < 0){
                    perror("Error occurred while sending random data.\n");
                    return 1;
                }
            }
        }while(user_choice != 0);
        
        // Sending exit message:
        char ending_message = 'E';
        if(rudp_send(sockfd, &ending_message, sizeof(ending_message)) < 0){
            perror("Error occurred while sending random data.\n");
            return 1;
        }

        // Closing the socket
        rudp_close(sockfd);
    
    }
    else{
        perror("Error Creating random data.\n");
        return 1;
    }

    return 0;
}
