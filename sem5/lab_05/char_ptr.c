#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	char* a;
	char** b;
	char** c;
	a = malloc(sizeof(char));
	*b = a; // +sizeof(char*);
	printf("a = %p\n", (void *)a);
	printf("b = %p\n", (void *)b);
}