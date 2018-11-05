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
double* getCol(double* b, n, col);

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
  int myid, numprocs, number;
  double starttime, endtime;
  MPI_Status status;
  /* insert other global variables here */
  int offset;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  if (argc > 1) {
    nrows = atoi(argv[1]);
    ncols = nrows;
    if (myid == 0) {
      // Master Code goes here
      offset = 0;
      aa = gen_matrix(nrows, ncols);
      bb = gen_matrix(ncols, nrows);
      cc1 = malloc(sizeof(double) * nrows * nrows); 
      
      printf("############\n");
      printf("aa\n");
      printMatrix(aa, nrows, ncols);
      printf("bb\n");
      printMatrix(bb, nrows, ncols);
      printf("############\n");

      starttime = MPI_Wtime();
      int i;
      number = 69;
      printf("### numprocs: %d\n", numprocs);
      for(i = 1; i < numprocs; i++){
          number *= i;
          //printf("aa[5] = %lf\n", aa[5]);
          printf("sending to %d\n", i);
          //MPI_Send(&b, M_SIZE * M_SIZE, MPI_INT, i, 0, MPI_COMM_WORLD);
          MPI_Send(&(aa[0]), 2*nrows*ncols, MPI_INT, i, 0, MPI_COMM_WORLD);
          //MPI_Send(&(aa[5]), 1, MPI_INT, i, 0, MPI_COMM_WORLD);
          MPI_Send(&offset, sizeof(int)*offset, MPI_INT, i,  0, MPI_COMM_WORLD);
          double *temp = getCol(bb, nrows, offset)
          MPI_Send(&(temp[0]), 2*nrows, MPI_INT, i, 0, MPI_COMM_WORLD);
          offset++;
      }
     
      /* Insert your master code here to store the product into cc1 */
      endtime = MPI_Wtime();
      printf("%f\n",(endtime - starttime));
      cc2  = malloc(sizeof(double) * nrows * nrows);
      mmult(cc2, aa, nrows, ncols, bb, ncols, nrows);
      compare_matrices(cc2, cc1, nrows, nrows);
    } else {
      double temp[1][ncols];
      aa = malloc(sizeof(double) * nrows * ncols);
      //MPI_Recv(&b, M_SIZE * M_SIZE, MPI_INT, MASTER_RANK, message_tag, MPI_COMM_WORLD, &status);
      MPI_Recv(&(aa[0]), 2*nrows*ncols, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      //MPI_Recv(&(aa[5]), 1, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&pffset, sizeof(int)*offset, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&(temp), 2*nrows, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      printf("\npassed aa:%d\n", myid);
      printf("%lf", aa[5]);
      printMatrix(aa, nrows, ncols);
      printMatrix(temp, nrows, offset)

      printf("\n\n%d done\n\n", myid);
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

double* getCol(double* b, n, col)
{
  ret[1][n]
  for(int i = 0; i < n; i++)
  {
    ret[1][i] = b[col][i]
  }
  return ret;
}
