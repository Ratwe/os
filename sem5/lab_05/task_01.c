#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SIZE_BUF 1024
#define PRODUCERS_AMOUNT 5
#define CONSUMERS_AMOUNT 3
#define MAX_RAND_P 5

#define BIN_SEM 0
#define BUF_EMPTY 1
#define BUF_FULL 2

#define P -1
#define V  1

struct sembuf start_produce[2] = { {BUF_EMPTY, P, 0}, {BIN_SEM, P, 0} };
struct sembuf stop_produce[2] = { {BIN_SEM, V, 0}, {BUF_FULL, V, 0} };
struct sembuf start_consume[2] = { {BUF_FULL, P, 0}, {BIN_SEM, P, 0} };
struct sembuf stop_consume[2] = { {BIN_SEM, V, 0}, {BUF_EMPTY, V, 0} };

int fl = 1;
void sig_handler(int sig_num)
{
    printf("Pid %d handled signal\n", getpid());
    fl = 0;
}

void producer(int semid, char** prod_ptr, char* alpha_ptr)
{
    while (fl)
    {
        srand(time(NULL));
        sleep(rand() % MAX_RAND_P + 1);
        if (semop(semid, start_produce, 2) == -1)
        {
            perror("start produce semop");
            exit(1);
        }
        *(*prod_ptr) = *alpha_ptr;
        printf("Producer pid = %d; wrote %c\n", getpid(), *(*prod_ptr));
        (*prod_ptr)++;
        *alpha_ptr = *alpha_ptr < 'z' ? *alpha_ptr + 1 : 'a';
        if (semop(semid, stop_produce, 2) == -1)
        {
            perror("stop produce semop");
            exit(1);
        }
    }
    exit(0);
}   

void consumer(int semid, char** cons_ptr)
{
    while (fl)
    {
        srand(time(NULL));
        sleep(rand() % MAX_RAND_P + 1);
        if (semop(semid, start_consume, 2) == -1)
        {
            perror("start consume semop");
            exit(1);
        }
        printf("Consumer pid = %d; got   %c\n", getpid(), *(*cons_ptr));
        (*cons_ptr)++;
        if (semop(semid, stop_consume, 2) == -1)
        {
            perror("stop consume semop");
            exit(1);
        }
    }
    exit(0);
}

int main()
{
    int childpid[PRODUCERS_AMOUNT + CONSUMERS_AMOUNT];
    pid_t w;
    int status;
    int segid;
    int semid;
    int perms = S_IRWXU | S_IRWXG | S_IRWXO;
    char* addr;
    char* (*cons_ptr);
    char* (*prod_ptr);
    char* alpha_ptr;
    segid = shmget(IPC_PRIVATE, SIZE_BUF, IPC_CREAT | perms);  // идентификатор разделяемой памяти
    printf("Press ctrl+c - stop producers and consumers\n");
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        perror("Signal error\n");
        exit(1);
    }  
    if (segid == -1)
    {
        perror("shmget");
        exit(1);
    }
    addr = shmat(segid, NULL, 0); // теперь addr содержит адрес разделяемой памяти
    if (addr == (char*)-1)
    {
        perror("shmat");
        exit(1);
    }
    // Соответственно, в буфере, начиная с начального адреса, положить текущий адрес производителя. 
    // Затем текущий адрес потребителя. Затем - текущую букву. 
    cons_ptr = (char**)addr;
    prod_ptr = cons_ptr + 1;
    alpha_ptr = (char*)(prod_ptr + 1);
    *cons_ptr = alpha_ptr + 1;
    *prod_ptr = *cons_ptr;
    *alpha_ptr = 'a';
    semid = semget(IPC_PRIVATE, 3, IPC_CREAT | perms);
    if (semid == -1)
    {
        perror("semget");
        exit(1);
    }
    int c_sb = semctl(semid, BIN_SEM, SETVAL, 1);
    if (c_sb == -1)
    {
        perror("bin semctl");
        exit(1);
    }
    int c_se = semctl(semid, BUF_EMPTY, SETVAL, SIZE_BUF);
    if (c_se == -1)
    {
        perror("empty semctl");
        exit(1);
    }
    int c_sf = semctl(semid, BUF_FULL, SETVAL, 0);
    if (c_sf == -1)
    {
        perror("full semctl");
        exit(1);
    }
    for (int i = CONSUMERS_AMOUNT; i < CONSUMERS_AMOUNT + PRODUCERS_AMOUNT; i++)
    {
        if ((childpid[i] = fork()) == -1)
        {
            printf("Can't fork producer!\n");
            exit(1);
        }
        if (childpid[i] == 0)
        {
            producer(semid, prod_ptr, alpha_ptr);
            printf("Producer pid = %d ended cycle\n", getpid());
            exit(0);
        }
    }
    for (int i = 0; i < CONSUMERS_AMOUNT; i++)
    {
        if ((childpid[i] = fork()) == -1)
        {
            printf("Can't fork consumer!\n");
            exit(1);
        }
        if (childpid[i] == 0)
        {   
            consumer(semid, cons_ptr);
            printf("Сonsumer pid = %d end cycle\n", getpid());
            exit(0);
        }
    }
    for (int i = 0; i < CONSUMERS_AMOUNT + PRODUCERS_AMOUNT; i++)
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
    if (semctl(semid, 1, IPC_RMID, NULL) == -1)
    {
        perror("semctl");
        exit(1);
    }
    if (shmdt((void*)addr) == -1)
    {
        perror("shmdt");
        exit(1);
    }
    if (shmctl(segid, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
        exit(1);
    }
    return 0;
}
