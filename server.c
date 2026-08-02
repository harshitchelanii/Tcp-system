#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>

#define MAX_CLIENTS 100


struct Client
{
 int socket;
 char username[50];
};

struct Client clients[MAX_CLIENTS];
pthread_mutex_t client_mutex;

void broadcast_message(const char *message, int sender_socket) {
    pthread_mutex_lock(&client_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i].socket != -1 &&
            clients[i].socket != sender_socket)
        {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }

    pthread_mutex_unlock(&client_mutex);
}


void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);


    int client_index = -1;
    char buffer[1024];
    char final_message[1200];

    pthread_mutex_lock(&client_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == -1) {
            clients[i].socket = client_fd;
            client_index = i;
            break;
        }
    }

    pthread_mutex_unlock(&client_mutex);

    if (client_index == -1) {
        // Server full, no free slot for this client
        printf("Server full, rejecting connection\n");
        close(client_fd);
        return NULL;
    }

    char username[50];
    int bytes_received = recv(client_fd, username, sizeof(username) - 1, 0);

    if (bytes_received <= 0) {
        pthread_mutex_lock(&client_mutex);
        clients[client_index].socket = -1;
        pthread_mutex_unlock(&client_mutex);
        close(client_fd);
        return NULL;
    }
    
    username[bytes_received] = '\0';
    pthread_mutex_lock(&client_mutex);

    strncpy(clients[client_index].username, username, sizeof(clients[client_index].username) - 1);
    clients[client_index].username[sizeof(clients[client_index].username) - 1] = '\0';
    
    char system_message[200];
    
    snprintf(
    system_message,
    sizeof(system_message),
    "[WAYP] %s joined the chat.\n",
    clients[client_index].username
    );

    printf("%s", system_message);

    pthread_mutex_unlock(&client_mutex);

    broadcast_message(system_message, client_fd);
    printf("Client registered: %s\n", clients[client_index].username);
    while (1) {

        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received == -1) {
            perror("recv");
            break;
        }

        if (bytes_received == 0) {
            break;
        }

        buffer[bytes_received] = '\0';


        snprintf(
            final_message,
            sizeof(final_message),
            "%s: %s",
            clients[client_index].username,
            buffer
        );

        broadcast_message(final_message, client_fd);


        printf("%s\n", final_message);
    }

    // Notify everyone else that this client left, however the loop ended
    snprintf(
        system_message,
        sizeof(system_message),
        "[WAYP] %s left the chat.\n",
        clients[client_index].username
    );

    broadcast_message(system_message, client_fd);

    printf("%s\n", system_message);

    // Remove client from client list
    pthread_mutex_lock(&client_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_fd) {
            clients[i].socket = -1;
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
        clients[i].socket = -1;
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