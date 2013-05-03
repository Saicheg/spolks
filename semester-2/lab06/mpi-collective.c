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


double *A, *A_block, *B, *C_block, *C_result;
double start_time, end_time;
int* group_numbers;
MPI_Comm group_communicator;
int groups_count, group_number, group_rank, group_size;

int *group_scatter_sizes, *group_scatter_displacements;
int scatter_size, scatter_displacement;

int N, async, global_rank, global_size, block_size;

void groupsPrepare() {
    group_numbers = calloc(global_size, sizeof(int));
    if (global_rank == 0) {
        int available_ranks = global_size - groups_count;
        int next_group_rank = 0;
        for (int i = 0; i < groups_count; i++) {
            if (i != groups_count - 1) {
                int random_group_modifier = rand() % (available_ranks * 2 / 3);
                available_ranks -= random_group_modifier;
                int random_group_size = 1 + random_group_modifier;
                for (int j = 0; j < random_group_size; j++) {
                    group_numbers[next_group_rank + j] = i;
                }
                next_group_rank += random_group_size;
            } else {
                for (int j = next_group_rank; j < global_size; j++) {
                    group_numbers[j] = groups_count - 1;
                }
            }
        }
    }
    MPI_Bcast(group_numbers, global_size, MPI_INT, 0, MPI_COMM_WORLD);
    group_number = group_numbers[global_rank];
    //printf("Process %d group number is %d\n", global_rank, group_number);
    MPI_Comm_split(MPI_COMM_WORLD, group_number, 0, &group_communicator);
    MPI_Comm_size(group_communicator, &group_size);
    MPI_Comm_rank(group_communicator, &group_rank);
}

void groupsFinalize() {

    double sum = 0;
    for (int i = 0; i < N * N - 1; i++) {
        sum += C_result[i];
    }
    printf("Group size: %d. Sum is %0.0lf. Time is %lf.\n", group_size, sum, end_time - start_time);
    MPI_Comm_free(&group_communicator);
}

void prepareCalculate() {
    MPI_Bcast(A, N*N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(B, N*N, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    if (group_rank == 0) {
        C_result = malloc(N * N * sizeof(double));
    }
}

void scatterCalculateBlock() {
    for (int i = 0; i < scatter_size; i += N) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                C_block[i + j] += A_block[i + k] * B[j * N + k];
            }
        }
    }

}

void scatterCalculate() {
    group_scatter_sizes = calloc(group_size, sizeof(int));
    group_scatter_displacements = calloc(group_size, sizeof(int));
    if (group_rank == 0) {
        start_time = MPI_Wtime();

        int worker_scatter_size = N / group_size;
        int last_worker_scatter_size = N - (group_size - 1) * worker_scatter_size;
        for (int i = 0; i < group_size; i++) {
            group_scatter_sizes[i] = (i == group_size - 1 ? last_worker_scatter_size : worker_scatter_size) * N;
            group_scatter_displacements[i] = i == 0 ? 0 : group_scatter_displacements[i - 1] + group_scatter_sizes[i - 1];
            //printf("Calculated group %d rank %d : size %7d disp %7d\n", group_number, i, group_scatter_sizes[i], group_scatter_displacements[i]);
        }
    }
    MPI_Scatter(group_scatter_sizes, 1, MPI_INT, &scatter_size, 1, MPI_INT, 0, group_communicator);
    MPI_Scatter(group_scatter_displacements, 1, MPI_INT, &scatter_displacement, 1, MPI_INT, 0, group_communicator);
    //printf("Group %d rank %d : size %7d disp %7d\n", group_number, group_rank, scatter_size, scatter_displacement);

    A_block = &A[scatter_displacement];
    C_block = calloc(scatter_size, sizeof(double));
    scatterCalculateBlock();
    MPI_Gatherv(C_block, scatter_size, MPI_DOUBLE, C_result, group_scatter_sizes, group_scatter_displacements, MPI_DOUBLE, 0, group_communicator);
    if (group_rank == 0)
        end_time = MPI_Wtime();
}

void globalInitMemory() {
    A = malloc(N * N * sizeof(double));
    B = malloc(N * N * sizeof(double));
}

void masterInitMatrix() {
    for (int i = 0; i < N * N; i++) {
        A[i] = rand() % MAX_VALUE;
        B[i] = rand() % MAX_VALUE;
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));

    if (argc < 3) {
        printf("Params: <groups count> <matrix side>\n");
        return 0;
    }

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &global_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &global_size);

    groups_count = atoi(argv[1]);

    N = atoi(argv[2]);

    globalInitMemory();
    if (global_rank == 0) {
        masterInitMatrix();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    groupsPrepare();
    prepareCalculate();
    scatterCalculate();
    if (group_rank == 0)
        groupsFinalize();

    MPI_Finalize();
    return 0;
}


