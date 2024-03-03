#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>

#define READERS_COUNT 5
#define WRITERS_COUNT 3
#define ACTIVE_READERS 0
#define ACTIVE_WRITER  1
#define WAITING_READERS 2
#define WAITING_WRITERS 3
#define BIN_SEM 4
#define MAX_RAND_P 10

struct sembuf start_read[] = {
    {WAITING_READERS, 1, 0},
    {WAITING_WRITERS, 0, 0},
    {BIN_SEM, 0, 0},
    {ACTIVE_READERS, 1, 0},
    {WAITING_READERS, -1, 0},
};
struct sembuf stop_read[] = {
    {ACTIVE_READERS, -1, 0},
};
struct sembuf start_write[] = {
    {WAITING_WRITERS, 1, 0},
    {ACTIVE_READERS, 0, 0},
    {WAITING_READERS, 0, 0},
    {WAITING_WRITERS, -1, 0},
    {BIN_SEM, -1, 0}
};
struct sembuf stop_write[2] = {
    {BIN_SEM, 1, 0},
};

int fl = 1;
void sig_handler(int sig_num)
{
    printf("Pid %d handled signal %d\n", getpid(), sig_num);
    fl = 0;
}
void reader(int sem_id, char* value_ptr)
{
    srand(time(NULL));
    int stime;
    while (fl)
    {
        stime = rand() % MAX_RAND_P + 1;
        sleep(stime);
        if (semop(sem_id, start_read, 5) == -1)
        {
            perror("read_lock");
            exit(1);
        }
        printf("Reader %d got: %d\n", getpid(), *value_ptr);
        if (semop(sem_id, stop_read, 1) == -1)
        {
            perror("read_release");
            exit(1);
        }
    }
    exit(0);
}
void writer(int sem_id, char* value_ptr)
{
    srand(time(NULL));
    int stime;
    while (fl)
    {
        stime = rand() % MAX_RAND_P + 1;
        sleep(stime);
        if (semop(sem_id, start_write, 5) == -1)
        {
            perror("write_lock");
            exit(1);
        }
        (*value_ptr)++;
        printf("Writer %d wrote: %d\n", getpid(), *value_ptr);
        if (semop(sem_id, stop_write, 1) == -1)
        {
            perror("wrte_release");
            exit(1);
        }
    }
    exit(0);
}
int main()
{
    pid_t pid;
    int status;
    pid_t w;
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    char* value_ptr;
    int shm_id;
    int sem_id;
    int child_pid;
    shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | perms);
    if (shm_id == -1)
    {
        perror("shmget");
        exit(1);
    }
    value_ptr = shmat(shm_id, NULL, 0);
    if (value_ptr == (char*)-1)
    {
        perror("shmat");
        exit(1);
    }
    *value_ptr = 0;
    sem_id = semget(IPC_PRIVATE, 5, IPC_CREAT | perms);
    if (sem_id == -1)
    {
        perror("semget");
        exit(1);
    }
    if (semctl(sem_id, ACTIVE_READERS, SETVAL, 0) == -1)
    {
        perror("active_readers semctl");
        exit(1);
    }
    if (semctl(sem_id, ACTIVE_WRITER, SETVAL, 1) == -1)
    {
        perror("active_writer semctl");
        exit(1);
    }
    if (semctl(sem_id, WAITING_READERS, SETVAL, 0) == -1)
    {
        perror("waiting_readers semctl");
        exit(1);
    }
    if (semctl(sem_id, WAITING_WRITERS, SETVAL, 0) == -1)
    {
        perror("waiting_writers semctl");
        exit(1);
    }
    if (semctl(sem_id, BIN_SEM, SETVAL, 1) == -1)
    {
        perror("bin_sems semctl");
        exit(1);
    }
    printf("Press Ctrl+C -> simulation stop\n");
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        perror("sigint signal");
        exit(1);
    }
    for (int i = 0; i < WRITERS_COUNT; i++)
    {
        child_pid = fork();
        if (child_pid == 0)
        {
            writer(sem_id, value_ptr);
        }
        else if (child_pid == -1)
        {
            perror("fork");
            exit(1);
        }
    }
    for (int i = 0; i < READERS_COUNT; i++)
    {
        child_pid = fork();
        if (child_pid == 0)
        {
            reader(sem_id, value_ptr);
        }
        else if (child_pid == -1)
        {
            perror("fork");
            exit(1);
        }
    }
    for (int i = 0; i < WRITERS_COUNT + READERS_COUNT; i++)
    {
        w = wait(&status);
        if (w == -1)
        {
            perror("wait");
            exit(1);
        }
        if (WIFEXITED(status))
        {
            printf("pid = %d: exited with code %d\n", w, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("pid = %d: aborted by signal %d\n", w, WTERMSIG(status));
        }
        else if (WIFSTOPPED(status))
        {
            printf("pid = %d: stopped by signal %d\n", w, WSTOPSIG(status));
        }
    }
    if (semctl(sem_id, 1, IPC_RMID, NULL) == -1)
    {
        perror("semctl");
        exit(1);
    }
    if (shmdt((void*)value_ptr) == -1)
    {
        perror("shmdt");
        exit(1);
    }
    if (shmctl(sem_id, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        exit(1);
    }
    return 0;
}
