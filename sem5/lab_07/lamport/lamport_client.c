#include "lamport.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int num = 0;

void bakery_prog_1(char* host)
{
    struct BAKERY* get_num_res;
    struct BAKERY  get_num_arg;
    struct BAKERY* get_res_res;
    struct BAKERY  get_res_arg;

    CLIENT* clnt = clnt_create(host, BAKERY_PROG, BAKERY_VER, "udp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        exit(EXIT_FAILURE);
    }

    sleep(rand() % 4 + 1);
    get_num_res = get_num_1(&get_num_arg, clnt);
    if (get_num_res == NULL)
    {
        clnt_perror(clnt, "call failed:");
        exit(EXIT_FAILURE);
    }
    if (get_num_res->num == -1)
    {
        printf("[Client %d] Server is overloaded and denied my request!\n", getpid());
        exit(EXIT_FAILURE);
    }
    printf("[Client %d] Got num = %d\n", getpid(), get_num_res->num);
    num = get_num_res->num;

    sleep(rand() % 4 + 1);
    get_res_arg.num = num;
    get_res_res = get_res_1(&get_res_arg, clnt);
    if (get_res_res == NULL)
    {
        clnt_perror(clnt, "call failed:");
        exit(EXIT_FAILURE);
    }
    printf("[Client %d] result is <%c>\n", getpid(), get_res_res->res);
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