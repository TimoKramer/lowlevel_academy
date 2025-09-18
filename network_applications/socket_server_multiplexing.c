#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>

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

int find_slot_by_fd(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientStates[i].fd == fd) {
            return i;
        }
    }
    return -1;  // not found
}

int main() {
    int listen_fd, conn_fd, freeSlot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    int opt = 1;

    init_clients();

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while (1) {

        int ii = 1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientStates[i].fd != -1) {
                fds[ii].fd = clientStates[i].fd;
                fds[ii].events = POLLIN;
                ii++;
            }
        }

        // wait for an event on one of the sockets
        int n_events = poll(fds, nfds, -1);
        if (n_events == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        // check for new connections
        if (fds[0].revents & POLLIN) {
            if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
                perror("accept");
                continue;
            }

            printf("New connection from %s:%d\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            freeSlot = find_free_slot();
            if (freeSlot == -1) {
                printf("Server full: closing new connection\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
                clientStates[freeSlot].state = STATE_CONNECTED;
                nfds++;
                printf("Slot %d has fd %d\n", freeSlot, clientStates[freeSlot].fd);
            }

            n_events--;
        }

        // check each client for read/write activity
        for (int i = 1; i <= nfds && n_events > 0; i++) { // start from 1 to skip the listen_fd
            if (fds[i].revents & POLLIN) {
                n_events--;

                int fd = fds[i].fd;
                int slot = find_slot_by_fd(fd);
                ssize_t bytes_read = read(fd, clientStates[slot].buffer, sizeof(clientStates[slot].buffer));
                if (bytes_read <= 0) {
                    // connection closed or error
                    close(fd);
                    if (slot == -1) {
                        printf("Tried to close fd that doesn't exist?\n");
                    } else {
                        clientStates[slot].fd = -1; // free up the slot
                        clientStates[slot].state = STATE_DISCONNECTED;
                        printf("Client disconnected or error\n");
                        nfds--;
                    }
                } else {
                    printf("Received data from client: %s\n", clientStates[slot].buffer);
                }
            }
        }
    }

    return 0;
}
