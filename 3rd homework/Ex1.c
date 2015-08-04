#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"


int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes
    int p;
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    // Get the rank of the process
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Request *request;

    //Initialize variables
    int N = 10000000;             //vector size
    int n = N/p;                  //vector portion to be sent to each task
                                  //don't forget that N%p != 0 might occur
    double st1, st2, stmin=10e8;     //Time before(current), after(current) and min(seen) for scatter
    double srt1, srt2, srtmin=10e8;  //Time before(current), after(current) and min(seen) for send/reciv
    double tmin;
    int NTests = 100;             //Number of tests
    int s = 0;
    int t = 0;

    int *A, *B, *C, *D;

    if (rank == 0){
        // Allocating vectors
        A = (int*) malloc(N * sizeof(int));
        B = (int*) malloc(N * sizeof(int));


        srand(time(NULL));
        //populating vectors
        for (int i = 0; i < N; i++){
            A[i] = rand() % 1000;
            B[i] = rand() % 1000;
        }
    }

    //Allocating C and D
    C = (int*) malloc(n * sizeof(int));
    D = (int*) malloc(n * sizeof(int));

    for(int k = 0; k< NTests; k++){
        if(N%p == 0){
            //Sending portions of size n of A and B using MPI_Scatter to each available task and saving in C and D
            st1 = MPI_Wtime();
            MPI_Scatter(A,n,MPI_INT,C,n,MPI_INT,0,MPI_COMM_WORLD);
            MPI_Scatter(B,n,MPI_INT,D,n,MPI_INT,0,MPI_COMM_WORLD);
            st2 = (MPI_Wtime() - st1);
            if (st2 < stmin) stmin = st2;
        }

        //calculating local intern product for each task
        for (int i=0; i < n; i++){
            s += C[i] * D[i];
        }

        /*  Now each task has an s variable with its intern product     **
        **  As requested in the homework, each task will send its       **
        **  intern product to the other tasks but I believe it would    **
        **  be way more effective using the function MPI_Allreduce in   **
        **  this moment because it does exacty what was requested       */

        for (int i=0; i < p; i++){
            if(rank != i){
                //send s (local intern product) to every task that is not the current task
                srt1 = MPI_Wtime();
                MPI_Send(&s,1,MPI_INT,i,1,MPI_COMM_WORLD);
                srt2 = (MPI_Wtime() - srt1);
                if (srt2 < srtmin) srtmin = srt2;
            }
        }

        for (int i=0; i < p; i++){
            if(rank != i){
                //receive s and save in t
                MPI_Recv(&t,1,MPI_INT,i,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                s += t;
            } 
        }
        tmin = stmin + srtmin;
    }
    /*  Now the s value on each task should have the intern product value   */
    //printf("Task %d has s = %d\n",rank,s);

    if(rank == 0){
        printf("tempo minimo = %f\n",tmin);
        free(A);
        free(B);
    }

    free(C);
    free(D);

    MPI_Finalize();
    return 0;
}