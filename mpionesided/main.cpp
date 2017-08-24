#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"

/* tests passive target RMA on 2 processes */

#define SIZE1 2
#define SIZE2 4
struct name{
  int i;
  name(){
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    //printf("I am rank %d %d\n",rank,i);
  }
};
int main2(int argc, char *argv[])
{
  MPI_Init(&argc,&argv);
  int rank, nprocs, i;
  name A[SIZE2], B[SIZE2],C[SIZE2];
  MPI_Win win,win1,win2;
  int errs = 0;
  MPI_Datatype  type[1] = {MPI_INT};
  int blocklen[1] = {1};
  MPI_Aint disp[1]={0};
  MPI_Datatype mpi_request;
  MPI_Type_struct(1, blocklen, disp, type, &mpi_request);
  MPI_Type_commit(&mpi_request);

  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  int server;

  if (rank == 0) {
    for (i=0; i<SIZE2; i++){
      A[i].i = i;
    }
    MPI_Win_create(C, SIZE2*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win1);
    MPI_Win_create(B, SIZE2*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    for (i=0; i<SIZE1; i++) {
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 1, 0, win);
      MPI_Put(A+i, 1, mpi_request, 1, i, 1, mpi_request, win);
      MPI_Win_unlock(1, win);
    }
    printf("putdone\n");
    MPI_Win_free(&win);
    MPI_Win_free(&win1);
  }else if(rank==2){
    for (i=0; i<SIZE2; i++){
      A[i].i = i;
    }
    MPI_Win_create(C, SIZE2*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win1);
    MPI_Win_create(B, SIZE2*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    for (i=0; i<SIZE1; i++) {
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
      MPI_Put(A+i, 1, mpi_request, 0, i, 1, mpi_request, win);
      MPI_Win_unlock(0, win);
    }
    printf("put done \n");
    MPI_Win_free(&win);
    MPI_Win_free(&win1);
  }
  else {  /* rank=1 */
    for (i=0; i<SIZE2; i++) B[i].i = (-4)*i;
    MPI_Win_create(C, SIZE2*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win1);
    MPI_Win_create(B, SIZE2*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    printf("bucket_win create 1 done \n");
    MPI_Win_free(&win);
    MPI_Win_free(&win1);
    printf("bucket_win free 1 done \n");

  }
  for (i=0; i<SIZE1; i++) {
    if (B[i].i != i) {
      printf("Put Error: B[%d] is %d, should be %d\n", i, B[i], i);
      errs++;
    }else{
      printf("Put correct: B[%d] is %d, should be %d\n", i, B[i], i);
    }
  }
  MPI_Type_free(&mpi_request);
  MPI_Finalize();
  return errs;
}