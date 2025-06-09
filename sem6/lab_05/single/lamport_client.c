#include "lamport.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int num = 0;

void bakery_prog_1(char* host)
{
    struct BAKERY* result_1;
    struct BAKERY  get_num_arg;
    struct BAKERY* result_2;
    struct BAKERY  get_res_arg;

    CLIENT* clnt = clnt_create(host, BAKERY_PROG, BAKERY_VER, "udp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        exit(EXIT_FAILURE);
    }

    sleep(rand() % 4 + 1);
    clock_t start_numb = clock();
    result_1 = get_num_1(&get_num_arg, clnt);
    if (result_1 == (struct BAKERY*) NULL)
    {
        clnt_perror(clnt, "call failed");
    }

    clock_t end_numb = clock();

    printf("[Client %d] Got num = %d\n", getpid(), result_1->num);
    printf("Number execution time: %f ms\n", (difftime(end_numb, start_numb)));
    num = result_1->num;

    sleep(rand() % 4 + 1);

    get_res_arg.num = result_1->num;
    get_res_arg.id = result_1->id;

    time_t start_res = clock();

    result_2 = get_res_1(&get_res_arg, clnt);
    if (result_2 == (struct BAKERY*) NULL) {
        clnt_perror(clnt, "call failed");
    }

    time_t end_res = clock();
    printf("[Client %d] result is <%c>\n", getpid(), result_2->res);
    printf("Res execution time: %f ms\n", (difftime(end_numb, start_numb) ));
    printf("Total execution time: %f ms\n", (difftime(end_numb, start_numb) + difftime(end_res, start_res)));
    clnt_destroy(clnt);
}


int main(int argc, char* argv[])
{
    char* host;

    if (argc < 2) {
        printf("usage: %s server_host\n", argv[0]);
        exit(1);
    }
    host = argv[1];
    bakery_prog_1(host);
}