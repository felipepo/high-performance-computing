#include <stdio.h> 
#include <stdlib.h> 
#include <math.h>   
#include "mpi.h"
 
#define M_PI 3.14159265358979323846 
 
int main(int argc, char* argv[])  
{ 
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of tasks
    int nTasks;
    MPI_Comm_size(MPI_COMM_WORLD, &nTasks);

    // Get the rank of the task
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


   char* usage_fmt = "usage:  %s number_of_samples seed\n"; 
   char* end_ptr_for_strtol; 
 
    /* Obtendo parametros por linha de comando  */ 
    if (argc != 3)  { 
        fprintf(stderr, usage_fmt, argv[0]); 
        exit(EXIT_FAILURE); 
    } 
    long num_samples = strtol(argv[1], &end_ptr_for_strtol, 10); 
    if (*end_ptr_for_strtol != '\0') { 
        fprintf(stderr, usage_fmt, argv[0]); 
        exit(EXIT_FAILURE); 
    } 
    long seed = strtol(argv[2], &end_ptr_for_strtol, 10); 
    if (*end_ptr_for_strtol != '\0') { 
        fprintf(stderr, usage_fmt, argv[0]); 
        exit(EXIT_FAILURE); 
    } 
 
 
    srand((unsigned int) seed*rank); 
    int count = 0; 
    double x, y; 
    for (int i = 0; i < num_samples; ++i) { 
        x = (double) rand() / (double) (RAND_MAX); 
        y = (double) rand() / (double) (RAND_MAX); 
        if ((x*x + y*y) <= 1.0)
            //printf("rank = %d has (x,y) = (%f,%f)\n",rank,x,y); 
            ++count; 
    }

    //printf("rank = %d has count = %d\n",rank,count);


    /*
    **  At this point, each task has a different value of count that      **
    **  indicates how many points are within the circle, thus, all we     **
    **  need to do is send every count variable to the master task and    **
    **  sum its value to the master local count and we need to multiply   **
    **  the number of samples by the number of tasks as well              **
                                                                          */

    if(rank != 0) {
        //All tasks (but the task 0) send their count to the task 0
        MPI_Send(&count,1,MPI_INT,0,1,MPI_COMM_WORLD);
    }

    else {
        int received_count = 0;
        for(int i=1; i<nTasks; i++){
            //Task 0 receive the count of all other tasks and adds them to its own count
            MPI_Recv(&received_count,1,MPI_INT,i,1,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            count += received_count;
        }
        //Update num_samples
        num_samples *= nTasks;
        //calculate pi (only on task 0)
        double pi = 4.0 * (double) count / (double) num_samples;  
        //print results
        printf("Resultado:\n"); 
        printf("Total de pontos = %ld, count= %d seed = %ld\n", num_samples, count, seed); 
        printf("Pi estimado = %12.10f\n", pi); 
        printf("Erro = %12.10f\n", fabs(pi - M_PI)); 
    }
    
    MPI_Finalize();
    /* clean up and return */ 
    return EXIT_SUCCESS; 
} 