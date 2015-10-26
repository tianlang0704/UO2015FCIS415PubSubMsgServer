#include <stdio.h>
#include <unistd.h>


int main(int argc, const char* argv[])
{
	int i;
	for(i = 0; i < 500; i++)
	{
		printf("%d\n", i);
		usleep(10000);

	}
	return 0;
}
