#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "mpi.h"

void populate(int dimention, int **Matrix, char *filename, int transpose){
    FILE *file;
    file = fopen(filename,"r");
    if(file == NULL){
        perror("Error");
    }
    else {
        fscanf(file,"%d",&dimention);
        for (int i = 0; i < dimention; i++){
            for(int j = 0; j < dimention; j++){
                if(transpose == 0){
                    if(!fscanf(file, "%d",&Matrix[i][j]));
                }
                else {
                    if(!fscanf(file, "%d",&Matrix[j][i]));
                }
            }
        }
    fclose(file);
    }
}


int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes
    int nTasks;
    MPI_Comm_size(MPI_COMM_WORLD, &nTasks);

    //matrix dimention
    int dimention;
    FILE *file;
    char *filename = argv[1];
    file = fopen(filename,"r");
    if(file == NULL){
        perror("Error");
    }
    else {
        fscanf(file,"%d",&dimention);
    }

    //number of rows to be sent
    int n = dimention/sqrt(nTasks);
    int start_row = 0;
    int start_col = 0;


    // Get the rank of the process
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int **A = malloc( dimention*sizeof(int*));
    for(int i = 0; i < dimention; i++){ 
        A[i] = malloc( dimention*sizeof(int));
    }

    int **B = malloc( dimention*sizeof(int*));
    for(int i = 0; i < dimention; i++){ 
        B[i] = malloc( dimention*sizeof(int));
    }

    int **C = malloc( n*sizeof(int*));
    for(int i = 0; i < n; i++){ 
        C[i] = malloc( n*sizeof(int));
    }

    for(int i=0; i<n; i++){
        for(int j=0; j<n; j++){
            C[i][j] = 0;
        }
    }

    if(rank == 0){
        populate(dimention,A,argv[1],0);
        populate(dimention,B,argv[2],1);

        //Send lines from A and B to other tasks
        for(int i = 1; i < nTasks; i++){
            //which line should I start sending? Depends on to which task I am sending.
            if(i%((int)sqrt(nTasks)) == 0){
                start_row += n;
            }
            //Sending n lines from A starting on line "start_row"
            for(int k = 0;k<n;k++){
                MPI_Send(&A[start_row+k][0],dimention,MPI_INT,i,1,MPI_COMM_WORLD);
            }
            //Sending start_row
            MPI_Send(&start_row,1,MPI_INT,i,2,MPI_COMM_WORLD);

            //For B the start_col goes from 0 to n and then goes back to 0
            if(i%((int)sqrt(nTasks)) == 0){
                start_col = 0;
            }

            else {
                start_col += n;
            }

            for(int k = 0;k<n;k++){
                MPI_Send(&B[start_col + k][0],dimention,MPI_INT,i,3,MPI_COMM_WORLD);
            }

            //Sending start_col
            MPI_Send(&start_col,1,MPI_INT,i,4,MPI_COMM_WORLD);
        }
    }


    if(rank != 0){
        //receiving A and start_row from task 0
        for(int k=0; k < n; k++){
            MPI_Recv(&A[k][0],dimention,MPI_INT,0,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
        MPI_Recv(&start_row,1,MPI_INT,0,2,MPI_COMM_WORLD,MPI_STATUS_IGNORE);

        for(int k=0; k < n; k++){
            MPI_Recv(&B[k][0],dimention,MPI_INT,0,3,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
        MPI_Recv(&start_col,1,MPI_INT,0,4,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    }

    //Perform product (including task 0)
    for(int i=0; i<n; i++){
        for(int j=0; j<n; j++){
            for(int k=0; k<dimention; k++){
                C[i][j] += A[i][k] * B[j][k];
            }
            //printf("%d ",B[i][j]);
        }
        //printf("\n");
    }

    
    if(rank != 0){
        //sending result to task 0
        MPI_Send(&start_row,1,MPI_INT,0,5,MPI_COMM_WORLD);
        MPI_Send(&start_col,1,MPI_INT,0,6,MPI_COMM_WORLD);
        for (int i=0; i<n; i++){
            MPI_Send(&C[i][0],n,MPI_INT,0,7,MPI_COMM_WORLD);
        }
    }

    if(rank == 0){
        for(int task=1; task < nTasks; task++){
            MPI_Recv(&start_row,1,MPI_INT,task,5,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            MPI_Recv(&start_col,1,MPI_INT,task,6,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            for(int i=0;i<n;i++){
                MPI_Recv(&A[start_row + i][start_col],n,MPI_INT,task,7,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            }
        }

        for(int i=0; i<n; i++){
            for(int j=0; j<n; j++){
                A[i][j] = C[i][j];
            }
        }

        //write matrix to a file
        FILE *file_out;
        char *filename_out = argv[3];
        file_out = fopen(filename_out,"w+");
        if(file_out == NULL){
            perror("Error");
        }
        else {
            fprintf(file_out,"%d \n",dimention);
            for(int i=0; i<dimention; i++) {
                for(int j=0;j<dimention;j++) {
                    fprintf(file_out," %d\t",A[i][j]);
                }
                fprintf(file_out,"\n");
            }
        }
    }

    for (int i = 0; i < dimention; ++i) {
        free(A[i]);
    }
    free(A);

    for (int i = 0; i < dimention; ++i) {
        free(B[i]);
    }
    free(B);

    for (int i = 0; i < n; ++i) {
        free(C[i]);
    }
    free(C);

    MPI_Finalize();
    return 0;
}