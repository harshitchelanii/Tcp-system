#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>

void *handle_client(void *arg) {
  int client_fd = *(int *)arg;
  free(arg);
  char buffer[1024];
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
    printf("Client %d: %s\n", client_fd, buffer);
  }
  close(client_fd);
  return NULL;
}

int main(void) {
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

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
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
    *client = client_fd;

    pthread_t thread;
    pthread_create(&thread, NULL, handle_client, client);
    {
    }
    pthread_detach(thread);
  }

  close(server_fd);
  return 0;
}