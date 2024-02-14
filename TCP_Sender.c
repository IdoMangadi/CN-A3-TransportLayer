#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define SERVER_PORT 9998

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


int main(){
    unsigned int size = 3 * 1024 * 1024;
    char *random_data = util_generate_random_data(size);

    if (random_data != NULL){

        //Creating the socket:
        int sock = -1;
        struct  sockaddr_in server;
        memset(&server, 0, sizeof(server));

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if ( sock == -1){
            perror("Error occured whlie creating socket");
            return 1;
        }

        if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) <= 0){
            perror("Error - invalid address / addres not supported");
            return 1;
        }

        server.sin_family = AF_INET;
        server.sin_port = htons(SERVER_PORT);

        // Connecting to the Receiver socket:
        if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0){
            perror("Connection failed");
            return 1;
        }

        
        int user_choice = 1;
        while(user_choice){
            // Sending the file:
            if (send(sock, random_data, size, 0) < 0){
                perror("Send failed");
                return 1;
            }
            // Asking the user about sending again:
            do{
                printf("Do you want to send the file again?\nYes -> Enter 1\nNo -> Enter 0\nYour answer: ");
            } while(scanf(" %d", &user_choice) != 1);
        }

        char exit_message = 'E';
        if (send(sock, &exit_message, sizeof(exit_message), 0) < 0){
                perror("Send failed");
                return 1;
        }
        close(sock);
        free(random_data);

    }

    return 0;
}