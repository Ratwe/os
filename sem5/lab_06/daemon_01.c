#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <time.h>
#include <syslog.h>
#include <unistd.h>
#define LOCKFILE "/var/run/daemon_1.pid"
sigset_t mask;
int lockfile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return fcntl(fd, F_SETLK, &fl);
}
int already_running(void)
{
    int fd;
    char buf[16];
    int perms = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    fd = open(LOCKFILE, O_RDWR | O_CREAT, perms);
    if (fd < 0)
    {
        syslog(LOG_ERR, "���������� ������� %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (lockfile(fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "���������� ���������� ���������� �� %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    return 0;
}
void daemonize(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;
    umask(0);
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        printf("%s: ���������� �������� ������������ ����� ����������� ", cmd);
    if ((pid = fork()) < 0)
        printf("%s: ������ ������ ������� fork", cmd);
    else if (pid > 0) 
        exit(0);
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        printf("%s: ���������� ������������ ������ SIGHUP", cmd);
    if(setsid() == -1)
    {
      printf("%s: ������ ������ ������� setsid", cmd);
      exit(1);
    }
    if (chdir("/") < 0)
        printf("%s: ���������� ������� ������� ������� ���������", cmd);
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; i++)
        close(i);
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "��������� �������� ����������� %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
}
void reread(void)
{
    FILE* fd;
    int pid;
    if ((fd = fopen(LOCKFILE, "r")) == NULL)
        syslog(LOG_ERR, "fopen");
    fscanf(fd, "%d", &pid);
    fclose(fd);
    syslog(LOG_INFO, "daemon pid: %d", pid);
}
void *thr_fn(void *arg)
{
    int err, signo;
    for (;;)
    {
        err = sigwait(&mask, &signo);
        if (err != 0)
        {
            syslog(LOG_ERR, "������ ������ ������� sigwait");
            exit(1);
        }
        switch (signo)
        {
        case SIGHUP:
            syslog(LOG_INFO, "������ ����������������� �����");
            reread();
            break;
        case SIGTERM:
            syslog(LOG_INFO, "������� SIGTERM; �����");
            exit(0);   
        case SIGINT:
            syslog(LOG_INFO, "got SIGINT");
            pthread_exit(0);
        default:
            syslog(LOG_INFO, "������� ������ %d\n", signo);
        }
    }
    pthread_exit(0);
}
int main(int argc, char *argv[])
{
    int err;
    pthread_t tid;
    char *cmd;
    struct sigaction sa;
    if ((cmd = strrchr(argv[0], '/')) == NULL)
        cmd = argv[0];
    else
        cmd++;
    daemonize(cmd);
    if (already_running())
    {
        syslog(LOG_ERR, "����� ��� �������");
        exit(1);
    }
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        syslog(LOG_SYSLOG, "���������� ������������ �������� SIG_DFL ��� SIGHUP");
    sigfillset(&mask);
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
        printf("������ ���������� �������� SIG_BLOCK");
    if (pthread_create(&tid, NULL, thr_fn, NULL) == -1)
        syslog(LOG_ERR, "���������� ������� �����");
    time_t raw_time;
    for (;;)
    {
        sleep(30);
        time(&raw_time);
        syslog(LOG_INFO, "Time: %s", ctime(&raw_time));
    }
}
