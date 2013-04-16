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

    if (async == 0) {
        for (int i = 0; i < workers_count; i++) {
            int current_worker_block_size = block_size;
            if (i == workers_count - 1)
                current_worker_block_size = last_worker_block_size;
            MPI_Send(&current_worker_block_size, 1, MPI_INT, i + 1, TAG_BLOCK_SIZE, MPI_COMM_WORLD);
        }

    } else {

        MPI_Request *requests = calloc(workers_count, sizeof(MPI_Request));
        MPI_Status *statuses = calloc(workers_count, sizeof(MPI_Status));
        for (int i = 0; i < workers_count; i++) {
            int current_worker_block_size = block_size;
            if (i == workers_count - 1)
                current_worker_block_size = last_worker_block_size;
            MPI_Isend(&current_worker_block_size, 1, MPI_INT, i + 1, TAG_BLOCK_SIZE, MPI_COMM_WORLD, &requests[i]);
        }
        MPI_Waitall(workers_count, requests, statuses);
        free(requests);
        free(statuses);
    }


    if (async == 0) {
        for (int i = 0; i < workers_count; i++) {
            int current_worker_block_size = block_size;
            if (i == workers_count - 1)
                current_worker_block_size = last_worker_block_size;
            MPI_Send(&(A[N * i * block_size]), current_worker_block_size * N, MPI_DOUBLE, i + 1, TAG_MATRIX_FIRST, MPI_COMM_WORLD);
        }
    } else {
        MPI_Request *requests = calloc(workers_count, sizeof(MPI_Request));
        MPI_Status *statuses = calloc(workers_count, sizeof(MPI_Status));
        for (int i = 0; i < workers_count; i++) {
            int current_worker_block_size = block_size;
            if (i == workers_count - 1)
                current_worker_block_size = last_worker_block_size;
            MPI_Isend(&(A[N * i * block_size]), current_worker_block_size * N, MPI_DOUBLE, i + 1, TAG_MATRIX_FIRST, MPI_COMM_WORLD, &requests[i]);
        }
        MPI_Waitall(workers_count, requests, statuses);
        free(requests);
        free(statuses);
    }

    if (async == 0) {
        for (int i = 0; i < workers_count; i++) {
            int current_worker_block_size = block_size;
            if (i == workers_count - 1)
                current_worker_block_size = last_worker_block_size;
            MPI_Recv(&(C[N * i * block_size]), current_worker_block_size * N, MPI_DOUBLE, i + 1, TAG_MATRIX_RESULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        MPI_Request *requests = calloc(workers_count, sizeof(MPI_Request));
        MPI_Status *statuses = calloc(workers_count, sizeof(MPI_Status));
        for (int i = 0; i < workers_count; i++) {
            int current_worker_block_size = block_size;
            if (i == workers_count - 1)
                current_worker_block_size = last_worker_block_size;
            MPI_Irecv(&(C[N * i * block_size]), current_worker_block_size * N, MPI_DOUBLE, i + 1, TAG_MATRIX_RESULT, MPI_COMM_WORLD, &requests[i]);
        }
        MPI_Waitall(workers_count, requests, statuses);
        free(requests);
        free(statuses);

    }
}

void slavePrepareMemory() {
    B = malloc(N * N * sizeof(double));
    MPI_Bcast(B, N*N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (async == 0) {
        MPI_Recv(&block_size, 1, MPI_INT, 0, TAG_BLOCK_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Request request;
        MPI_Status status;
        MPI_Irecv(&block_size, 1, MPI_INT, 0, TAG_BLOCK_SIZE, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
    }
    A = malloc(block_size * N * sizeof(double));

    C = calloc(block_size * N, sizeof(double));
}

void slaveCalculateBlock() {
    if (async == 0) {
        MPI_Recv(A, block_size * N, MPI_DOUBLE, 0, TAG_MATRIX_FIRST, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else {
        MPI_Request request;
        MPI_Status status;
        MPI_Irecv(A, block_size * N, MPI_DOUBLE, 0, TAG_MATRIX_FIRST, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
    }
    for (int i = 0; i < block_size; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                C[i * N + j] += A[i * N + k] * B[j * N + k];
            }
        }
    }
    if (async == 0) {
        MPI_Send(C, block_size * N, MPI_DOUBLE, 0, TAG_MATRIX_RESULT, MPI_COMM_WORLD);
    } else {
        MPI_Request request;
        MPI_Status status;
        MPI_Isend(C, block_size * N, MPI_DOUBLE, 0, TAG_MATRIX_RESULT, MPI_COMM_WORLD, &request);
        MPI_Wait(&request, &status);
    }
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

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &current_size);

    N = atoi(argv[1]);
    async = atoi(argv[2]);

    if (current_rank == 0) {
        masterInitMemory();
        masterInitMatrix();
        masterCalculateOnSelf();
        startTime = MPI_Wtime();
        masterCalculateOnSlaves();
        endTime = MPI_Wtime();
        printf("Time is %f\n", endTime - startTime);

        masterValidateResults();
    } else {
        slavePrepareMemory();
        slaveCalculateBlock();
    }

    MPI_Finalize();
    return 0;
}


