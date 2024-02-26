#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <netinet/tcp.h>

#define DATA_SIZE 3 * 1024 * 1024

/**
 * This function generating a file.
 * param size := unsigned int representing the requested size in bytes.
*/
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
    perror("Error occured whlie getting arguments");
    return 1;
}


int main(int argc, char* argv[]){

    // Handling arguments:
    if(argc != 7){
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
    // CC Algorithm:
    char* algorithm = NULL;
    if(strcmp("-algo", argv[5]) == 0){
        algorithm = argv[6];
    }
    else {return arguments_error();}


    // Generating the file to send:
    unsigned int size = DATA_SIZE;
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

        // Defining CC algorithm:
        if(setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, algorithm, strlen(algorithm)) < 0){
            perror("Error occured whlie defining CC algorithm");
            return 1;
        }

        if (inet_pton(AF_INET, ip, &server.sin_addr) <= 0){
            perror("Error - invalid address / addres not supported");
            return 1;
        }

        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        // Connecting to the Receiver socket:
        if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0){
            perror("Connection failed");
            return 1;
        }

        // Sending the file for the first time:
        if (send(sock, random_data, size, 0) < 0){
            perror("Send failed");
            return 1;
        }

        int user_choice = 1;
        
        do{
            printf("Do you want to send the file again?\nYes -> Enter 1\nNo -> Enter 0\nYour answer: ");
            scanf(" %d", &user_choice);
            if(user_choice == 1){
                // Sending continuing message:
                char continue_message = 'C';
                if (send(sock, &continue_message, sizeof(continue_message), 0) < 0){
                    perror("Send failed");
                    return 1;
                }
                //Sending the file again:
                if (send(sock, random_data, size, 0) < 0){
                    perror("Send failed");
                    return 1;
                }
            }
        } while(user_choice != 0);

        // Sending an exit message:
        char exit_message = 'E';
        if (send(sock, &exit_message, sizeof(exit_message), 0) < 0){
            perror("Send failed");
            return 1;
        }
        // Closing the socket:
        close(sock);
        free(random_data);
    }

    return 0;
}