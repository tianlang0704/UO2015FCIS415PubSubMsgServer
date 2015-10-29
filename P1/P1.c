#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define STD_TIME_SLICE 50000

typedef enum {P_RUNNING, P_TERMINATED} P_Status;

typedef struct
{
	float CPUTime;
} PInfo;

typedef struct
{
	P_Status sta;
	pid_t pid;
	PInfo info; 
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

//Get info from system scheduler about a specific process
//arg1: Target process in the list
void GetInfo(PidTimePair *Target)
{
	char buff[250];

	sprintf(buff, "/proc/%d/sched", Target->pid);
	FILE *fPInfo = fopen(buff, "rb");
	if(!fPInfo)
	{
		Target->info.CPUTime = -1;
		return;
	}

	int i;
	for(i = 0; i < 10; i++)
		fscanf(fPInfo, "%*s");
	fscanf(fPInfo, "%f", &Target->info.CPUTime);
	
	char str[250];
	sprintf(str, "P%d: %f",
			Target->pid, Target->info.CPUTime);
	perror(str);

	fclose(fPInfo);
}

//Schedule according to system scheduler time
//assign time slice from 1/5 to 5 times
//arg1: Program list
//arg2: Program count
void Reschedule(PidTimePair *PList, int PCount)
{
	float TimeSum = 0, PTime = 0, TimeFactor = 0, TimeAvg = 0;
	int i; 
	for(i = 0; i < PCount; i++)
	{
		GetInfo(PList + i);
		if(PList[i].info.CPUTime > 0)
			TimeSum += PList[i].info.CPUTime;
	}
	TimeAvg = TimeSum / PCount; 

	for(i = 0; i < PCount; i++)
	{
		PTime = PList[i].info.CPUTime;
		if(TimeSum == -1)
		{		
			TimeFactor = 0;
		}
		if(TimeSum == 0 || PTime == 0)
		{
			TimeFactor = 1;
		}
		else
		{
			if(TimeAvg / PTime > 5)
				TimeFactor = 0.2;
			else if(PTime / TimeAvg > 5)
				TimeFactor = 5;
			else
				TimeFactor = PTime / TimeAvg;
		}

		PList[i].time = STD_TIME_SLICE * TimeFactor;
	}
}

//Check and mark running programs in the list
//arg1: Program list
//arg2: Program count
//return: 0 if no program running, 1 if there is at least one running.
int CheckRunning(PidTimePair *PList, int PCount)
{
	int i, result = 0;
	for(i = 0; i < PCount; i++)
	{
		waitpid(-1, NULL, WNOHANG);
		if(PList[i].sta == P_RUNNING && kill(PList[i].pid, 0) < 0)
			PList[i].sta = P_TERMINATED;

		if(result == 0 && PList[i].sta == P_RUNNING)
			result = 1;
	}

	return result;
}

//Run programs according to the list
//arg1: Program list
//arg2: Program count
void RunP(PidTimePair *PList, int PCount)
{
	int PIndex = 0, i;

	for(i = 0; i < PCount; i++)
		PList[i].sta = P_RUNNING;

	//While any of the program is still running
	//Cycle through each running program and adaptively assign time
	while(CheckRunning(PList, PCount))
	{
		if(PList[PIndex].sta == P_RUNNING)
		{
			Reschedule(PList, PCount);
			if(kill(PList[PIndex].pid, SIGCONT) < 0)
				perror("Failed sending SIGCONT");
			char buff[250];
			sprintf(buff, "P%d scheduled time: %ld", 
					PList[PIndex].pid, PList[PIndex].time);
			perror(buff);
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

	RunP(m_PList, m_PCount);

	return 0;
}
