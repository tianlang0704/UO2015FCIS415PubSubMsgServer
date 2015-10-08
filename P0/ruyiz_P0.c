#include <stdio.h>

typedef enum { false, true } bool;
typedef enum {DT_INT, DT_DOUBLE, DT_STR, DT_N, DT_ERROR} DecodeType;

DecodeType m_dtType;
int m_iN = 0;

bool ParseHeader(FILE *pInFile)
{
	char chType;
	unsigned char ucN = 0;

	if (!fread(&chType, 1, 1, pInFile))
		return false;
	switch (chType)
	{
	case 'n':
		m_dtType = DT_N;
		break;
	case 'i':
		m_dtType = DT_INT;
		if (!fread(&ucN, 1, 1, pInFile))
		{
			fprintf(stderr, "Input error: no size\n");
			return false;
		}
		m_iN = ((int)ucN) + 1;
		break;
	case 'd':
		m_dtType = DT_DOUBLE;
		if (!fread(&ucN, 1, 1, pInFile))
		{
			fprintf(stderr, "Input error: no size\n");
			return false;
		}
		m_iN = ((int)ucN) + 1;
		break;
	case 's':
		m_dtType = DT_STR;
		if (!fread(&ucN, 1, 1, pInFile))
		{
			fprintf(stderr, "Input error: no size\n");
			return false;
		}
		m_iN = ((int)ucN) + 1;
		break;
	default:
		fprintf(stderr, "Input error: bad type\n");
		return false;
		break;
	}

	return true;
}

bool DecodeInt(FILE *pInFile, FILE *pOutFile)
{
	int iBuffer;
	int i;
	for (i = 0; i < m_iN; i++)
	{
		if (!fread(&iBuffer, 4, 1, pInFile))
		{
			fprintf(stderr, "Input error: not enough ints\n");
			return false;
		}
		fprintf(pOutFile, "%d ", iBuffer);
	}

	return true;
}

bool DecodeDouble(FILE *pInFile, FILE *pOutFile)
{
	double dBuffer;
	int i;
	for (i = 0; i < m_iN; i++)
	{
		if (!fread(&dBuffer, 8, 1, pInFile))
		{
			fprintf(stderr, "Input error: not enough doubles\n");
			return false;
		}
		fprintf(pOutFile, "%.10g ", dBuffer);
	}

	return true;
}

bool DecodeStr(FILE *pInFile, FILE *pOutFile)
{
	char chBuff[260];
	unsigned char ucStrSize;
	int iStrSize;

	int i;
	for (i = 0; i < m_iN; i++)
	{
		if (!fread(&ucStrSize, 1, 1, pInFile))
		{
			fprintf(stderr, "Input error: no string size\n");
			return false;
		}
		iStrSize = ((int)ucStrSize) + 1;

		if (iStrSize != fread(&chBuff, 1, iStrSize, pInFile))
		{
			fprintf(stderr, "Input error: not enough chars\n");
			return false;
		}
		chBuff[iStrSize] = 0;
		fprintf(pOutFile, "%s ", chBuff);
	}

	return true;
}

int main(int argc, char **argv )
{
	FILE *pInFile = fopen(argv[1], "rb");
	if (!pInFile)
	{
		fprintf(stderr, "Input error: failed opening the input file.\n");
		return 1;
	}
	FILE *pOutFile = fopen(argv[2], "wb");
	if (!pOutFile)
	{
		fprintf(stderr, "Input error: failed creating the output file.\n");
		return 1;
	}

	while(!feof(pInFile))
	{
		if(!ParseHeader(pInFile))
			break;

		switch (m_dtType)
		{
		case DT_INT:
			DecodeInt(pInFile, pOutFile);
			break;
		case DT_DOUBLE:
			DecodeDouble(pInFile, pOutFile);
			break;
		case DT_STR:
			DecodeStr(pInFile, pOutFile);
			break;
		case DT_N:
			fprintf(pOutFile, "\n");
		default:
			break;
		}
	}

	fclose(pInFile);
	fclose(pOutFile);
	return 0;
}