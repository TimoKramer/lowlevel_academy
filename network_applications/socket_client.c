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
    PROTO_VERSION_1,
} proto_type_e;

typedef struct {
    proto_type_e type;
    unsigned short len;
    unsigned short string_len;
} proto_hdr_t;

void handle_server_response(int fd) {
    char buf[4096] = {0};

    ssize_t bytes_read = read(fd, buf, sizeof(proto_hdr_t));
    if (bytes_read != sizeof(proto_hdr_t)) {
        perror("read header");
        return;
    }

    proto_hdr_t *hdr = (proto_hdr_t*)buf;
    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);
    hdr->string_len = ntohs(hdr->string_len);

    printf("Received header - type: %d, len: %d, string_len: %d\n",
       hdr->type, hdr->len, hdr->string_len);

    // read protocol version
    int version_data;
    bytes_read = read(fd, &version_data, sizeof(int));
    if (bytes_read != sizeof(int)) {
        perror("read version data");
        return;
    }
    version_data = ntohl(version_data);

    if (version_data != 1) {
        printf("Protocol mismatch! Expected version 1, got %d\n", version_data);
        return;
    }


    // read payload
    if (hdr->string_len > 0) {
        char string_buf[4096] = {0};
        bytes_read = read(fd, string_buf, hdr->string_len);
        if (bytes_read != hdr->string_len) {
            printf("Meh: %d %ld\n", hdr->string_len, bytes_read);
            perror("read string data");
            return;
        }

        printf("Successfully connected to server, protocol v%d\n", version_data);
        printf("Server message: %.*s\n", hdr->string_len, string_buf);
    } else {
        printf("Successfully connected to server, protocol v%d (no message)\n", version_data);
    }
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

    handle_server_response(serverSocket);

    close(serverSocket);
}
