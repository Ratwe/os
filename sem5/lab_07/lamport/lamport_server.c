#include "lamport.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "syslog.h"
#include "memory.h"
#include "stdlib.h"
#include <math.h>

#define MAX_CLIENTS 30

bool_t choosing[MAX_CLIENTS] = { FALSE };
int ticket_numbers[MAX_CLIENTS] = { 0 };

typedef struct BAKERY worker_params;
pthread_t worker_threads[MAX_CLIENTS];
worker_params worker_results[MAX_CLIENTS];

int next_thread_idx = 0;

char curr_symb = 'a';

time_t raw_time;
struct tm* local_time;


void get_ticket(void* arg)
{
    worker_params* w_param = arg;
    setbuf(stdout, NULL);

    w_param->num = next_thread_idx++;
    next_thread_idx %= MAX_CLIENTS;
    if (ticket_numbers[w_param->num] != 0)
    {
        syslog(LOG_WARNING, "Overload!");
        w_param->num = -1;
        return;
    }

    choosing[w_param->num] = TRUE;
    int max = ticket_numbers[0];
    for (int i = 1; i < MAX_CLIENTS; ++i)
    {
        if (ticket_numbers[i] > max)
            max = ticket_numbers[i];
    }
    ticket_numbers[w_param->num] = max + 1;
    choosing[w_param->num] = FALSE;
}


// executed in each thread
void* bakery(void* arg)
{
    worker_params* params = arg;
    setbuf(stdout, NULL);

    pthread_t tid;
    tid = pthread_self();

    struct tm* tm_info;
    struct timeval tv;

    gettimeofday(&tv, NULL);

    int millisec = lrint(tv.tv_usec / 1000.0);
    if (millisec >= 1000) {
        millisec -= 1000;
        tv.tv_sec++;
    }

    tm_info = localtime(&tv.tv_sec);
    char buffer[26];
    strftime(buffer, 26, "%H:%M:%S", tm_info);
    syslog(LOG_INFO, "[Worker %d][%s.%03d] start, client num is %d, ticket %d", tid, buffer, millisec, params->num, ticket_numbers[params->num]);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        while (choosing[i]);
        while (ticket_numbers[i] > 0 && (ticket_numbers[i] < ticket_numbers[params->num] ||
            (ticket_numbers[i] == ticket_numbers[params->num] && i < params->num)));
    }
    params->res = curr_symb++;
    if (curr_symb > 'z')
        curr_symb = 'a';

    gettimeofday(&tv, NULL);

    millisec = lrint(tv.tv_usec / 1000.0);
    if (millisec >= 1000) {
        millisec -= 1000;
        tv.tv_sec++;
    }

    tm_info = localtime(&tv.tv_sec);
    char buffer2[26];
    strftime(buffer2, 26, "%H:%M:%S", tm_info);
    syslog(LOG_INFO, "[Worker %d][%s.%03d] finish, client num is %d, ticket %d, symbol %c", tid, buffer2, millisec, params->num, ticket_numbers[params->num], params->res);
    ticket_numbers[params->num] = 0;
    return 0;
}

struct BAKERY* get_num_1_svc(struct BAKERY* argp, struct svc_req* rqstp)
{
    static struct BAKERY  result;

    worker_params params;
    get_ticket(&params);
    result.num = params.num;
    if (params.num == -1)
        return(&result);

    worker_results[params.num].num = params.num;
    pthread_create(&worker_threads[params.num], NULL, bakery, &worker_results[params.num]);
    return &result;
}

struct BAKERY* get_res_1_svc(struct BAKERY* argp, struct svc_req* rqstp)
{
    static struct BAKERY  result;

    pthread_join(worker_threads[argp->num], NULL);
    result.res = worker_results[argp->num].res;

    return(&result);
}
