#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, const char* argv[])
{
	
	int i, iCounter = atoi(argv[1]), 
	    pid = getpid(), iInter = atoi(argv[2]);
	for(i = 0; i < iCounter; i++)
	{
		printf("Pid:%d, %d\n", pid, i);
		usleep(iInter);

	}
	return 0;
}
