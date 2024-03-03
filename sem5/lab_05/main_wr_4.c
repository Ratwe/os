#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WAITING_WRITERS 0
#define WAITING_READERS 1
#define ACTIVE_WRITER 2
#define ACTIVE_READERS 3

#define BUFF_SIZE sizeof(int)
#define WRITER_AMOUNT 5
#define READER_AMOUNT 3

int fl = 1;
void sig_handler(int sig)
{
    printf("Process [pid = %d]: recieve signal SIGINT\n", getpid());
    fl = 0;
}
struct sembuf start_write[] = {
    {WAITING_WRITERS, 1, SEM_UNDO},
    {ACTIVE_WRITER, 0, 0},
    {ACTIVE_READERS, 0, 0},
    {ACTIVE_WRITER, 1, SEM_UNDO},
    {WAITING_WRITERS, -1, 0},
};
struct sembuf stop_write[] = {
    {ACTIVE_WRITER, -1, 0},
};
struct sembuf start_read[] = {
    {WAITING_READERS, 1, SEM_UNDO},
    {ACTIVE_WRITER, 0, 0},
    {WAITING_WRITERS, 0, 0},
    {ACTIVE_READERS, 1, SEM_UNDO},
    {WAITING_READERS, -1, 0},
};
struct sembuf stop_read[] = {
    {ACTIVE_READERS, -1, 0},
};
void write_lock(int semid)
{
    if (semop(semid, start_write, 5) == -1)
    {
        perror("start_write semop");
        exit(1);
    }
}
void write_release(int semid)
{
    if (semop(semid, stop_write, 1) == -1)
    {
        perror("stop_write semop");
        exit(1);
    }
}
void read_lock(int semid)
{
    if (semop(semid, start_read, 5) == -1)
    {
        perror("start_read semop");
        exit(1);
    }
}
void read_release(int semid)
{
    if (semop(semid, stop_read, 1) == -1)
    {
        perror("stop_read semop");
        exit(1);
    }
}
void writer(int semid, char *value_ptr)
{
    srand(time(0));
    while (fl)
    {
        write_lock(semid);
        usleep((double)rand() / RAND_MAX * 1000000 * 100);
        (*value_ptr)++;
        printf("Writer [pid = %d]: %d\n", getpid(), *value_ptr);
        write_release(semid);
    }
    exit(0);
}
void reader(int semid, char *value_ptr)
{
    srand(time(0));
    while(fl)
    {
        read_lock(semid);
        usleep((double)rand() / RAND_MAX * 1000000 * 100);
        printf("Reader [pid = %d]: %d\n", getpid(), *value_ptr);
        read_release(semid);
    }
    exit(0);
}
void stats(int semid)
{
    srand(time(0));
    while (fl)
    {
        printf("waiting writers amount: %d\n", semctl(semid, WAITING_WRITERS, GETVAL, 0));
        printf("waiting readers amount: %d\n", semctl(semid, WAITING_READERS, GETVAL, 0));
        usleep((double)rand() / RAND_MAX * 1000000 * .5);
    }
    exit(0);
}
int main(void)
{
    pid_t pid;
    int status;
    pid_t w;
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    char *value_ptr;
    int shmid;
    int semid;
    shmid = shmget(100, sizeof(int), IPC_CREAT | perms);
    if (shmid == -1)
    {
        perror("shmget");
        exit(1);
    }
    value_ptr = shmat(shmid, NULL, 0);
    if (value_ptr == (char*) -1)
    {
        perror("shmat");
        exit(1);
    }
    *value_ptr = 0;
    semid = semget(100, 5, IPC_CREAT | perms);
    if (semid == -1)
    {
        perror("semget");
        exit(1);
    }
    if (semctl(semid, ACTIVE_READERS, SETVAL, 0) == -1)
    {
        perror("active_readers semctl");
        exit(1);
    }
    if (semctl(semid, ACTIVE_WRITER, SETVAL, 0) == -1)
    {
        perror("active_writer semctl");
        exit(1);
    }
    if (semctl(semid, WAITING_READERS, SETVAL, 0) == -1)
    {
        perror("waiting_readers semctl");
        exit(1);
    }
    if (semctl(semid, WAITING_WRITERS, SETVAL, 0) == -1)
    {
        perror("waiting_writers semctl");
        exit(1);
    }
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        perror("sigint signal");
        exit(1);
    }
    printf("Press Ctrl+C to stop simulation\n\n");
    for (int i = 0; i < WRITER_AMOUNT; i++)
    {
        pid = fork();
        if (pid == -1)
        {
            perror("writer fork");
            exit(1);
        }
        if (pid == 0)
        {
            writer(semid, value_ptr);
        }
    }
    for (int i = 0; i < READER_AMOUNT; i++)
    {
        pid = fork();
        if (pid == -1)
        {
            perror("reader fork");
            exit(1);
        }
        if (pid == 0)
        {
            reader(semid, value_ptr);
        }
    }
    pid = fork();
    if (pid == -1)
    {
        perror("stats fork");
        exit(1);
    }
    if (pid == 0)
    {
        stats(semid);
    }
    for (int i = 0; i < READER_AMOUNT + WRITER_AMOUNT; i++)
    {
        w = wait(&status);
        if (w == -1)
        {
            perror("wait");
            exit(1);
        }
        if (WIFEXITED(status))
        {
            printf("Child [pid = %d]: exit with code %d\n", w, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Child [pid = %d]: abort by signal %d\n", w, WTERMSIG(status));
        }
        else if (WIFSTOPPED(status))
        {
            printf("Child [pid = %d]: stop by signal %d\n", w, WSTOPSIG(status));
        }
    }
    if (semctl(semid, 1, IPC_RMID, NULL) == -1)
    {
        perror("semctl");
        exit(1);
    }
    if (shmdt((void *) value_ptr) == -1)
    {
        perror("shmdt");
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        exit(1);
    }
    exit(0);
    return EXIT_SUCCESS;
}
