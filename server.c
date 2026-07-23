#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main(void){
    printf("TCP server is starting...\n");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }
    printf("Socket Created successsfully\n");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            perror("bind");
            return 1;
        }
    printf("socket bound sucessfully\n");
    if(listen(server_fd, 5) == -1){
    perror("Listen fail");
    exit(EXIT_FAILURE);
    }
    printf("Waiting for a client...\n");
    int client_fd = accept(server_fd,NULL, NULL);
    if(client_fd == -1){
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    else{
        printf("Client connection sucessfull!");
    }

    return 0;
    }