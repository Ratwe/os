#include "lamport.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

int num = 0;
int id = 0;

void bakery_prog_1(char* host)
{
    struct BAKERY* get_num_res;
    struct BAKERY  get_num_arg;
    struct BAKERY* wait_res_res;
    struct BAKERY  wait_res_arg;
    struct BAKERY* res_res;
    struct BAKERY res_arg;

    CLIENT* clnt = clnt_create(host, BAKERY_PROG, BAKERY_VER, "udp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        exit(EXIT_FAILURE);
    }

    sleep(rand() % 4 + 1);
    clock_t start_numb = clock();
    get_num_res = get_num_1(&get_num_arg, clnt);
    clock_t end_numb = clock();
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
    // printf("[Client %d] Got id = %d\n", getpid(), get_num_res->id);
    printf("Number execution time: %f ms\n", difftime(end_numb, start_numb));
    num = get_num_res->num;
    id = get_num_res->id;

    //sleep(rand() % 4 + 1);
    wait_res_arg.num = num;
    wait_res_arg.id = id;
    clock_t start_wait = clock();
    wait_res_res = wait_res_1(&wait_res_arg, clnt);
    clock_t end_wait = clock();
    if (wait_res_res == NULL)
    {
        clnt_perror(clnt, "call failed:");
        exit(EXIT_FAILURE);
    }
    printf("[Client %d] wait result is <%c>\n", getpid(), wait_res_res->symb);
    printf("Wait execution time: %f ms\n", difftime(end_wait, start_wait));
    
    sleep(rand() % 4 + 1);
    res_arg.id = id;
    res_arg.num = num;
    clock_t start_res = clock();
    res_res = get_res_1(&res_arg, clnt);
    clock_t end_res = clock();
    if (res_res == NULL)
    {
        clnt_perror(clnt, "call failed:");
        exit(EXIT_FAILURE);
    }
    printf("[Client %d] res is <%c>\n", getpid(), res_res->symb);
    printf("Res execution time: %f ms\n", difftime(end_res, start_res));
    printf("Total execution time: %f ms\n", difftime(end_numb, start_numb) + difftime(end_wait, start_wait) + difftime(end_res, start_res));
    
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