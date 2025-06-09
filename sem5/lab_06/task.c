#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <errno.h> 
#include <stdio.h>
#include <sys/stat.h>
#include ""

#define LOCKFILE "/var/run/daemon.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

void daemonize(const char *name);
int lockfile(int fd);
int already_running(void);
void *my_thread(void *arg);

sigset_t mask;

int main(int argc, char *argv[])
{
    daemonize("dasha_daemon"); // ���� �� ��� ������

    if (already_running())
    {
        syslog(LOG_ERR, "����� ��� �������\n");
        exit(1);
    }
    else
    {
        syslog(LOG_INFO, "����� �������\n");
    }

    int err;
	pthread_t tid;
    struct sigaction sa;

    //������������ �������� �� ��������� ��� ������� SIGHUP
    //� ������������� ��� �������
    
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
        syslog(LOG_ERR, "���������� ������������ �������� SIG_DFL ��� SIGHUP\n");
    }

    sigfillset(&mask);
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
    {
        syslog(LOG_ERR, "������ ���������� �������� SIG_BLOCK\n");
    }
    
    //�������� ������, ������� ����� ���������� ���������� SIGHUP � SIGTERM
     
    err = pthread_create(&tid, NULL, my_thread, 0);
	if (err != 0)
    {
        syslog(LOG_ERR, "���������� ������� �����\n");
    }
    else
    {
        syslog(LOG_INFO, "������ ����� �����\n");
    }

    time_t timestamp;
    struct tm *time_info;
    syslog(LOG_DAEMON, "����: %s", getlogin());
    
    for (;;)
    {
        sleep(SLEEP_TIME);
        time(&timestamp);
        time_info = localtime(&timestamp);
        syslog(LOG_DAEMON, "%s", asctime(time_info));  
    }

    return 0;
}

void daemonize(const char *name)
{
    // ����� ������� ��� ��������� ����� � ������ ������� �������

    umask(0); 

    //�������� ����������� ��������� ����� ����������� �����, ���� ����� ������� ���

    struct rlimit r_limit;

    if (getrlimit(RLIMIT_NOFILE, &r_limit) < 0) 
    { 
        syslog(LOG_ERR, "���������� ���������� ������������ ����� ����������� �����\n");
    }

    pid_t pid;

    if ((pid = fork()) == -1)
    {
        syslog(LOG_ERR, "������ ������ fork\n");
    } 
    else if (pid != 0)
    {
        exit(0);
    }

    struct sigaction sa;

    // ������������ ������������� ��������� ������������ ���������

    sa.sa_handler = SIG_IGN; // ������ ignore this signal
    sigemptyset(&sa.sa_mask); // ��������� empty mask
    sa.sa_flags = 0;

    if (sigaction(SIGHUP, &sa, NULL) < 0) // SIGHUB �������� ��������, ��� ������� ������� ����������� ��������
    {
        syslog(LOG_ERR, "���������� ������������ SIGHUP\n");
    }  

    // ������� ������ �� �������� ������� ������ � ��� �������� �������� ������ ������� setsid()
    // ������� ������. ��� ���� ������� ��������� ������� ����� ������, ������� ����� ������ ���������
    // � �������� ������������ ���������

    if (setsid() == -1) 
    {
        syslog(LOG_ERR, "������ ������ setsid\n");
        exit(1);
    }

    // ������ ������� ������� �� ��������
    

    if (chdir("/") < 0) 
    {
        syslog(LOG_ERR, "���������� �������� ������� �� /\n");
    }

    // ��������� ��� �������� �������� �����������

    if (r_limit.rlim_max == RLIM_INFINITY)
    {
        r_limit.rlim_max = 1024; // ����������� ��������� ���������� �������� ������
    }

    for (int i = 0; i < r_limit.rlim_max; i++)
    {
        close(i);
    }

    // ������������ �������� ����������� � 0, 1, 2 � /dev/null

    int fd0, fd1, fd2; // stdin, stdout, stderr

    fd0 = open("/dev/null", O_RDWR); 
    fd1 = dup(0);
    fd2 = dup(0);
   
    // �������������� ���� �������

    openlog(name, LOG_CONS, LOG_DAEMON);

    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "������ %d, %d, %d\n", fd0, fd1, fd2);
        exit(1);
    }

}

int lockfile(int fd)
{
    // ��������� ���� ���������
    // ����������� ������������ ���������� � ��������� � �����������
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

    fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);

    if (fd < 0)
    {
        syslog(LOG_ERR, "���������� ������� %s : %s\n", LOCKFILE, strerror(errno));
        exit(1);
    }

    if (lockfile(fd) < 0)
    {
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);

            return 1; // ���� �������� ������� �����
        }

        syslog(LOG_ERR, "������ lock %s: %s\n", LOCKFILE, strerror(errno));

        exit(1); // ���� ������ ���������� 
    }

    // �������� ������� ����� ���������� �� ��� �������, ��� 
    // ������������� �������� ���������� ����� ������, 
    // �������������� � ���� ������, ��� ����� ������� �����.
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    return 0;
}

void reread(void) // ������������ ���������������� ����
{
    FILE *fd;
    int pid;
    
    syslog(LOG_DAEMON, "����: %s", getlogin());
    
    if ((fd = fopen("/var/run/daemon.pid", "r")) == NULL)
    {
        syslog(LOG_ERR, "������ �������� ����� /var/run/daemon.pid.");
    }

    fscanf(fd, "%d", &pid);
    fclose(fd);

    syslog(LOG_INFO, "Pid: %d", pid);
}

void *my_thread(void *arg)
{
    int err, signo;
    for (;;)
    {
        err = sigwait(&mask, &signo);

        if (err != 0)
        {
            syslog(LOG_ERR, "������ ������ sigwait\n");
            exit(1);
        }

        switch (signo)
        {
        case SIGHUP:
            syslog(LOG_INFO, "������� ������ SIGHUB\n");
            reread();
            break;
        case SIGTERM:
            syslog(LOG_INFO, "������� ������ SIGTERM, �����\n");
            exit(0);
        default:
            syslog(LOG_INFO, "������� �������������� ������ %d\n", signo);
            break;
        }
    }

    return (void*)0;
}