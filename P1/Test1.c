#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, const char* argv[])
{
	
	int i, iCounter = atoi(argv[1]);
	for(i = 0; i < iCounter; i++)
	{
		printf("%d\n", i);
		usleep(10000);

	}
	return 0;
}
