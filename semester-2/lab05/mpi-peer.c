#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define TAG_DEFAULT       1
#define TAG_BLOCK_SIZE    2
#define TAG_MATRIX_FIRST  3
#define TAG_MATRIX_RESULT 4

#define MAX_VALUE 100
#define MAX_ERROR 0.001


double *A, *B, *C, *C_self;
int N, async, current_rank, current_size,
block_size;

void masterInitMemory() {
    A = malloc(N * N * sizeof(double));
    B = malloc(N * N * sizeof(double));
    C = malloc(N * N * sizeof(double));
    C_self = malloc(N * N * sizeof(double));
}

void masterInitMatrix() {
    for (int i = 0; i < N * N; i++) {
        A[i] = rand() % MAX_VALUE;
        B[i] = rand() % MAX_VALUE;
        C[i] = 0.0;
        C_self[i] = 0.0;
    }
}

void masterCalculateOnSelf() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                C_self[i * N + j] += A[i * N + k] * B[j * N + k];
            }
        }
    }
}

void masterValidateResults() {
    long errors = 0;
    for (int i = 0; i < N * N; i++) {
        if (fabs(1.0 - (C[i] / C_self[i])) > MAX_ERROR)
            errors++;
    }
    if (errors == 0) {
        printf("No errors.\n");
    } else {
        printf("%ld of %ld values are wrong!\n", errors, (long) N * N);
    }
}

void masterCalculateOnSlaves() {
    int workers_count = current_size - 1;

    block_size = N / workers_count;
    int last_worker_block_size = N - (workers_count - 1) * block_size;

    MPI_Bcast(B, N*N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    for (int i = 0; i < workers_count; i++) {
        int current_worker_block_size = block_size;
        if (i == workers_count - 1)
            current_worker_block_size = last_worker_block_size;

        MPI_Send(&current_worker_block_size, 1, MPI_INT, i + 1, TAG_BLOCK_SIZE, MPI_COMM_WORLD);
    }

    for (int i = 0; i < workers_count; i++) {
        int current_worker_block_size = block_size;
        if (i == workers_count - 1)
            current_worker_block_size = last_worker_block_size;
        MPI_Send(&(A[N * i * block_size]), current_worker_block_size * N, MPI_DOUBLE, i + 1, TAG_MATRIX_FIRST, MPI_COMM_WORLD);
    }

    for (int i = 0; i < workers_count; i++) {
        int current_worker_block_size = block_size;
        if (i == workers_count - 1)
            current_worker_block_size = last_worker_block_size;
        MPI_Recv(&(C[N * i * block_size]), current_worker_block_size * N, MPI_DOUBLE, i + 1, TAG_MATRIX_RESULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

void slavePrepareMemory() {
    B = malloc(N * N * sizeof(double));
    MPI_Bcast(B, N*N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    MPI_Recv(&block_size, 1, MPI_INT, 0, TAG_BLOCK_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    A = malloc(block_size * N * sizeof(double));
    
    C = calloc(block_size * N, sizeof(double));
}

void slaveCalculateBlock() {
    MPI_Recv(A, block_size * N, MPI_DOUBLE, 0, TAG_MATRIX_FIRST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++)
            {
                C[i*N + j] += A[i*N + k] * B[j*N + k];
            }
        }
    }
    MPI_Send(C, block_size * N, MPI_DOUBLE, 0, TAG_MATRIX_RESULT, MPI_COMM_WORLD);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (argc < 3) {
        printf("Params: <matrix side> <mode>\n");
        printf("Mode: 0 - sync\n");
        printf("      1 - async\n");
        return 0;
    }


    double startTime, endTime;
    int numElements, offset, stripSize, i, j, k;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &current_size);

    N = atoi(argv[1]);
    async = atoi(argv[2]);

    if (current_rank == 0) {
        masterInitMemory();
        masterInitMatrix();
        masterCalculateOnSelf();
        MPI_Barrier(MPI_COMM_WORLD);
        masterCalculateOnSlaves();
        masterValidateResults();
    } else {
        MPI_Barrier(MPI_COMM_WORLD);
        slavePrepareMemory();
        slaveCalculateBlock();
    }
    /*
        // start timer
        if (current_rank == 0) {
            startTime = MPI_Wtime();
        

        stripSize = N / current_size;

        // send each node its piece of A -- note could be done via MPI_Scatter
        if (current_rank == 0) {
            offset = stripSize;
            numElements = stripSize * N;
            for (i = 1; i < current_size; i++) {
                MPI_Send(A[offset], numElements, MPI_DOUBLE, i, TAG_DEFAULT, MPI_COMM_WORLD);
                offset += stripSize;
            }
        } else { // receive my part of A
            MPI_Recv(A[0], stripSize * N, MPI_DOUBLE, 0, TAG_DEFAULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // everyone gets B
        MPI_Bcast(B[0], N*N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // Let each process initialize C to zero 
        for (i = 0; i < stripSize; i++) {
            for (j = 0; j < N; j++) {
                C[i][j] = 0.0;
            }
        }

        // do the work
        for (i = 0; i < stripSize; i++) {
            for (j = 0; j < N; j++) {
                for (k = 0; k < N; k++) {
                    C[i][j] += A[i][k] * B[k][j];
                }
            }
        }

        // master receives from workers  -- note could be done via MPI_Gather
        if (current_rank == 0) {
            offset = stripSize;
            numElements = stripSize * N;
            for (i = 1; i < current_size; i++) {
                MPI_Recv(C[offset], numElements, MPI_DOUBLE, i, TAG_DEFAULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                offset += stripSize;
            }
        } else { // send my contribution to C
            MPI_Send(C[0], stripSize * N, MPI_DOUBLE, 0, TAG_DEFAULT, MPI_COMM_WORLD);
        }

        // stop timer
        if (current_rank == 0) {
            endTime = MPI_Wtime();
            printf("Time is %f\n", endTime - startTime);
        }

        // print out matrix here, if I'm the master
        if (current_rank == 0 && N < 10) {
            for (i = 0; i < N; i++) {
                for (j = 0; j < N; j++) {
                    printf("%f ", C[i][j]);
                }
                printf("\n");
            }
        }
     */
    MPI_Finalize();
    return 0;
}


