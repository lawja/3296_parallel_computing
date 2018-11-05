#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/times.h>
#define min(x, y) ((x)<(y)?(x):(y))

double* gen_matrix(int n, int m);
int mmult(double *c, double *a, int aRows, int aCols, double *b, int bRows, int bCols);
void compare_matrix(double *a, double *b, int nRows, int nCols);

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
  int myid, numprocs;
  double starttime, endtime;
  MPI_Status status;
  /* insert other global variables here */
  int workers, source, dest, rows, i,j,k, offset;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  workers = numprocs - 1;

  if (argc > 1) {
    nrows = atoi(argv[1]);
    ncols = nrows;
    if (myid == 0) {
      // Master Code goes here
      aa = gen_matrix(nrows, ncols);
      bb = gen_matrix(ncols, nrows);
      cc1 = malloc(sizeof(double) * nrows * nrows);
      rows = nrows/workers;
      offset = 0;
      starttime = MPI_Wtime();
      /* Insert your master code here to store the product into cc1 */

        for (dest=1; dest<=workers; dest++)
        {
            MPI_Send(&offset, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            MPI_Send(&rows, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            MPI_Send(aa[offset], rows*ncols, MPI_DOUBLE,dest,1, MPI_COMM_WORLD);
            MPI_Send(bb, nrows*ncols, MPI_DOUBLE, dest, 1, MPI_COMM_WORLD);
            MPI_Send(cc1, nrows*ncols, MPI_DOUBLE, dest, 1, MPI_COMM_WORLD);
            offset = offset + rows;
        }

        /* wait for results from all worker tasks */
        for (i=1; i<=workers; i++)
        {
            source = i;
            MPI_Recv(&offset, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(&rows, 1, MPI_INT, source, 2, MPI_COMM_WORLD, &status);
            MPI_Recv(cc1[offset], rows*nrows, MPI_DOUBLE, source, 2, MPI_COMM_WORLD, &status);
        }


        endtime = MPI_Wtime();
      printf("%f\n",(endtime - starttime));
      cc2  = malloc(sizeof(double) * nrows * nrows);
      mmult(cc2, aa, nrows, ncols, bb, ncols, nrows);
      compare_matrices(cc2, cc1, nrows, nrows);
    } else {
        source = 0;
        MPI_Recv(&offset, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&rows, 1, MPI_INT, source, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&aa, rows*ncols, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&bb, nrows*ncols, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&cc1, nrows*ncols, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, &status);
        /* Matrix multiplication */
        for (k=0; k<ncols; k++)
            for (i=0; i<rows; i++) {
                cc1[i][k] = 0.0;
                for (j=0; j<ncols; j++)
                    cc1[i][k] = cc1[i][k] + aa[i][j] * bb[j][k];
            }


        MPI_Send(&offset, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(&rows, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(cc1, rows*ncols, MPI_DOUBLE, 0, 2, MPI_COMM_WORLD);
    }
  } else {
    fprintf(stderr, "Usage matrix_times_vector <size>\n");
  }
  MPI_Finalize();
  return 0;
}
