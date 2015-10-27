#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int m_PCount = 0;
pid_t m_PidList[255];

void SkipNewLine(FILE *file)
{
	//Skip White space
	while(fgetc(file) == '\n' && !feof(file))
		continue;
	if(!feof(file))
		fseek(file, -1, SEEK_CUR);
}

void SkipWhiteSpace(FILE *file)
{
	//Skip White space
	while(fgetc(file) == ' ' && !feof(file))
		continue;
	if(!feof(file))
		fseek(file, -1, SEEK_CUR);
}

int StartP(const char* filePath)
{
	char chPNameBuffer[255];
	FILE* fIn = fopen(filePath, "rb");
	if(!fIn) perror("Failed opening file.");
	
	int iArgMax = 10;
	char **pchArgList = malloc(iArgMax * sizeof(void*));

	while(!feof(fIn))
	{
		long lPosI, lPosA, lArgSize, lArgN = 1;
		
		//Get program name
		fscanf(fIn, "%s", chPNameBuffer);
		SkipWhiteSpace(fIn);	
		pchArgList[0] = malloc((strlen(chPNameBuffer) + 1) * sizeof(char));
		strcpy(pchArgList[0], chPNameBuffer);

		printf("Program: %s\n", chPNameBuffer);
		
		//Get argument list
		while(fgetc(fIn) != '\n' && !feof(fIn))
		{	
			if(!feof(fIn))
				fseek(fIn, -1, SEEK_CUR);
			
			//Expand argument list when out of space
			//+1 saving space for NULL termination
			if(lArgN + 1 >= iArgMax)
			{
				int iOldMax = iArgMax;
				char **pchOldList = pchArgList;
				iArgMax *= 2;
				pchArgList = malloc(iArgMax * sizeof(void*));
				memcpy(pchArgList, pchOldList, iOldMax);
				free(pchOldList);
			}

			//Find out current argument size
			lPosI = ftell(fIn);
			fscanf(fIn, "%*s");
			lPosA = ftell(fIn);
			lArgSize = lPosA - lPosI;
			fseek(fIn, lPosI, SEEK_SET);		

			//Extract current argument
			pchArgList[lArgN] = malloc((lArgSize + 1) * sizeof(char));
			fscanf(fIn, "%s", pchArgList[lArgN]);
			pchArgList[lArgN][lArgSize] = 0;
			
			SkipWhiteSpace(fIn);

			lArgN++;
		}
		SkipWhiteSpace(fIn);
		SkipNewLine(fIn);
		pchArgList[lArgN] = NULL;

		//Add to list and start running program	
		m_PidList[m_PCount] = fork();	
		if(m_PidList[m_PCount] < 0)
			perror("Failed forking");
	
		if(m_PidList[m_PCount] == 0)
		{
			perror("start waiting for signal.");
			int iSig;
			sigset_t sigset;
			sigemptyset(&sigset);
			sigaddset(&sigset, SIGUSR1);
			sigprocmask(SIG_BLOCK, &sigset, NULL);
			if(sigwait(&sigset, &iSig))
				perror("Failed sigwait");
			perror("Signal reived.");

			if(execvp(chPNameBuffer, pchArgList) < 0)
			{
				perror("Failed running program");
				_exit(1);
			}
			else
			{
				_exit(0);
			}
		}

		m_PCount++;

		int j;
		for(j = 0; j < lArgN; j++)
			free(pchArgList[j]);
	}

	fclose(fIn);
	free(pchArgList);

	return 0;
}


int main(int argc, const char* argv[])
{
	StartP(argv[1]);

	printf("Sending SIGUSR1 to programs\n");
	int i;
	for(i = 0; i < m_PCount; i++)
		if(kill(m_PidList[i], SIGUSR1) < 0)
			perror("Failed sending SIGUSR1");
	usleep(50000);

	printf("Sending SIGSTOP to programs\n");
	for(i = 0; i < m_PCount; i++)
		if(kill(m_PidList[i], SIGSTOP) < 0)
			perror("Failed sending SIGSTOP");
	usleep(5000000);
	
	printf("Sending SIGCONT to programs\n");
	for(i = 0; i < m_PCount; i++)
		if(kill(m_PidList[i], SIGCONT) < 0)
			perror("Failed sending SIGCONT");

	int iStatus;
	for(i = 0; i < m_PCount; i++)
	{
		printf("main waiting: %d\n", i);
		waitpid(m_PidList[i], &iStatus, 0);
	}
	return 0;
}
