#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
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

void handle_server(int fd) {
    char buf[4096] = {0};
    proto_hdr_t *hdr = (proto_hdr_t*)buf;
    read(fd, buf, sizeof(proto_hdr_t) + sizeof(int));
    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    int *data = (int*)&hdr[1];
    *data = ntohl(*data);

    if (*data != 1) {
        printf("Protocol mismatch!\n");
        return;
    }

    printf("Successfully connected to the server, protocol v1.\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <ip of the host>\n", argv[0]);
        return 0;
    }

    struct sockaddr_in serverInfo = {0};
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = inet_addr(argv[1]);
    serverInfo.sin_port = htons(9090);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("socket");
        return 0;
    }

    if (connect(serverSocket, (struct sockaddr*)&serverInfo, sizeof(serverInfo)) == -1) {
        perror("connect");
        close(serverSocket);
        return 0;
    }

    handle_server(serverSocket);

    close(serverSocket);
}
