#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>

#define MAX_CLIENTS 100

int clients[MAX_CLIENTS];
pthread_mutex_t client_mutex;

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[1024];

    // Add client to client list
    pthread_mutex_lock(&client_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == -1) {
            clients[i] = client_fd;
            break;
        }
    }

    pthread_mutex_unlock(&client_mutex);

    while (1) {

        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received == -1) {
            perror("recv");
            break;
        }

        if (bytes_received == 0) {
            printf("Client disconnected.\n");
            break;
        }

        buffer[bytes_received] = '\0';

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
        if (clients[i] != -1 && clients[i] != client_fd)
        {
        send(clients[i], buffer, bytes_received, 0);
        }
        
        }

        printf("Client %d: %s\n", client_fd, buffer);
    }

    // Remove client from client list
    pthread_mutex_lock(&client_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == client_fd) {
            clients[i] = -1;
            break;
        }
    }

    pthread_mutex_unlock(&client_mutex);

    close(client_fd);
    return NULL;
}

int main(void) {

    pthread_mutex_init(&client_mutex, NULL);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = -1;
    }

    printf("TCP server is starting...\n");

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    printf("Socket Created successfully\n");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    printf("Socket bound successfully\n");

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        return 1;
    }

    while (1) {

        printf("Waiting for a client...\n");

        int client_fd = accept(server_fd, NULL, NULL);

        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        printf("Client connection successful!\n");

        int *client = malloc(sizeof(int));
        if (client == NULL) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        *client = client_fd;

        pthread_t thread;

        if (pthread_create(&thread, NULL, handle_client, client) != 0) {
            perror("pthread_create");
            free(client);
            close(client_fd);
            continue;
        }

        pthread_detach(thread);
    }

    close(server_fd);
    pthread_mutex_destroy(&client_mutex);

    return 0;
}