#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define TIME_SLICE_STD 500000
#define TIME_SLICE_FACTOR_MIN 0.1


//enum to mark process status
typedef enum {P_RUNNING, P_TERMINATED} P_Status;
//struct for process system info
typedef struct
{
	float procCPUTime;
	float procDeltaCPUTime;
} PInfo;
//struct for process scheduling info
typedef struct
{
	P_Status sta;
	pid_t pid;
	PInfo info;
	long lastUpdateTime;
	long scheduledTime;
} PrcsCtrlBlk;
//Global variables for process creating and storing
//dynamic allocation is used since there is no way to 
//know the number of process there will be.
int m_PCount = 0;
int m_MaxCount = 10;
PrcsCtrlBlk *m_PList;


//Helper functions for manipulating process list
PrcsCtrlBlk * InitProcessList(int maxCount)
{
	PrcsCtrlBlk *pList = malloc(m_MaxCount * sizeof(PrcsCtrlBlk));
	memset(pList, 0, m_MaxCount * sizeof(PrcsCtrlBlk));
	return pList;
}

void UninitProcessList(PrcsCtrlBlk* pList)
{
	free(pList);
}

//Helper function for blocking process until signal received
//arg1: the signal to check
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
	sigprocmask(SIG_UNBLOCK, &sigset, NULL);

	return 0;
}

//Helper function for skipping empty line
//arg1: the file stream to read
void SkipEmptyLine(FILE *file)
{
	//Skip White space
	while(fgetc(file) == '\n' && !feof(file))
		continue;
	if(!feof(file))
		fseek(file, -1, SEEK_CUR);
}

//Helper function for skipping white space in file stream
//arg1: the file stream to read
void SkipWhiteSpace(FILE *file)
{
	//Skip White space
	while(fgetc(file) == ' ' && !feof(file))
		continue;
	if(!feof(file))
		fseek(file, -1, SEEK_CUR);
}

//Parse file and create processes from file
//arg1: the file to parse
//arg2: the global list for outputing
//arg3: the global counter for outputing
int StartP(PrcsCtrlBlk **pPList, int *pCount, int *maxCount)
{
	char chPNameBuffer[255];
	int iArgMax = 10;
	char **pchArgList = malloc(iArgMax * sizeof(void*));
	PrcsCtrlBlk *pList = (*pPList);
	(*pCount) = 0;

	//Read until eof
	while(!feof(stdin))
	{
		long lPosI, lPosA, lArgSize, lArgN = 1;
		
		//Get program name
		fscanf(stdin, "%s", chPNameBuffer);
		SkipWhiteSpace(stdin);	
		pchArgList[0] = malloc((strlen(chPNameBuffer) + 1) * sizeof(char));
		strcpy(pchArgList[0], chPNameBuffer);

		printf("Program: %s\n", chPNameBuffer);
		
		//Parse each argument in the list
		//using dynamic allocation because unix argument
		//can be super long
		while(fgetc(stdin) != '\n' && !feof(stdin))
		{	
			if(!feof(stdin))
				fseek(stdin, -1, SEEK_CUR);
			
			//Realloc argument list holder when out of space
			//+1 saving space for NULL termination
			if(lArgN + 1 >= iArgMax)
			{
				iArgMax *= 2;
				pchArgList = realloc(pchArgList, 
						iArgMax * sizeof(char*));
			}

			//Find out current argument size
			lPosI = ftell(stdin);
			fscanf(stdin, "%*s");
			lPosA = ftell(stdin);
			lArgSize = lPosA - lPosI;
			fseek(stdin, lPosI, SEEK_SET);		

			//Extract current argument
			pchArgList[lArgN] = malloc((lArgSize + 1) * sizeof(char));
			fscanf(stdin, "%s", pchArgList[lArgN]);
			pchArgList[lArgN][lArgSize] = 0;
			
			SkipWhiteSpace(stdin);
			lArgN++;
		}
		SkipWhiteSpace(stdin);
		SkipEmptyLine(stdin);
		pchArgList[lArgN] = NULL;

		//Add to process list and wait for a signal to run new program	
		pList[*pCount].pid = fork();	
		if(pList[*pCount].pid < 0)
			perror("Failed forking");
	
		if(pList[*pCount].pid == 0)
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

		int j;
		for(j = 0; j < lArgN; j++)
			free(pchArgList[j]);
		(*pCount)++;

		if((*pCount) > (*maxCount))
		{
			(*maxCount) *= 2;
			(*pPList) = realloc((*pPList), 
					(*maxCount) * sizeof(PrcsCtrlBlk));	
		}
	}

	fclose(stdin);
	free(pchArgList);

	return 0;
}

//Get info from system scheduler about a specific process
//arg1: Target process in the list
void UpdateProcInfo(PrcsCtrlBlk *pTarget)
{
	char buff[250];
	float oldCPUTime;
	
	pTarget->info.procDeltaCPUTime = 0;

	if(pTarget->sta == P_TERMINATED)
		return;

	sprintf(buff, "/proc/%d/sched", pTarget->pid);
	FILE *fPInfo = fopen(buff, "rb");
	if(!fPInfo)
	{
		perror("Failed reading process info");
		return;
	}
	//skiping first 13 terms to get total cpu time
	int i;
	for(i = 0; i < 13; i++)
		fscanf(fPInfo, "%*s");
	oldCPUTime = pTarget->info.procCPUTime;
	fscanf(fPInfo, "%f", &(pTarget->info.procCPUTime));
	pTarget->info.procDeltaCPUTime = pTarget->info.procCPUTime - oldCPUTime;

	fclose(fPInfo);
}

//Schedule according to system scheduler
//arg1: Program list
//arg2: Program count
void Reschedule(PrcsCtrlBlk *pList, PrcsCtrlBlk *pTarget, int pCount)
{
	float sumDeltaCPUTime = 0, pDeltaTime = 0, timeFactor = 0, deltaTimeAvg = 0;
	int i, runningCount = 0; 
	char buff[250];

	for(i = 0; i < pCount; i++)
	{
		UpdateProcInfo(&pList[i]);
		sumDeltaCPUTime += pList[i].info.procDeltaCPUTime;
		if(pList[i].sta == P_RUNNING) runningCount++;
	}

	if(sumDeltaCPUTime == 0 || pTarget->info.procDeltaCPUTime == 0)
	{
		timeFactor = TIME_SLICE_FACTOR_MIN;
	}
	else
	{	
		pDeltaTime = pTarget->info.procDeltaCPUTime;
		if(pDeltaTime / sumDeltaCPUTime < TIME_SLICE_FACTOR_MIN)
			timeFactor = TIME_SLICE_FACTOR_MIN;
		else
			timeFactor = pDeltaTime / sumDeltaCPUTime;
	}

	pTarget->scheduledTime = (long)(TIME_SLICE_STD * timeFactor);
	sprintf(buff, "Scheduling %d:\n   system delta cpu time %f jiffies,\n   scheduling for %ld micro seconds",
			pTarget->pid, 
			pTarget->info.procDeltaCPUTime, 
			pTarget->scheduledTime);
	perror(buff);

}

//Check and mark running processes in the list
//arg1: Program list
//arg2: Program count
//return: 0 if no program running, 1 if there is at least one running.
int CheckRunning(PrcsCtrlBlk *pList, int pCount)
{
	int i;
	pid_t terminatingPid;
	
	while((terminatingPid = waitpid(-1, NULL, WNOHANG)) > 0)
		for(i = 0; i < pCount; i++)
			if(pList[i].pid == terminatingPid)
				pList[i].sta = P_TERMINATED;	

	for(i = 0; i < pCount; i++)
		if(pList[i].sta == P_RUNNING)
			return 1;
	return 0;
}

//Run programs according to the list
//arg1: Program list
//arg2: Program count
void RunP(PrcsCtrlBlk *pList, int pCount)
{
	int pIndex = 0, i;

	for(i = 0; i < pCount; i++)
		pList[i].sta = P_RUNNING;

	//While any of the program is still running
	//Cycle through each running program and adaptively assign time and run
	while(CheckRunning(pList, pCount))
	{
		if(pList[pIndex].sta == P_RUNNING)
		{
			Reschedule(pList, &pList[pIndex], pCount);
			if(kill(pList[pIndex].pid, SIGCONT) < 0)
				perror("Failed sending SIGCONT");
			ualarm(pList[pIndex].scheduledTime, 0);
			WaitForSignal(SIGALRM);
			if(kill(pList[pIndex].pid, SIGSTOP) < 0)
				perror("Failed sending SIGSTOP");
		}
	
		pIndex++;
		pIndex %= pCount;
	}
}

int main(int argc, const char* argv[])
{
	m_PList = InitProcessList(m_MaxCount);

	StartP(&m_PList, &m_PCount, &m_MaxCount);
	RunP(m_PList, m_PCount);
	
	UninitProcessList(m_PList);
	return 0;
}
