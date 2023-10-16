#include <stdio.h>

void draw()
{
    printf("\n");
    printf(" /\\_/\\ \n");
    printf("( o.o )\n");
    printf(" > ^ < \n");
    printf("\n");
}

int main()
{
    printf("This program draws a cat until input char 'c'\n");

    char ch;
    printf("Enter char: ");
    scanf("%c", &ch);

    while (ch != 'c')
    {
        draw();

        printf("Enter char: ");
        scanf(" %c", &ch);
    }

    printf("Entered 'c', terminating program...\n");

    return 0;
}

