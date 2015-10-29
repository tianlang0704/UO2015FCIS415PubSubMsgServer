// program3.c
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char * argv[])
{
    int *a;

    a = calloc(11, sizeof(int));//Only allocated 10 ints but using 11 
    
    for(int i=0;i <= 10; i++)
    {
        a[i] = i;
    }
    for(int i=0;i <= 10; i++)
    {
        printf("%d\n", a[i]);
    }

    free(a);//And it didn't free the memory either.
}
