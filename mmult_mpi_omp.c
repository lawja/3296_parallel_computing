#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/times.h>
#define min(x, y) ((x)<(y)?(x):(y))

double* gen_matrix(int n, int m);
int mmult(double *c, double *a, int aRows, int aCols, double *b, int bRows, int bCols);
void compare_matrix(double *a, double *b, int nRows, int nCols);
void printMatrix(double* a, int rows, int cols);
void writeMatrix(double* a, int rows, int cols);

/** 
    Program to multiply a matrix times a matrix using both
    mpi to distribute the computation among nodes and omp
    to distribute the computation among threads.
*/

int main(int argc, char* argv[])
{
  char *source_file_1;
  char *source_file_2;
  FILE *sf1;
  FILE *sf2;
  int nrows, ncols, arows, acols, brows, bcols;
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
  //printf("%d", argc);
  if (argc > 1) {
    // if file names are passed
    if(argc > 2){
        source_file_1 = argv[1];
        source_file_2 = argv[2];
        
        sf1 = fopen(source_file_1, "r");
        if(sf1 == NULL){
            printf("unable to open %s\n", source_file_1);
            return 1;
        }
        sf2 = fopen(source_file_2, "r");
        if(sf2 == NULL){
            printf("unable to open %s\n", source_file_2);
            return 1;
        }
        
        int max_line = 2048 * 5;
        
        char *lineBuffer = (char *) malloc(max_line);
        
        
        char *line = NULL;
        size_t len = 0;
        ssize_t read;
        int c;
        int line_count = 0;
        char *p1, *p2, *p3, *p4;
        char *row_str_1, *col_str_1, row_str_2, *col_str_2;
        row_str_1 = (char *) malloc(8);
        col_str_1 = (char *) malloc(8);
        row_str_2 = (char *) malloc(8);
        col_str_2 = (char *) malloc(8);
        char s[2] = " ";
        char *token;
        int rows_1, cols_1, rows_2, cols_2;
        int t_count;

        while((read = getline(&line, &len, sf1))!=-1){
            if(line_count++ == 0){
               p1 = strstr(line, "rows(");
               if(p1){
                   p2 = strstr(p1, ")c");
                   if(p2){
                       sprintf(row_str_1, "%.*s", p2-p1-5, p1+5);
                       p3 = strstr(line, "cols(");
                       if(p3){
                           p4 = strstr(p3, ")");
                           if(p4){
                               sprintf(col_str_1, "%.*s", p4-p3-5, p3+5);
                               rows_1 = atof(row_str_1);
                               cols_1 = atof(col_str_1);
                           }else{
                               printf("unable to parse matrix dimensions for %s\n", source_file_1);
                               exit(1);
                           }
                       }else{
                           printf("unable to parse matrix dimensions for %s\n", source_file_1);
                           exit(1);
                       }
                   }else{
                       printf("unable to parse matrix dimensions for %s\n", source_file_1);
                       exit(1);
                   }
               }else{
                   printf("unable to parse matrix dimensions for %s\n", source_file_1);
                   exit(1);
               }
            }else{
                if(myid == 0){
                    if(line_count == 2){
                        aa = malloc(sizeof(double)*rows_1*cols_1);
                    }
                    token = strtok(line, s);
                    t_count = 0;
                    while(token != NULL){
                        //printf("token: %s :: %d     ", token, ((line_count-2)*cols_1)+t_count);
                        aa[((line_count-2)*cols_1) + t_count++] = atof(token);
                        token = strtok(NULL, s);
                    }
                }
            }
        }
        char *token_;
        line_count = 0;
        len = 0;
        char *p5, *p6, *p7, *p8;
        while((read = getline(&line, &len, sf2))!=-1){
            if(line_count++ == 0){
               p5 = strstr(line, "rows(");
               if(p5){
                   p6 = strstr(p5, ")c");
                   if(p6){
                       sprintf(row_str_1, "%.*s", p6-p5-5, p5+5);
                       p7 = strstr(line, "cols(");
                       if(p7){
                           p8 = strstr(p7, ")");
                           if(p8){
                               sprintf(col_str_1, "%.*s", p8-p7-5, p7+5);
                               rows_2 = atof(row_str_1);
                               cols_2 = atof(col_str_1);
                           }else{
                               printf("unable to parse matrix dimensions for %s\n", source_file_2);
                               exit(1);
                           }
                       }else{
                           printf("unable to parse matrix dimensions for %s\n", source_file_2);
                           exit(1);
                       }
                   }else{
                       printf("unable to parse matrix dimensions for %s\n", source_file_2);
                       exit(1);
                   }
               }else{
                   printf("unable to parse matrix dimensions for %s\n", source_file_2);
                   exit(1);
               }
            }else{
                if(myid == 0){
                    if(line_count == 2){
                        bb = malloc(sizeof(double)*rows_2*cols_2);
                    }
                    token_ = strtok(line, s);
                    t_count = 0;
                    while(token_ != NULL){
                        bb[((line_count-2)*cols_2) + t_count++] = atof(token_);
                        token_ = strtok(NULL, s);
                    }
                }
            }
        }
        if(cols_1 != rows_2){
            printf("matrix dimensions do not comply with multplication requirements\n");
            printf("A: %dx%d\tB: %dx%d", rows_1, cols_1, rows_2, cols_2);
            exit(1);
        }/*
        if(myid == 0){
            printf("rows1: %d\tcols_1:%d", rows_1, cols_1); 
            printf("rows2: %d\tcols_2:%d", rows_2, cols_2);
            printf("aa:");
            printMatrix(aa, rows_1, cols_1);
            printf("bb:");
            printMatrix(bb, rows_2, cols_2);
            
        }*/

        nrows = rows_1;
        ncols = cols_1;

        arows = rows_1;
        acols = cols_1;
        brows = rows_2;
        bcols = cols_2;
        
        fclose(sf1);
        fclose(sf2);

        free(lineBuffer);
    }else{
        nrows = atoi(argv[1]);
        ncols = nrows;
        
        arows = nrows;
        acols = ncols;
        brows = nrows;
        bcols = ncols;
    }
    if (myid == 0) {
      // Master Code goes here
      if(argc == 2){
          //printf("generating\n");
          aa = gen_matrix(nrows, ncols);
          bb = gen_matrix(ncols, nrows);
      }
      cc1 = malloc(sizeof(double) * arows * bcols); 
  /*    
      printf("############\n");
      printf("aa\n");
      printMatrix(aa, nrows, ncols);
      printf("bb\n");
      printMatrix(bb, nrows, ncols);
      printf("############\n");
*/
      starttime = MPI_Wtime();
      
      
      rows_per = arows / (numprocs-1);
      remainder_rows = arows / (numprocs-1);
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
          MPI_Send(&(bb[0]), 2*brows*bcols, MPI_INT, i, 0, MPI_COMM_WORLD);
          MPI_Send(&offset, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
          if(i == numprocs-1)
              rows_per += remainder_rows;
          MPI_Send(&rows_per, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

          for(j = 0; j < acols*rows_per; j++){
              MPI_Send(&(aa[offset*acols + j]), 2, MPI_INT, i, 0, MPI_COMM_WORLD);
          }
          MPI_Send(&(bb[offset*brows]), 2*ncols, MPI_INT, i, 0, MPI_COMM_WORLD);
          //MPI_Send(&(aa[5]), 1, MPI_INT, i, 0, MPI_COMM_WORLD);
          offset+=rows_per;
      }
      //return 1;
      temp_cc = malloc(sizeof(double)*2 * arows*bcols);
      for(i = 1; i < numprocs; i++){
          MPI_Recv(&offset, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
          MPI_Recv(&(temp_cc[arows*offset]), 2*arows*bcols, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          //printf("returned cc:\n");
          //printMatrix(temp_cc, arows, bcols);
      }
      cc1 = temp_cc;
      /* Insert your master code here to store the product into cc1 */
      endtime = MPI_Wtime();
      printf("%f\n",(endtime - starttime));
      cc2  = malloc(sizeof(double) * arows * bcols);
      mmult(cc2, aa, arows, acols, bb, brows, bcols);
      compare_matrices(cc2, cc1, arows, bcols);
      writeMatrix(cc2, arows, bcols);
      printf("resulting matrix written to out.txt\n");
    } else {
      bb = malloc(sizeof(double) * brows * bcols);
     
      //MPI_Recv(&b, M_SIZE * M_SIZE, MPI_INT, MASTER_RANK, message_tag, MPI_COMM_WORLD, &status);
      MPI_Recv(&(bb[0]), 2*brows*bcols, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&offset, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      MPI_Recv(&rows_per, 1, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      
      aa = malloc(sizeof(double) * acols* rows_per);
      temp_cc = malloc(sizeof(double) * arows*bcols); 
          
      int k, h;
      for(k = 0; k < acols*rows_per; k++){
          MPI_Recv(&(aa[k]), 2, MPI_INT, 0, 0, MPI_COMM_WORLD,MPI_STATUS_IGNORE); 
      }

      mmult(temp_cc,aa,rows_per,acols,bb,brows,bcols); 
      MPI_Send(&offset, 1, MPI_INT, 0, 0,MPI_COMM_WORLD);
      MPI_Send(&(temp_cc[0]), 2*arows*bcols, MPI_INT, 0, 0, MPI_COMM_WORLD);
    
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

void writeMatrix(double *a, int rows, int cols){
    FILE *f = fopen("out.txt", "w");
    int i, j;
    fprintf(f, "rows(%d)cols(%d)\n", rows, cols);
    for(i = 0; i < rows; i++){
        for(j = 0; j < cols; j++){
            fprintf(f, "%lf ", a[i*rows + j]);
        }
        fprintf(f, "\n");
    }
    //fprintf(f, "\0");
    fclose(f);
}
