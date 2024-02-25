#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/tcp.h>

#define BUFFER_SIZE 3 * 1024 * 1024

int arguments_error(){
    perror("Error occured whlie getting arguments");
    return 1;
}

int main(int argc, char* argv[]){

    // Handling arguments:
    if(argc != 5){
        return arguments_error();
    }
    // PORT:
    int port = 9998;
    if(strcmp("-p", argv[1]) == 0){
        port = atoi(argv[2]);
    }
    else {return arguments_error();}
    // CC Algorithm:
    char* algorithm = NULL;
    if(strcmp("-algo", argv[3]) == 0){
        algorithm = argv[4];
    }
    else {return arguments_error();}

    printf("Starting receiver...\n");

    int sock = -1;  // Holding the socket file descriptor
    struct sockaddr_in server;  // Holding the server details
    struct sockaddr_in client;  // Holding the client details
    socklen_t client_len = sizeof(client);
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));

    sock = socket(AF_INET, SOCK_STREAM, 0); // Socket creation
    // Socket creation check:
    if ( sock == -1){
        perror("Error occured whlie creating socket");
        return 1;
    }

    // Asking the OS to give the socket IP (Maybe will change later!)
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Defining CC algorithm:
    if(setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, algorithm, strlen(algorithm)) < 0){
        perror("Error occured whlie defining CC algorithm");
        return 1;
    }

    // Binding:
    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) < 0){
        perror("Error occured whlie binding");
        return 1;
    }
    // Listening:
    if (listen(sock, 1) < 0){
        perror("Error occured whlie listening");
        return 1;
    }

    printf("Waiting for TCP connection...\n");

    // Creating space in memory to sava the measured times:
    double *time_taken_array = NULL;
    size_t num_times = 0;

    // File handling:
    int file_fd = open("received_file.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(file_fd < 0){
        perror("Error occured whlie opening file");
        return 1;
    }

    // Accepting a TCP connection:
    int client_sock = accept(sock, (struct sockaddr *)&client, &client_len);
    if (client_sock < 0){
        perror("Error occured whlie accepting");
        return 1;
    }
    fprintf(stdout, "Client %s:%d connected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    
    // Reading file from the sender:
    while(1){

        char buffer[(BUFFER_SIZE)] = {0};

        printf("Starting to receive file from Sender\n");

        // Starting time measuring:
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

        // The receiving part:
        size_t bytes_received = recv(client_sock, buffer, (BUFFER_SIZE), 0);
        if( bytes_received < 0){
            perror("Error occured whlie reading");
            close(file_fd);
            break;
        }
        if(buffer[0] == 'E'){
            printf("Sender sent exit message.\n");
            close(file_fd);
            break;
        }
        write(file_fd, buffer, (BUFFER_SIZE));

        // End of measuring:
        gettimeofday(&end_time, NULL);

        printf("File transfer completed.\n");

        // Times handling: (*1000.0): second -> milliseconds , (/1000.0): microseconds -> milliseconds
        double time_taken = (double)(end_time.tv_sec - start_time.tv_sec)*1000.0 + (double)(end_time.tv_usec - start_time.tv_usec) / 1000.0;

        num_times++;
        time_taken_array = realloc(time_taken_array, num_times * sizeof(double));
        if (time_taken_array == NULL) {
            perror("Error occured while reallocating memory");
            return 1;
        }
        time_taken_array[num_times - 1] = time_taken;  // Saving the time taken to receive the current file.
    }
    close(client_sock);
    close(sock);

    // Printing:
    printf("----------------------------\n");
    printf("-      *Statistics*        -\n");
    double times_sum = 0;
    double speeds_sum = 0;
    for (int i=0; i<num_times; i++){
        double speed = (3.0 * 1024 * 1024) / (time_taken_array[i] / 1000.0);  // Converting to seconds
        speeds_sum += speed;
        printf("- Run #%d:  Time=%fms; Speed=%f MB/s\n", i, time_taken_array[i], speed);
        times_sum += time_taken_array[i];
    }

    printf("- CC Algorithm: %s\n", algorithm);
    printf("- Average time: %fms\n", times_sum/num_times);
    printf("- Average bandwidth: %fMB/s\n", speeds_sum/num_times);
    printf("----------------------------\n");
    printf("Receiver end.\n");

    free(time_taken_array);

    return 0;
}

