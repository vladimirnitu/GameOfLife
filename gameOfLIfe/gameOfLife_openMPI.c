#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#pragma warning(disable : 4996)
#include <omp.h>

int** createMatrix(int rows, int collumns, int** matrix, const char* filePath)
{
	int** mat;
	mat = (int**)malloc(rows * sizeof(int*));
	for (int i = 0; i < rows; i++)
		mat[i] = (int*)malloc(collumns * sizeof(int));

	char element;
	int reader;
	int file = open(filePath, O_RDONLY);
	if (file == -1)
	{
		printf("Nothing to open");
		exit(1);
	}
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < collumns; j++)
		{
			reader = read(file, &element, 1);
			if (reader == -1)
			{
				printf("Nothing to read");
				exit(1);
			}
			if (element == '0')
			{
				mat[i][j] = 0;
			}
			else
			{
				mat[i][j] = 1;
			}
		}
	}
	if (close(file) != 0)
	{
		printf("The file couldn't be closed");
		exit(1);
	}
	return mat;
}
int countNeighbors(int** a, int row, int collumn, int rows, int collumns)
{
	int i, j, count = 0;
	for (i = row - 1; i <= row + 1; i++)
	{
		for (j = collumn - 1; j <= collumn + 1; j++)
		{
			if ((i == row && j == collumn) || (i < 0 || j < 0) || (i >= rows || j >= collumns))
			{
				continue;
			}
			if (a[i][j] == 1)
			{
				count++;
			}
		}
	}
	return count;
}
void printToFile(int** matrix, int rows, int collumns, int episodes, double elapsedTime)
{
	char fileName[100];
	sprintf(fileName, "../statistics/statistics_gameOfLifeOpenMPI_rows_%dxcolumns_%d_episodes_%d", rows, collumns, episodes);
	int file = open(fileName, O_WRONLY | O_CREAT, S_IWRITE);

	if (file == -1)
	{
		printf("Error opening file with the write privileague");
		exit(1);
	}
	char dead = '0';
	char alive = '1';

	int row, collumn;

	for (row = 0; row < rows; row++)
	{
		for (collumn = 0; collumn < collumns; collumn++)
		{
			if (matrix[row][collumn] == 0)
			{
				if (write(file, &dead, 1) != 1)
				{
					printf("Error writing to file");
					exit(1);
				}
			}
			else if (write(file, &alive, 1) != 1)
			{
				printf("Error writing to file");
				exit(1);
			}
		}
	}
	char totalElapsedTime[40];
	sprintf(totalElapsedTime, "\nTotal time:%f", elapsedTime);
	if (write(file, totalElapsedTime, strlen(totalElapsedTime)) != strlen(totalElapsedTime))
	{
		printf("Error while writing the elapsed time");
		exit(1);
	}
	close(file);
}
void** computeNextMatrix(int** a, int** b, int row, int col, int chunck, int threads)
{
	int i, j, neighbours;
	 
#pragma omp parallel default(shared) private(i, j, neighbours) num_threads(threads)
	{
#pragma omp for schedule (dynamic, chunck)
		for (i = 0; i < row; i++)
		{
			for (j = 0; j < col; j++)
			{
				neighbours = countNeighbors(a, i, j, row, col);
				if (a[i][j] == 1 && (neighbours < 2 || neighbours > 3))
				{
					b[i][j] = 0;
				}

				else if (a[i][j] == 0 && neighbours == 3)
				{
					b[i][j] = 1;
				}

			}
		}
	}
}
void printMatrix(int** matrix, int rows, int collumns)
{

	for (int row = 0; row < rows; row++)
	{
		for (int columns = 0; columns < collumns; columns++)
		{
			printf("%d", matrix[row][columns]);
		}
		printf("\n");
	}
}
int main(int argc, char** argv)
{

	int** initialMatrix = NULL;
	int** computedMatrix;
	int row = atoi(argv[1]);
	int collumn = atoi(argv[2]);
	computedMatrix = (int**)malloc(sizeof(int*) * row);
	int episodes = atoi(argv[3]);
	
	for (int i = 0; i < row; i++)
	{
		computedMatrix[i] = (int*)malloc(sizeof(int) * collumn);
	}
	struct stat file_existence;
	initialMatrix = createMatrix(row, collumn, initialMatrix, argv[4]);
	int chunck = atoi(argv[5]);
	int threads = atoi(argv[6]);
	int flag = 0;
	clock_t t;
	t = clock();
	for (int episode = 0; episode < episodes; episode++)
	{
		if (flag == 0)
		{
			computeNextMatrix(initialMatrix, computedMatrix, row, collumn, chunck, threads);
			flag = 1;
		}
		else
		{
			computeNextMatrix(computedMatrix, initialMatrix, row, collumn, chunck, threads);
			flag = 0;
		}
	}
	t = clock() - t;
	double time_taken = ((double)t) / CLOCKS_PER_SEC;

	int** finalMatrix;
	if (flag == 0)
	{
		finalMatrix = initialMatrix;
	}
	else
	{
		finalMatrix = computedMatrix;
	}

	printMatrix(finalMatrix, row, collumn);
	printToFile(finalMatrix, row, collumn, episodes, time_taken);
	return 0;

}