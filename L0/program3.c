#include <stdio.h>
#include <stdlib.h>

int main(int argc, char * argv[])
{
	int *a;
	int i;

	a = calloc(10, sizeof(10));

	for(i = 0; i < 10; i++)
	{
		a[i] = i;
	}
	for(i = 0; i < 10; i++)
	{
		printf("%d\n", a[i]);
	}

	free(a);
	return 0;
}
