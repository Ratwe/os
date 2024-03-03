#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
	pid_t childpid[2];
	for (int i = 0; i < 2; i++)
	{
		if ((childpid[i] = fork()) == -1)
			exit(1);
		else if (childpid[i] == 0)
		{
			printf("Child[%d]: pid=%d, ppid=%d, gid=%d\n", i, getpid(), getppid(), getpgrp());
			sleep(2);
			printf("After sleep(2)\nChild[%d]: pid=%d, ppid=%d, gid=%d\n", i, getpid(), getppid(), getpgrp());
			exit(0);
		}
		else
			printf("Parent: pid=%d, ppid=%d, gid=%d\n", getpid(), getppid(), getpgrp());
	}
	return 0;
}

