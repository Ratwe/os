#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int main() {
    int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Can't create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    pid_t mypid = getpid();
    sprintf(addr.sun_path, "%d.socket", mypid);

    int addr_len = sizeof(addr);

    if (bind(sockfd, (struct sockaddr*)&addr, addr_len) == -1) {
        perror("Can't bind socket");
        exit(EXIT_FAILURE);
    }

    printf("Server started; socket file is %s\n", addr.sun_path);

    struct sockaddr_un sender;
    socklen_t sender_len = sizeof(sender);
    char buf[1024];

    while (1) {
        ssize_t bytes_received = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&sender, &sender_len);
        if (bytes_received > 0) {
            printf("%.*s\n", (int)bytes_received, buf);
        }
    }

    close(sockfd);
    unlink(addr.sun_path);  // Remove the socket file
    return 0;
}
