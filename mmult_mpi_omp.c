#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#define min(x, y) ((x)<(y)?(x):(y))

double* gen_matrix(int n, int m);
int mmult(double *c, double *a, int aRows, int aCols, double *b, int bRows, int bCols);
void compare_matrix(double *a, double *b, int nRows, int nCols);
void printMatrix(double* a, int rows, int cols);

/** 
    Program to multiply a matrix times a matrix using both
    mpi to distribute the computation among nodes and omp
    to distribute the computation among threads.
*/

int main(int argc, char* argv[])
{
  int nrows, ncols;
  double *aa;	/* the A matrix */
  double *bb;	/* the B matrix */
  double *cc1;	/* A x B computed using the omp-mpi code you write */
  double *cc2;	/* A x B computed using the conventional algorithm */
  double *temp_cc;
  int myid, numprocs, number;
  double starttime, endtime;
  int rows_per, remainder_rows, offset;
  MPI_Status status;
  /* insert other global variables here */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  if (argc > 1) {
    nrows = atoi(argv[1]);
    ncols = nrows;
    if (myid == 0) {
      // Master Code goes here
      aa = gen_matrix(nrows, ncols);
      bb = gen_matrix(ncols, nrows);
      cc1 = malloc(sizeof(double) * nrows * nrows); 
  /*    
      printf("############\n");
      printf("aa\n");
      printMatrix(aa, nrows, ncols);
      printf("bb\n");
      printMatrix(bb, nrows, ncols);
      printf("############\n");
*/
      starttime = MPI_Wtime();
      
      
      rows_per = nrows / (numprocs-1);
      remainder_rows = nrows / (numprocs-1);
      offset = 0;
/*
      printf("### numprocs: %d\n", numprocs);
      printf("### %d * %d\n", nrows, ncols);
      printf("### rows_per: %d\n", rows_per);
      printf("### remaining: %d\n", remainder_rows);
  */    int i, j;
      for(i = 1; i < numprocs; i++){
          number *= i;
          //printf("aa[5] = %lf\n", aa[5]);
          //printf("sending to %d\n", i);
          //MPI_Send(&b, M_SIZE * M_SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);
          MPI_Send(&(bb[0]), 2*nrows*ncols, MPI_INT, i, 0, MPI_COMM_WORLD);
          MPI_Send(&offset, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
          if(i == numprocs-1)
              rows_per += remainder_rows;
          MPI_Send(&rows_per, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

          for(j = 0; j < nrows*rows_per; j++){
              MPI_Send(&(aa[offset*ncols + j]), 2, MPI_INT, i, 0, MPI_COMM_WORLD);
          }
          MPI_Send(&(bb[offset*nrows]), 2*ncols, MPI_INT, i, 0, MPI_COMM_WORLD);
          //MPI_Send(&(aa[5]), 1, MPI_INT, i, 0, MPI_COMM_WORLD);
          offset+=rows_per;
      }
      //return 1;
      temp_cc = malloc(sizeof(double)*2 * nrows*nrows);
      for(i = 1; i < numprocs; i++){
          MPI_Recv(&offset, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
          MPI_Recv(&(temp_cc[nrows*offset]), 2*nrows*nrows, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          //printf("returned cc:\n");
          //printMatrix(temp_cc, nrows, nrows);
      }
      cc1 = temp_cc;
      /* Insert your master code here to store the product into cc1 */
      endtime = MPI_Wtime();
      printf("%f\n",(endtime - starttime));
      cc2  = malloc(sizeof(double) * nrows * nrows);
      mmult(cc2, aa, nrows, ncols, bb, ncols, nrows);
      compare_matrices(cc2, cc1, nrows, nrows);
    } else {
      bb = malloc(sizeof(double) * nrows * ncols);
     
      //MPI_Recv(&b, M_SIZE * M_SIZE, MPI_INT, MASTER_RANK, message_tag, MPI_COMM_WORLD, &status);
      MPI_Recv(&(bb[0]), 2*nrows*ncols, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&offset, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&rows_per, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      
      aa = malloc(sizeof(double) * ncols* rows_per);
      temp_cc = malloc(sizeof(double) * nrows*nrows); 
          
      int k, h;
      for(k = 0; k < ncols*rows_per; k++){
          MPI_Recv(&(aa[k]), 2, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE); 
      }

      mmult(temp_cc,aa,rows_per,ncols,bb,nrows,ncols); 
      MPI_Send(&offset, 1, MPI_INT, 0, 0,MPI_COMM_WORLD);
      MPI_Send(&(temp_cc[0]), 2*nrows*nrows, MPI_INT, 0, 0, MPI_COMM_WORLD);
    
      //MPI_Recv(&(aa[5]), 1, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      /*
      printf("\npassed bb:%d\n", myid);
      printMatrix(bb, nrows, ncols);
      printf("passed offset: %d\n", offset);
      printf("passed rows_per: %d\n", rows_per);
      printf("passed aa:\n");
      int j;
      for(j = 0; j < nrows; j++){
          printf("%lf ", aa[j]);
      }
      printf("\ngenerated c:\n");
      for(j = 0; j < nrows; j++){
          for(k = 0; k < ncols; k++){
              printf("%lf ",temp_cc[j*ncols + k]); 
          }
          printf("\n");
      }
      printf("\n\n%d done\n\n", myid);*/
    }
  } else {
    fprintf(stderr, "Usage matrix_times_vector <size>\n");
  }
  MPI_Finalize();
  return 0;
}

void printMatrix(double *a, int rows, int cols){
    int i, j;
    printf("\n");
    for(i = 0; i < rows; i++){
        for(j = 0; j < cols; j++){
            printf("%lf ", a[i*rows + j]);
        }
        printf("\n");
    }
}
