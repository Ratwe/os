#include "lamport.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "syslog.h"
#include "memory.h"
#include "stdlib.h"
#include <math.h>
#include <stdbool.h>

#define MAX_CLIENTS 16

bool_t choosing[MAX_CLIENTS] = { FALSE };
int ticket_numbers[MAX_CLIENTS] = { 0 };

int curr_id = 0;
char curr_symb = 'a';

struct BAKERY* get_num_1_svc(struct BAKERY* argp, struct svc_req* rqstp)
{
    static struct BAKERY result;
    int i = curr_id;
    curr_id++;
    choosing[i] = TRUE;
    int max = 0;
    for (int j = 0; j < MAX_CLIENTS; j++)
        if (ticket_numbers[j] > max)
            max = ticket_numbers[j];
    ticket_numbers[i] = max + 1;
    result.id = i;
    result.num = ticket_numbers[i];
    choosing[i] = false;
    
    return &result;
}

struct BAKERY* get_res_1_svc(struct BAKERY* argp, struct svc_req* rqstp) {
    static struct BAKERY result;
    int i = argp->id;
    result.id = i;
    result.num = argp->num;

    time_t start = clock();
    time_t end;

    for (int j = 0; j < MAX_CLIENTS; j++) {
        while (choosing[j]);
        while ((ticket_numbers[j] > 0) && (ticket_numbers[j] < ticket_numbers[i] || (ticket_numbers[j] == ticket_numbers[i] && j < i))) {
            end = clock();
            if ((end - start) / CLOCKS_PER_SEC > 1) {
                ticket_numbers[i] = 0;
                return NULL;
            }
        }
    }

    result.res = curr_symb;
    curr_symb++;

    ticket_numbers[i] = 0;

    return &result;
}

struct BAKERY*
    get_service_1_svc(struct BAKERY* argp, struct svc_req* rqstp) {
    return NULL;
}
