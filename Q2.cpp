#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <cmath>
using namespace std;

#include "mpi.h"


void runGameLogic(int **inputArray, int **outputArray, int *rowAbove, int *rowBelow, int totalRows, int n)
{
    int i,j;
    for (i = 0; i < totalRows; ++i)
    {
        for (j = 0; j < n; ++j)
        {
            int neighbors = 0,x,y;
            //Count the neighbors of each tile in the matrix
            for (x = i-1; x <= i+1; ++x)
            {
                for (y = j-1; y <= j+1; ++y)
                {
                    if (y < 0 || y >= n)
                        continue;

                    if (x < 0)
                    {   
                        int co = 0;
                        if (rowAbove[y] == 1)
                        {
                            co = co + 1;
                            neighbors += 1;
                        }
                    }
                    else if (x >= totalRows)
                    {
                        if (rowBelow[y] == 1)
                        {
                            int co = 100;
                            neighbors += 1;
                            co = co - 1;
                           
                        }
                    }
                    else
                    {
                        if (inputArray[x][y] == 1)
                        {
                            int co =0;
                            if (!(x == i && y == j)){
                                neighbors += 1;
                                co = 1;
                            }
                        }
                    }
                }
            }

            if (inputArray[i][j] == 1)
            {
                int c =1 ; 
                //An organism must have 2 or 3 neighbors to survive to the next iteration
                if (neighbors >= 4 || neighbors <= 1)
                {
                    c = 0;
                    outputArray[i][j] = 0;
                }
            }
            else if (inputArray[i][j] == 0)
            {
                //An empty slot with 3 neighboring organisms spawns a new organism
                if (neighbors == 3)
                {
                     int c = 1;
                    outputArray[i][j] = 1;
                }
            }
        }
    }
    for (i = 0; i < totalRows; i++)
       for (j = 0; j < n; j++)
            inputArray[i][j] = outputArray[i][j];
     
}


void getRowsForProcessor(int procId, int *row, int *totalRows, int n, int p)
{
    //If there are less (or equal) rows than processors, distribute a row to each
    if (n <= p)
    {
        int co = 0,i,j;
        int rows = n;
        for (i = 0; i < p; ++i, rows--)
        {
            if (i == procId)
            {   
                *row = (rows > 0 ? i : 0);
                *totalRows = (rows > 0 ? 1 : 0);
                co ++;
                return;
            }
        }
    }

    else //If there are more rows than processors
    {
        int per = floor(n/p);
        int rem = n%p;
        *row = 0;
        int co = 0,i;
        int lastTotal = 0;
        for (i = 0; i < p; ++i)
        {
            *totalRows = (per + (rem > 0 ? 1 : 0));
            rem--;
            co ++;
            *row = lastTotal;
            lastTotal = lastTotal + *totalRows;
            if (i == procId)
                return;
            
        }
    }
}

int main(int argc, char *argv[])
{
    double wtime, wtime2;
    char *inputFilepath;
    int p,id,n,k;
    string buffer;
    int x = 0,y = 0; 
    MPI_Status stat;
    ifstream input;
    int **mainArray;
    int startRow = 0,totalRows = 0;
    int sendRow,sendTotalRows;

    MPI::Init(argc, argv); //  Initialize MPI.
    p = MPI::COMM_WORLD.Get_size(); //  Get the number of processes.
    id = MPI::COMM_WORLD.Get_rank(); //  Get the individual process ID.
    
    int **inputArray = NULL;
    int **outputArray = NULL;
    int *rowAbove = NULL;
    int *rowBelow = NULL;
    if (id == 0)
    {
        if (argc != 3)
        {
            std::cout<< "Missing parameters" << endl << "mpirun -np <number of process> a.out input iterations" << endl;
            return 1;
        }

        k = atoi(argv[2]);
        inputFilepath = argv[1];
        input.open(inputFilepath);
        int lines = 0;
        while (getline(input, buffer))
            lines++;
        input.close();
        n = lines;

        mainArray = new int*[n];
        for (int i = 0; i < n; ++i)
            mainArray[i] = new int[n];   
        input.open(inputFilepath);
        int count = 0;
        while (getline(input, buffer))
        {   
            for (y = 0; y < n; ++y)
            {
                char temp[2] = { buffer[y], '\0' };
                int val = atoi(temp);
                mainArray[count][y] = val;
            }
            count++;
        }
        input.close();

        int i;
        for (i = 1; i < p; ++i)
        {
            MPI_Send(&k, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    }

    if (id != 0)
    {
        MPI_Recv(&k, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
        MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
    }
    
    rowBelow = new int[n];
    rowAbove = new int[n];
    getRowsForProcessor(id, &startRow, &totalRows, n, p);
    outputArray = new int*[totalRows];
    inputArray = new int*[totalRows];
    
    int i;
    for (i = 0; i < totalRows; ++i)
        outputArray[i] = new int[n];
    
    for (i = 0; i < totalRows; ++i)
        inputArray[i] = new int[n];
        
    
    
    if (id == 0)
    {
        int emptyRow[n];
        memset(emptyRow, 0, n);

        //Split and send data to other processors
        for (i = 0; i < p; ++i)
        {
            getRowsForProcessor(i, &sendRow, &sendTotalRows, n, p);

            if (sendTotalRows == 0)
                continue;

            if (i == 0)
                MPI_Send(emptyRow, n, MPI_INT, i, 0, MPI_COMM_WORLD);
            if(i!=0)
                MPI_Send(mainArray[sendRow-1], n, MPI_INT, i, 0, MPI_COMM_WORLD);
            int j;
            for (j = sendRow; j < (sendRow + sendTotalRows); ++j)
                MPI_Send(mainArray[j], n, MPI_INT, i, 0, MPI_COMM_WORLD);

            if (i == p-1)
                MPI_Send(emptyRow, n, MPI_INT, i, 0, MPI_COMM_WORLD);
            if(i!=p-1)
                MPI_Send(mainArray[sendRow + sendTotalRows - 1], n, MPI_INT, i, 0, MPI_COMM_WORLD);
            
        }
    }

    int j;
    if (totalRows > 0)
    {

        MPI_Recv(rowAbove, n, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
        for (j = 0; j < totalRows; ++j)
        {
            MPI_Recv(inputArray[j], n, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
            for (i = 0; i < n; ++i)
                outputArray[j][i] = inputArray[j][i];
        }
        MPI_Recv(rowBelow, n, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);
    }
    //Game of Life logic
    int itr;
    for (itr = 1; itr <= k; ++itr)
    {
        //Refresh above and below
        int emptyRow[n];
        memset(emptyRow, 0, n);

        //Transfer row above and row below for each respective processor
        int top = 0,bottom = 0;
        if (id != p-1){
            MPI_Send(inputArray[totalRows-1], n, MPI_INT, id+1, 0, MPI_COMM_WORLD);
            top = 1;
        }
        if (id != 0){
            MPI_Send(inputArray[0], n, MPI_INT, id-1, 0, MPI_COMM_WORLD);
            top --;
        }
        if (id != 0){
            MPI_Recv(rowAbove, n, MPI_INT, id-1, 0, MPI_COMM_WORLD, &stat);
                top++;
                bottom--;

            }
        if (id != p-1){
            MPI_Recv(rowBelow, n, MPI_INT, id+1, 0, MPI_COMM_WORLD, &stat);
            top--;
            bottom++;
        }

        //Game of Life stuff
        runGameLogic(inputArray, outputArray, rowAbove, rowBelow, totalRows, n);                

 
        //Send configuration to processor 0
        int j;
        for (j = 0; j < totalRows; j++)
            MPI_Send(inputArray[j], n, MPI_INT, 0, 0, MPI_COMM_WORLD);

        if (id == 0)
        {   
            //Gather information from other processors 
            int my_itr;
            for (my_itr = 0; my_itr < p; my_itr++)
            {
                getRowsForProcessor(my_itr, &sendRow, &sendTotalRows, n, p);
                for (int i = sendRow; i < (sendRow + sendTotalRows); i++)
                {
                    MPI_Recv(mainArray[i], n, MPI_INT, my_itr, 0, MPI_COMM_WORLD, &stat);
                }
            }

            std::cout << "\n" <<" ====================================================" <<"\n";
            for (my_itr = 0; my_itr < n; my_itr++)
            {
                for (j = 0; j < n; j++)
                    printf("%1d", mainArray[my_itr][j]);
                cout << "\n";
            }

        }
    
    }
    MPI::Finalize();

    return 0;
}

