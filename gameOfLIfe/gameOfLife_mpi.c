#include <mpi.h>
#include <malloc.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#pragma warning(disable : 4996)
#define Master 0


typedef struct MatrixDetails {
	int row;
	int col;
	int change;
} MatrixDetails;

 
 
int  nrTasks, ID;
 

int countNeighbors(int** finalMatrix, int row, int collumn, int rows, int collumns)
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
			if (finalMatrix[i][j] == 1)
			{
				count++;
			}
		}
	}
	return count;
}
void computeNextMatrix(int row, int collumn, int* changes, int **finalMatrix, MatrixDetails* secondMatrix, int rows,int collumns) {
	int count = countNeighbors(finalMatrix, row, collumn, rows, collumns);

	if (finalMatrix[row][collumn] == 1 && (count > 3 || count < 2)) {
 
		secondMatrix[*changes].row = row;
		secondMatrix[*changes].col = collumn;
		secondMatrix[*changes].change = 0;
		(*changes)++;
	}

	if ((finalMatrix[row][collumn] == 0) && (count == 3)) {
 
		secondMatrix[*changes].row = row;
		secondMatrix[*changes].col = collumn;
		secondMatrix[*changes].change = 1;
		(*changes)++;
	}

}


void printToFile(int** finalMatrix, int rows, int collumns, int episodes, double elapsedTime)
{
	char fileName[100];
	sprintf(fileName, "../statistics/statistics_gameOfLife_MPI_rows_%dxcolumns_%d_episodes_%d", rows, collumns, episodes);
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
			if (finalMatrix[row][collumn] == 0)
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
	sprintf(totalElapsedTime, "\nTotal elapsed time:%f", elapsedTime);
	if (write(file, totalElapsedTime, strlen(totalElapsedTime)) != strlen(totalElapsedTime))
	{
		printf("Error while writing the elapsed time");
		exit(1);
	}
	close(file);
}


void printMatrix(int **finalMatrix, int rows, int collumns) {
	int i, j;

	for (i = 0; i < rows; i++) {
		for (j = 0; j < collumns; j++)
			printf("%i ", finalMatrix[i][j]);
		printf("\n");
	}
}
 

int main(int argc, char* argv[]) {

	 
 

 MPI_Init(&argc, &argv);
 


	//Get mpi info
	MPI_Comm_size(MPI_COMM_WORLD, &nrTasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &ID);
 
	 
	int episodes, rows, col;
	
	rows = atoi(argv[1]);
	col = atoi(argv[2]);
	episodes = atoi(argv[3]);

	int i;
	int j;
	int k;
	int** finalMatrix = NULL;
	//Allocate memory for the finalMatrix
	finalMatrix = malloc(rows * sizeof(int*));
	for (i = 0; i < rows; i++) finalMatrix[i] = malloc(col * sizeof(int));
	
	//creating the finalMatrix
	char element;
	int reader;
	int file = open(argv[4], O_RDONLY);
	if (file == -1)
	{
		printf("Nothing to open");
		exit(1);
	}
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < col; j++)
		{
			reader = read(file, &element, 1);
			if (reader == -1)
			{
				printf("Nothing to read");
				exit(1);
			}
			if (element == '0')
			{
				finalMatrix[i][j] = 0;
			}
			else
			{
				finalMatrix[i][j] = 1;
			}
		}
	}
	if (close(file) != 0)
	{
		printf("The file couldn't be closed");
		exit(1);
	}

	MatrixDetails* secondMatrix;
	MatrixDetails* auxiliarMatrix;
	secondMatrix = malloc(rows * col * sizeof(MatrixDetails));
	auxiliarMatrix = malloc(rows * col * sizeof(MatrixDetails));
	int* changeCounter = malloc(nrTasks * sizeof(int));
	int* disps = malloc(nrTasks * sizeof(int));
	int modRowTask, modColTask;
	int size = rows * col; //total value of the matrix we analyze
	int taskSize = size / nrTasks; // how much a task will analyze at the time
	int firstRowTask = taskSize * ID / col; 
	int firstColTask = taskSize * ID % col; 

	//check if we need data from other processes
	if (ID != nrTasks) {  
		modRowTask = taskSize * (ID + 1) / col;
		modColTask = taskSize * (ID + 1) % col;
 
	}
	else {
		modRowTask = rows - 1; 
		modColTask = col;
	}
	
	int changes, count, disp;
	clock_t t;
	t = clock();
	for (int episode = 0; episode < episodes; episode++) {
		count = 0;
		changes = 0;
		
		for (j = firstColTask; j < col && firstRowTask != modRowTask; j++) computeNextMatrix(firstRowTask, j, &changes ,finalMatrix, secondMatrix, rows,col);
		for (j = 0; j < modColTask && firstRowTask != modRowTask; j++) computeNextMatrix(modRowTask, j, &changes,finalMatrix, secondMatrix, rows, col);
		for (j = firstColTask; j < modColTask && firstRowTask == modRowTask; j++) computeNextMatrix(firstRowTask, j, &changes,finalMatrix, secondMatrix, rows, col);
		for (j = firstRowTask + 1; j < modRowTask; j++)
			for (k = 0; k < col; k++)
				computeNextMatrix(j, k, &changes, finalMatrix, secondMatrix, rows, col);
 
		MPI_Allgather(&changes, 1, MPI_INT,
			changeCounter, 1, MPI_INT,
			MPI_COMM_WORLD);

		disp = 0;

		for (j = 0; j < nrTasks; j++) {
			disps[j] = disp;
			count += changeCounter[j];
			changeCounter[j] *= 3;
			disp += changeCounter[j];
		}
		MPI_Allgatherv(secondMatrix, 3 * changes, MPI_INT,
			auxiliarMatrix, changeCounter, disps,
			MPI_INT, MPI_COMM_WORLD);
 
		for (j = 0; j < count; j++) {
			if (auxiliarMatrix[j].change == 0) finalMatrix[auxiliarMatrix[j].row][auxiliarMatrix[j].col] = 0;
			if (auxiliarMatrix[j].change == 1) finalMatrix[auxiliarMatrix[j].row][auxiliarMatrix[j].col] = 1;
		}
	}

	if (ID == 0) {
		t = clock() - t;
		double timeTaken = ((double)t) / CLOCKS_PER_SEC; 
		 
		//printMatrix(finalMatrix,rows,col);
		printToFile(finalMatrix, rows, col, episodes, timeTaken);
}
	MPI_Finalize();
}

 
 