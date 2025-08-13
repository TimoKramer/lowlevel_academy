#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 10
#define MAX_CLIENTS 256
#define BUFF_SIZE 4096

typedef enum {
    STATE_NEW,
    STATE_CONNECTED,
    STATE_DISCONNECTED
} state_e;

typedef struct {
    int fd;
    state_e state;
    char buffer[4096];
} clientstate_t;

typedef enum {
    PROTO_HELLO,
} proto_type_e;

typedef struct {
    proto_type_e type;
    unsigned short len;
    unsigned short string_len;
} proto_hdr_t;

clientstate_t clientStates[MAX_CLIENTS];

void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clientStates[i].fd = -1; // indicating free slot
        clientStates[i].state = STATE_NEW;
        memset(&clientStates[i].buffer, '\0', BUFF_SIZE);
    }
}

int find_free_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientStates[i].fd == -1) {
            return i;
        }
    }
    return -1; // no free slot found
}

void handle_client(int fd) {
    char buf[4096] = {0};
    proto_hdr_t *hdr = (proto_hdr_t *)buf;

    const char *response = "Hello World!";
    int response_len = strlen(response);

    hdr->type = htonl(PROTO_HELLO);
    hdr->len = sizeof(int);
    hdr->string_len = htons(response_len);
    int reallen = hdr->len;
    hdr->len = htons(hdr->len);

    // add protocol version after header
    int *data = (int*)&hdr[1];
    *data = htonl(1);

    // add payload after protocol version
    char *string_data = (char*)&data[1];
    memcpy(string_data, response, response_len);

    ssize_t total_size = sizeof(proto_hdr_t) + sizeof(int) + response_len;

    ssize_t bytes_written = write(fd, hdr, total_size);
    if (bytes_written == -1) {
        perror("write");
    } else if (bytes_written != total_size) {
        printf("Partial write (%zd of %zd bytes)\n", bytes_written, total_size);
    }
}

int main() {
    int listen_fd, conn_fd, nfds, freeSlot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    fd_set read_fds, write_fds;

    init_clients();

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        FD_SET(listen_fd, &read_fds);
        nfds = listen_fd + 1;

        // add active connections to the read set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientStates[i].fd != -1) {
                FD_SET(clientStates[i].fd, &read_fds);
                if (clientStates[i].fd >= nfds) nfds = clientStates[i].fd + 1;
            }
        }

        // wait for an activity on one of the sockets
        if (select(nfds, &read_fds, &write_fds, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // check for new connections
        if (FD_ISSET(listen_fd, &read_fds)) {
            if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
                perror("accept");
                continue;
            }

            printf("New connection from: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // find a free slot for the new connection
            freeSlot = find_free_slot();
            if (freeSlot == -1) {
                printf("Server full: closing new connection\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
                clientStates[freeSlot].state = STATE_CONNECTED;
            }
        }


        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientStates[i].fd != -1 &&
                FD_ISSET(clientStates[i].fd, &read_fds)) {
                ssize_t  bytes_read = read(clientStates[i].fd, &clientStates[i].buffer, sizeof(clientStates[i].buffer));

                if (bytes_read <= 0) {
                    close(clientStates[i].fd);
                    clientStates[i].fd = -1;
                    clientStates[i].state = STATE_DISCONNECTED;
                    printf("Client disconnnected or error\n");
                } else {
                    printf("Received data from client: %s\n", clientStates[i].buffer);
                }
            }
        }
    }

    return 0;
}
