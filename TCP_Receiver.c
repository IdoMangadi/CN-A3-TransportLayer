#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>


#define SERVER_PORT 9998
#define BUFFER_SIZE 3 * 1024 * 1024

int main(){

    printf("Starting receiver...");

    int sock = -1;  // Holding the socket file descriptor
    struct sockaddr_in server;  // Holding the server details
    struct sockaddr_in client;  // Holding the client details
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
    server.sin_port = htons(SERVER_PORT);

    if (bind(sock, (struct sock_addr*)&server, sizeof(server)) < 0){
        perror("Error occured whlie binding");
        return 1;
    }

    if (listen(sock, 1) < 0){
        perror("Error occured whlie listening");
        return 1;
    }

    printf("Waiting TCP connection...");

    double *time_taken_array = NULL;
    size_t num_times = 0;

    // Accepting a TCP connection:
    int client_sock = accept(sock, (struct sockaddr *)&client, (socklen_t*)&client);
    if (client_sock < 0){
        perror("Error occured whlie accepting");
        return 1;
    }
    fprintf(stdout, "Client %s:%d connected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    // Reading file from the sender:
    while(1){

        char buffer[BUFFER_SIZE] = {0};

        // Starting time measuring:
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

        printf("Starting to receive file from Sender");

        int read_size = read(client_sock, buffer, 1024);

        // End of measuring:
        gettimeofday(&end_time, NULL);

        if (read_size < 0){
            perror("Error occured whlie reading from client");
            return 1;
        }
        else if (read_size == 0){  // Means user disconnect
            fprintf(stdout, "Client %s:%d disconnected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            close(client_sock);
            break;
        }
        printf("File transfer completed.");

        if(buffer[0] == 'E'){
            printf("Sender sent exit message.");
            close(client_sock);
            break;
        }

        double time_taken = (double)(end_time.tv_sec - start_time.tv_sec)*1000.0 + (double)(end_time.tv_usec - start_time.tv_usec) / 1000.0;

        // Handling the memory issues:
        num_times++;
        time_taken_array = realloc(time_taken_array, num_times * sizeof(double));
        if (time_taken_array == NULL) {
            perror("Error occured while reallocating memory");
            return 1;
        }
        time_taken_array[num_times - 1] = time_taken;

    }

    // Printing:
    printf("----------------------------");
    printf("-      *Statistics*         ");
    double times_sum = 0;
    double speeds_sum = 0;
    for (int i=0; i<num_times; i++){
        double speed = (3*1024*1024) * time_taken_array[i];
        speeds_sum += speed;
        printf("- Run #%d:  Time=%fms; Speed=%f MB/s", i, time_taken_array[i], speed);
        times_sum += time_taken_array[i];
    }

    printf("- Average time: %fms", times_sum/num_times);
    printf("- Average bandwidth: %fMB/s", speeds_sum/num_times);
    printf("----------------------------");
    printf("Receiver end.");

    free(time_taken_array);

    return 0;
}

