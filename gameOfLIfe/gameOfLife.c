#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

int **createMatrix(int rows, int collumns, int **matrix, const char *filePath)
{
    int **mat;
    mat = (int **)malloc(rows * sizeof(int *));
    for (int i = 0; i < rows; i++)
        mat[i] = (int *)malloc(collumns * sizeof(int));

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
void printToFile(int **matrix, int rows, int collumns, int episodes, double elapsedTime)
{
    char fileName[50];
    sprintf(fileName, "../statistics/statistics_gameOfLife_rows_%dxcolumns_%d_episodes_%d", rows, collumns, episodes);
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
    sprintf(totalElapsedTime, "\nTotal elapsed time:%f", elapsedTime);
    if (write(file, totalElapsedTime, strlen(totalElapsedTime)) != strlen(totalElapsedTime))
    {
        printf("Error while writing the elapsed time");
        exit(1);
    }
    close(file);
}
void **computeNextMatrix(int **matrix, int **nextMatrix, int rows, int collumns)
{

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < collumns; col++)
        {
            int neighbors = countNeighbors(matrix, row, col, rows, collumns);
            int state = matrix[row][col];
            if (state == 0 && neighbors == 0)
            {
                nextMatrix[row][col] = 1;
            }
            else if (state == 1 && neighbors < 2 || neighbors > 3)
            {
                nextMatrix[row][col] = 0;
            }
            else
            {
                nextMatrix[row][col] = state;
            }
        }
    }
}
void printMatrix(int **matrix, int rows, int collumns)
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

int countNeighbors(int **matrix, int row, int collumn, int rows, int collumns)
{
    int sum = 0;
    for (int i = -1; i < 2; i++)
    {
        for (int j = -1; j < 2; j++)
        {
            int computatedRow = (row + i + rows) % rows;
            int computatedCollumn = (collumn + j + collumns) % collumns;
            sum += matrix[computatedRow][computatedCollumn];
        }
    }
    sum -= matrix[row][collumn];
    return sum;
}

int main(int argc, char **argv)
{
    int **initialMatrix = NULL;
    int **computedMatrix;
    int row = atoi(argv[1]);
    int collumn = atoi(argv[2]);
    computedMatrix = (int **)malloc(sizeof(int *) * row);
    int episodes = atoi(argv[3]);
    for (int i = 0; i < row; i++)
    {
        computedMatrix[i] = (int *)malloc(sizeof(int) * collumn);
    }
    struct stat file_existence;
    if (stat(argv[4], &file_existence) != 0)
    {
        printf("No such file");
    }
    if (!S_ISREG(file_existence.st_mode))
    {
        printf("No .txt file");
    }
    initialMatrix = createMatrix(row, collumn, initialMatrix, argv[4]);

    int flag = 0;
    clock_t t;
    t = clock();
    for (int episode = 0; episode < episodes; episode++)
    {
        if (flag == 0)
        {
            computeNextMatrix(initialMatrix, computedMatrix, row, collumn);
            flag = 1;
        }
        else
        {
            computeNextMatrix(computedMatrix, initialMatrix, row, collumn);
            flag = 0;
        }
    }
    t = clock() - t;
    double time_taken = ((double)t) / CLOCKS_PER_SEC;

    int **finalMatrix;
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