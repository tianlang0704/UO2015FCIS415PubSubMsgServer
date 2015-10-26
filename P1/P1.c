#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
		long lPosI, lPosA, lArgSize, lArgN = 0;
		
		//Get program name
		fscanf(fIn, "%s", chPNameBuffer);
		SkipWhiteSpace(fIn);	


		printf("Program Name: %s........\n", chPNameBuffer);

		//Get argument list
		while(fgetc(fIn) != '\n' && !feof(fIn))
		{	
			if(!feof(fIn))
				fseek(fIn, -1, SEEK_CUR);
			
			//Expand argument list when out of space
			if(lArgN >= iArgMax)
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

		//Add to list and start running program	
		m_PidList[m_PCount] = fork();	
		if(m_PidList[m_PCount] < 0)
			perror("Failed forking");
		
		if(m_PidList[m_PCount] == 0)
		{
			if(execvp(chPNameBuffer, pchArgList) < 0)
				perror("Failed running program");
			_exit(0);
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

	int i, status;
	for(i = 0; i < m_PCount; i++)
		waitpid(m_PidList[i], &status, 0);

	return 0;
}
