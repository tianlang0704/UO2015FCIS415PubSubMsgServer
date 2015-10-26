#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, const char* argv[])
{
	int i;
	for(i = 0; i < 50; i++)
	{
		printf("%d\n", i);
		usleep(10000);

	}
	return 0;
}
