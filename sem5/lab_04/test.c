#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void sig_handler(int sig_num)
{
	signal(sig_num, sig_handler);
	printf("sig catch %d", sig_num);
}

int main()
{
	printf("This is the test\n");
	signal(SIGTERM, sig_handler);
	signal(SIGINT, SIG_IGN);
	signal(SIGSEGV, SIG_DFL);
	return 0;
}
