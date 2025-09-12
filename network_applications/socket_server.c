#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 9090
#define BACKLOG 0

typedef enum {
    PROTO_HELLO,
} proto_type_e;

typedef struct {
    proto_type_e type;
    unsigned short len;
} proto_hdr_t;

void handle_client(int fd) {
    char buf[4096] = {0};
    proto_hdr_t *hdr = (proto_hdr_t *)buf;
    hdr->type = htonl(PROTO_HELLO);
    hdr->len = sizeof(int);
    int reallen = hdr->len;
    hdr->len = htons(hdr->len);

    int *data = (int*)&hdr[1];
    *data = htonl(1);

    write(fd, hdr, sizeof(proto_hdr_t) + reallen);
}

int main() {
    struct sockaddr_in serverInfo = {0};
    struct sockaddr_in clientInfo = {0};
    socklen_t clientSize = sizeof(clientInfo);

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(PORT);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (bind(fd, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) == -1) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, BACKLOG) == -1) {
        perror("listen");
        close(fd);
        return -1;
    }

    while (1) {
        clientSize = sizeof(clientInfo);
        int cfd = accept(fd, (struct sockaddr *)&clientInfo, &clientSize);
        if (cfd == -1) {
            perror("accept");
            close(fd);
            return -1;
        }

        handle_client(cfd);

        close(cfd);
    }
}
