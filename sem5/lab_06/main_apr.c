#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdarg.h>
#include <pthread.h>

#define LOCKFILE "/var/run/lab6_daemon.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

#define MAXLINE 4096

sigset_t    mask;

int lockfile(int fd)
{
	struct flock fl;

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return(fcntl(fd, F_SETLK, &fl));
}

static void err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
	char    buf[MAXLINE];

	vsnprintf(buf, MAXLINE, fmt, ap);
	if (errnoflag)
		snprintf(buf+strlen(buf), MAXLINE-strlen(buf), ": %s",
				 strerror(error));
	strcat(buf, "\n");
	fflush(stdout);     /* in case stdout and stderr are the same */
	fputs(buf, stderr);
	fflush(NULL);       /* flushes all stdio output streams */
}

void err_quit(const char *fmt, ...)
{
	va_list     ap;

	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
	exit(1);
}

void err_exit(int error, const char *fmt, ...)
{
	va_list     ap;

	va_start(ap, fmt);
	err_doit(1, error, fmt, ap);
	va_end(ap);
	exit(1);
}


int already_running(void)
{
	int fd;
	char buf[16];
	fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
	if (fd < 0)
	{
		syslog(LOG_ERR, "�� �������� ������� %s: %s", LOCKFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (lockfile(fd) < 0)
	{
		if (errno == EACCES || errno == EAGAIN)
		{
			close(fd);
			return(1);
		}
		syslog(LOG_ERR, "���������� ���������� ���������� �� %s: %s", LOCKFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}
	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf)+1);
	return(EXIT_SUCCESS);
}

void daemonize(const char *cmd)
{
	int i, fd0, fd1, fd2;
	pid_t pid;
	struct rlimit rl;
	struct sigaction sa;
	/*
	 * �������� ����� ������ �������� �����.
	 */
	umask(0);
	/*
	 * �������� ����������� ��������� ����� ����������� �����.
	 */
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		err_quit("%s: ���������� �������� ������������ ����� ����������� ", cmd);
	/*
	 * ����� ������� ����� ������, ����� �������� ����������� ��������.
	 */
	if ((pid = fork()) < 0)
		err_quit("%s: fork", cmd);
	else if (pid != 0) /* ������������ ������� */
		exit(EXIT_SUCCESS);
	setsid();  // �������� ����� ������
	/*
	 * ���������� ������������� ��������� ������������ ��������� � �������.
	 */
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		err_quit("%s: ���������� ������������ ������ SIGHUP");
	if ((pid = fork()) < 0)
		err_quit("%s: ������ ������ ������� fork", cmd);
	else if (pid != 0) // ������������ �������
		exit(0);
/*
 * ��������� �������� ������� ������� ������� ���������,
 * ����� ������������ ����� ���� ������������� �������� �������.
 */
	if (chdir("/") < 0)
		err_quit("%s: ���������� ������� ������� ������� ��������� /");
/*
 * ������� ��� �������� �������� �����������.
 */
	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for (i = 0; i < rl.rlim_max; i++)
		close(i);
/*
 * ������������ �������� ����������� 0, 1 � 2 � /dev/null.
 */
	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);
/*
 * ���������������� ���� �������.
 */
	openlog(cmd, LOG_CONS, LOG_DAEMON);
	if (fd0 != 0 || fd1 != 1 || fd2 != 2)
	{
		syslog(LOG_ERR, "��������� �������� ����������� %d %d %d",
			   fd0, fd1, fd2);
		exit(EXIT_FAILURE);
	}
}
void reread(void)
{
	syslog(LOG_INFO, "reread");
}
void *thr_fn(void *arg)
{
	int err, signo;
	for (;;) {
		err = sigwait(&mask, &signo);
		if (err != 0) {
			syslog(LOG_ERR, "������ ������ ������� sigwait");
			exit(EXIT_FAILURE);
		}

		switch (signo) {
			case SIGHUP:
				syslog(LOG_INFO, "������ ����������������� �����");
				reread();
				return(EXIT_SUCCESS);
				break;

			case SIGTERM:
				syslog(LOG_INFO, "������� ������ SIGTERM; �����");
				exit(EXIT_SUCCESS);

			default:
				syslog(LOG_INFO, "������� �������������� ������ %d\n", signo);
		}
	}
	return(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	int                 err;
	pthread_t           tid;
	char                *cmd;
	struct sigaction    sa;
	if ((cmd = strrchr(argv[0], '/')) == NULL)
		cmd = argv[0];
	else
		cmd++;
	/*
	 * Become a daemon.
	 */
	daemonize(cmd);
	/*
	 * Make sure only one copy of the daemon is running.
	 */
	if (already_running()) {
		syslog(LOG_ERR, "����� ��� �������");
		exit(EXIT_FAILURE);
	}
	/*
	 * Restore SIGHUP default and block all signals.
	 */
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		err_quit("%s: ���������� ������������ �������� SIG_DFL ��� SIGHUP");
	sigfillset(&mask);
	if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
		err_exit(err, "������ ���������� �������� SIG_BLOCK");
	/*
	  Create a thread to handle SIGHUP and SIGTERM.
	*/
	err = (&tid, NULL, thr_fn, 0);
	if (err != 0)
		err_exit(err, "���������� ������� �����");
	syslog(LOG_INFO, "trying to join");
	pthread_join(tid, NULL);
	syslog(LOG_INFO, "joined");
	char buff[64];
	int i = 0;
	while(1)
	{
		sleep(1);
		sprintf(buff, "aaa%d", i++);
		syslog(LOG_INFO, buff);
	}
	exit(EXIT_SUCCESS);
}