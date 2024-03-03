#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
	char* buf;
    char* msg[2] = {"aaa", "bbbbbbbbbb"};
    int pipefd[2];
	int childpid[2];
	int wstatus;

    if (pipe(pipefd) == -1)
    {
        perror("Can't pipe.");
        exit(1);
    }

	for (int i = 0; i < 2; i++)
	{
		if ((childpid[i] = fork()) == -1)
		{
			printf("Can't fork!\n");
			exit(1);
		}
		else if (childpid[i] == 0)
		{
            close(pipefd[0]);
            write(pipefd[1], msg[i], strlen(msg[i]));
            printf("Child %d sent message %s\n", getpid(), msg[i]);
            exit(0);
		}
	}

	for (int i = 0; i < 2; i++)
	{
		waitpid(childpid[i], &wstatus, WUNTRACED | WCONTINUED);
		if (WIFEXITED(wstatus)) {
			printf("exited, status=%d\n", WEXITSTATUS(wstatus));
		}
		else if (WIFSIGNALED(wstatus)) {
			printf("killed by signal %d\n", WTERMSIG(wstatus));
		}
		else if (WIFSTOPPED(wstatus)) {
			printf("stopped by signal %d\n", WSTOPSIG(wstatus));
		}
		else if (WIFCONTINUED(wstatus)) {
			printf("continued\n");
		}
	}

	for (int i = 0; i < 2; i++)
	{
		buf = "";
		buf = malloc(strlen(msg[i]));
		close(pipefd[1]);
		read(pipefd[0], buf, strlen(msg[i]));
		printf("got %s\n", buf);
	}

	buf = "";
	buf = malloc(strlen(msg[1]));
	close(pipefd[1]);
	read(pipefd[0], buf, strlen(msg[1]));
	printf("got %s\n", buf);

	return 0;
}
