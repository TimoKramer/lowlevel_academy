#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
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
    unsigned short string_len;
} proto_hdr_t;

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
