#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

int fl = 0;
void sig_handler(int sig_num)
{
	fl = 1;
}
int main()
{
    int pipefd[2];
	int childpid[2];
	int wstatus;
	ssize_t res;
	char cur_char;

    if (pipe(pipefd) == -1)
    {
        perror("Can't pipe.");
        exit(1);
    }
	printf("Press ctrl+c - child  write in pipe\n");
	if (signal(SIGINT, sig_handler) == SIG_ERR)
	{
		perror("Signal error\n");
		exit(1);
	}
	sleep(2);
	//printf("Press ctrl+c - child  write in pipe\n");
	
	printf("Parent pid = %d, grid = %d\n", getpid(), getpgrp());
	for (int i = 0; i < 2; i++)
	{
		if ((childpid[i] = fork()) == -1)
		{
			printf("Can't fork!\n");
			exit(1);
		}
		else if (childpid[i] == 0)
		{
			printf("Child pid = %d, grid = %d\n", getpid(), getpgrp());
			if (fl == 1)
			{
				close(pipefd[0]);
				if (i == 0)
				{
					write(pipefd[1], "aaa\n", 4);
					printf("Child %d sent message \n", getpid());
				}
				else if (i == 1)
				{
					write(pipefd[1], "xxxxxxxxxxxxxxxxxxxx\n", 22);
					printf("Child %d sent message bbbbbbb\n", getpid());
				}
				exit(0);
			}
		}
	}
	
	close(pipefd[1]);
	for (int i = 0; i < 2; i++)
	{
		if (fl == 1)
		{
			printf("Parent got: ");
			do
			{
				res = read(pipefd[0], &cur_char, 1);
				if (res == -1)
				{
					printf("Read error!\n");
					exit(1);
				}
				printf("%c", cur_char);
			} while (cur_char != '\n');
		}
	}
	close(pipefd[0]);
	for (int i = 0; i < 2; i++)
	{
		int pid_w = waitpid(childpid[i], &wstatus, WUNTRACED | WCONTINUED);
		if (WIFEXITED(wstatus)) {
			printf("pid_w %d exited, status=%d\n", pid_w, WEXITSTATUS(wstatus));
		}
		else if (WIFSIGNALED(wstatus)) {
			printf("pid_w %d killed by signal %d\n", pid_w, WTERMSIG(wstatus));
		}
		else if (WIFSTOPPED(wstatus)) {
			printf("pid_w %d stopped by signal %d\n", pid_w, WSTOPSIG(wstatus));
		}
		else if (WIFCONTINUED(wstatus)) {
			printf("pid_w %d continued\n", pid_w);
		}
	}
	return 0;
}
