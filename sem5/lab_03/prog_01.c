#include <stdio.h>

int main()
{
	printf("This program runs until input 0 or not a digit\n");
	int num;
	printf("Enter number: ");
	scanf("%d", &num);
	while (num != 0)
	{
		printf("Enter number: ");
		if (scanf("%d", &num) != 1)
			break;
	}
	printf("Terminating program...\n");
	return 0;
}
