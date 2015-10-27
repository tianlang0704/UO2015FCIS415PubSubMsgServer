#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

typedef enum {P_RUNNING, P_TERMINATED} P_Status;

typedef struct
{
	P_Status sta;
	pid_t pid;
	long time;
} PidTimePair;

int m_PCount = 0;
PidTimePair m_PList[255];

int WaitForSignal(int iSignal)
{
	int iSigNum;
	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, iSignal);
	sigprocmask(SIG_BLOCK, &sigset, NULL);
	if(sigwait(&sigset, &iSigNum))
	{
		perror("Failed sigwait");
		return 1;
	}

	return 0;
}

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

int StartP(const char* filePath, PidTimePair *PList,int *PCount)
{
	char chPNameBuffer[255];
	FILE* fIn = fopen(filePath, "rb");
	if(!fIn) perror("Failed opening file.");	
	int iArgMax = 10;
	char **pchArgList = malloc(iArgMax * sizeof(void*));
	(*PCount) = 0;

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
				iArgMax *= 2;
				pchArgList = realloc(pchArgList, 
						iArgMax * sizeof(char*));
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
		PList[*PCount].pid = fork();	
		if(PList[*PCount].pid < 0)
			perror("Failed forking");
	
		if(PList[*PCount].pid == 0)
		{
			perror("start waiting for signal.");
			WaitForSignal(SIGCONT);
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

		(*PCount)++;

		int j;
		for(j = 0; j < lArgN; j++)
			free(pchArgList[j]);

		usleep(100000);
	}

	fclose(fIn);
	free(pchArgList);

	return 0;
}

int CheckRunning(PidTimePair *PList, int PCount)
{
	int i, result = 0;
	for(i = 0; i < PCount; i++)
	{
		waitpid(-1, NULL, WNOHANG);
		if(PList[i].sta == P_RUNNING && kill(PList[i].pid, 0) < 0)
			PList[i].sta = P_TERMINATED;
		//**Should check errno too
		if(result == 0 && PList[i].sta == P_RUNNING)
			result = 1;
	}

	return result;
}

void Reschedule(PidTimePair *Target)
{
	Target->time = 30000;
}

void Run(PidTimePair *PList, int PCount)
{
	int PIndex = 0, i;

	for(i = 0; i < PCount; i++)
		PList[i].sta = P_RUNNING;

	while(CheckRunning(PList, PCount))
	{
		if(PList[PIndex].sta == P_RUNNING)
		{
			Reschedule(&PList[PIndex]);
			if(kill(PList[PIndex].pid, SIGCONT) < 0)
				perror("Failed sending SIGCONT");
			ualarm(PList[PIndex].time, 0);
			WaitForSignal(SIGALRM);
			if(kill(PList[PIndex].pid, SIGSTOP) < 0)
				perror("Failed sending SIGSTOP");
		}
	
		PIndex++;
		PIndex %= PCount;
	}
}

int main(int argc, const char* argv[])
{
	StartP(argv[1], m_PList, &m_PCount);

	Run(m_PList, m_PCount);

	return 0;
}
