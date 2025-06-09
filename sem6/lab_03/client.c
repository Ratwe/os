#include "stdlib.h"
#include "string.h"
#include "sys/socket.h"
#include "stdio.h"
#include "unistd.h"

int main(int argc, char *argv[])
{
	int sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{	
		perror("Can't create socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr addr = {.sa_family = AF_UNIX};
	strncpy(addr.sa_data, argv[1], 14);
	printf("Client started; socket file is %s\n", addr.sa_data);

	pid_t mypid = getpid();
	char buf[1024] = {};
	snprintf(buf, 1024, "%d", mypid);
	sendto(sockfd, buf, strlen(buf) + 1, 0, &addr, sizeof(addr.sa_family) + (strlen(addr.sa_data) + 1) * sizeof(char));
}
