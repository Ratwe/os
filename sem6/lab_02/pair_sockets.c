#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/wait.h>
#include "fcntl.h"
#include "errno.h"

#define N 2

int main()
{

	char* msg[2] = {"bb", "ccc"};

	int fd[2]; // дескрипторы сокетов
	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, fd) != 0)
		perror("Can't socketpair");

	int n;
	pid_t childpid[N];
	char buf[1024];
	for (int i = 0; i < N; i++)
	{
		if ((childpid[i] = fork()) == -1)
		{
			perror("Can't fork");
			return EXIT_FAILURE;
		}
		else if (childpid[i] == 0)
		{
			close(fd[1]);
			write(fd[0], msg[i], strlen(msg[i]) + 1);
			printf("child %d sent %s\n", getpid(), msg[i]);
			read(fd[0], buf, sizeof(buf));
			printf("child %d got %s\n", getpid(), buf);
			close(fd[0]);
			exit(0);
		}
		
		else
		{ 
			close(fd[0]);
			read(fd[1], buf, sizeof(buf));
			printf("parent %d got from child %d----- %s\n", getpid(), childpid[i], buf);
			write(fd[1], getpid(), sizeof(int));
			//printf("parent %d sent: %s\n", getpid(), "a");
			close(fd[1]);
		}
		
	}	

	int wstatus;
	for (int i = 0; i < N; i++)
	{
		int pid_w = waitpid(childpid[i], &wstatus, WUNTRACED | WCONTINUED);
		if (WIFEXITED(wstatus)) {
			printf("child %d exited, status=%d\n", pid_w, WEXITSTATUS(wstatus));
		}
		else if (WIFSIGNALED(wstatus)) {
			printf("child %d killed by signal %d\n", pid_w, WTERMSIG(wstatus));
		}
		else if (WIFSTOPPED(wstatus)) {
			printf("child %d stopped by signal %d\n", pid_w, WSTOPSIG(wstatus));
		}
		else if (WIFCONTINUED(wstatus)) {
			printf("child %d continued\n", pid_w);
		}
	}
	
	return 0;
}


