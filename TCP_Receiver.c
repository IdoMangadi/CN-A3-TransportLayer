#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#define SERVER_PORT 9998
#define BUFFER_SIZE 1024

int main(){
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

    while (1)
    {
        int client_sock = accept(sock, (struct sockaddr *)&client, (socklen_t*)&client);
        if (client_sock < 0 ){
            perror("Error occured whlie accepting");
            return 1;
        }
        fprintf(stdout, "Client %s:%d connected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        char buffer[BUFFER_SIZE] = {0};
        int read_size = read(client_sock, buffer, 1024);
        if (read_size < 0){
            perror("Error occured whlie reading from client");
            return 1;
        }
        else if (read_size == 0){
            fprintf(stdout, "Client %s:%d disconnected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            close(client_sock);
            continue;
        }
        else{
            fprintf(stdout, "Client %s:%d says:%s\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port), buffer);
            close(client_sock);
        }



    }
    
    


    


    return 0;
}

