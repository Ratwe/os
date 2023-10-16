#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>


/* Оба потомка пишут в один программный канал свои сообщения.
Запрещается передавать какие-то осмысленные сообщения.
Первый child передаёт три буквы, второй - десять.
Размер сообщения определять динамически - sizeof().
Parent их читает три раза. Чтобы убедиться, что прочитанное сообщение перестаёт существовать:
читает от первого, второго, читает третий раз -- труба пустая.
Писать в один buf неопределённого размера.*/

int main()
{
	char text[11] = {0};
    char* msg[11] = { "aaa", "bbbbbbbbbb" };
    int pipefd[2];
	int childpid[2];
	int wstatus;
    if (pipe(pipefd) == -1)
    {
        perror("Can't pipe.");
        exit(EXIT_FAILURE);
    }
    printf("Parent: pid=%d, ppid=%d, gid=%d\n", getpid(), getppid(), getpgrp());
    
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
            write(pipefd[1], msg[i], sizeof(msg[i]));
            printf("Message '%s' sent to parent.\n", msg[i]);
            return EXIT_SUCCESS;
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

	printf("\nMessage received:\n");
	close(pipefd[1]);
	read(pipefd[0], text, 11);
	printf("%s\n", text);
}
