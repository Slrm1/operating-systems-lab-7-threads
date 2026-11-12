#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MATRIX_SIZE 20
#define THREAD_COUNT 10
#define RANDOM_VALUE_LIMIT 10

static int matrix_size = MATRIX_SIZE;
static int matrix_a[MATRIX_SIZE][MATRIX_SIZE];
static int matrix_b[MATRIX_SIZE][MATRIX_SIZE];
static int matrix_sum[MATRIX_SIZE][MATRIX_SIZE];
static int matrix_difference[MATRIX_SIZE][MATRIX_SIZE];
static int matrix_product[MATRIX_SIZE][MATRIX_SIZE];

typedef struct {
    int start_cell;
    int end_cell;
} WorkRange;

/* Fill the active square portion of a matrix with small random integers. */
static void fill_matrix(int matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    for (int row = 0; row < matrix_size; ++row) {
        for (int column = 0; column < matrix_size; ++column) {
            matrix[row][column] = rand() % RANDOM_VALUE_LIMIT;
        }
    }
}

/* Print only the active square portion of a matrix. */
static void print_matrix(const int matrix[MATRIX_SIZE][MATRIX_SIZE]) {
    for (int row = 0; row < matrix_size; ++row) {
        for (int column = 0; column < matrix_size; ++column) {
            /* Preserve the starter format: every value is followed by a space. */
            printf("%d ", matrix[row][column]);
        }
        putchar('\n');
    }
}

/* Each worker owns a disjoint range of result cells, so no lock is needed. */
static void *calculate(void *argument) {
    const WorkRange *range = argument;

    for (int cell = range->start_cell; cell < range->end_cell; ++cell) {
        const int row = cell / matrix_size;
        const int column = cell % matrix_size;
        int dot_product = 0;

        matrix_sum[row][column] = matrix_a[row][column] + matrix_b[row][column];
        matrix_difference[row][column] =
            matrix_a[row][column] - matrix_b[row][column];

        for (int index = 0; index < matrix_size; ++index) {
            dot_product += matrix_a[row][index] * matrix_b[index][column];
        }
        matrix_product[row][column] = dot_product;
    }
    return NULL;
}

/* Parse an optional matrix size while rejecting malformed input. */
static int parse_matrix_size(const char *text, int *result) {
    char *end = NULL;
    errno = 0;
    const long value = strtol(text, &end, 10);

    if (errno != 0 || end == text || *end != '\0' ||
        value < 1 || value > MATRIX_SIZE || value > INT_MAX) {
        return 0;
    }
    *result = (int)value;
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [matrix-size]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2 && !parse_matrix_size(argv[1], &matrix_size)) {
        fprintf(stderr, "Matrix size must be an integer from 1 to %d.\n",
                MATRIX_SIZE);
        return EXIT_FAILURE;
    }

    srand((unsigned)time(NULL));
    fill_matrix(matrix_a);
    fill_matrix(matrix_b);

    printf("Matrix A:\n");
    print_matrix(matrix_a);
    printf("Matrix B:\n");
    print_matrix(matrix_b);

    const int total_cells = matrix_size * matrix_size;
    /* Avoid idle workers for tiny matrices while still using 10 for 20x20. */
    const int active_threads =
        total_cells < THREAD_COUNT ? total_cells : THREAD_COUNT;
    pthread_t threads[THREAD_COUNT];
    WorkRange ranges[THREAD_COUNT];
    int created_threads = 0;

    /* Integer partitioning covers every cell exactly once, including any remainder. */
    for (int thread = 0; thread < active_threads; ++thread) {
        ranges[thread].start_cell = thread * total_cells / active_threads;
        ranges[thread].end_cell = (thread + 1) * total_cells / active_threads;
        const int status = pthread_create(&threads[thread], NULL, calculate,
                                          &ranges[thread]);
        if (status != 0) {
            fprintf(stderr, "pthread_create failed with code %d.\n", status);
            for (int joined = 0; joined < created_threads; ++joined) {
                pthread_join(threads[joined], NULL);
            }
            return EXIT_FAILURE;
        }
        ++created_threads;
    }

    for (int thread = 0; thread < created_threads; ++thread) {
        const int status = pthread_join(threads[thread], NULL);
        if (status != 0) {
            fprintf(stderr, "pthread_join failed with code %d.\n", status);
            return EXIT_FAILURE;
        }
    }

    printf("Results:\nSum:\n");
    print_matrix(matrix_sum);
    printf("Difference:\n");
    print_matrix(matrix_difference);
    printf("Product:\n");
    print_matrix(matrix_product);
    return EXIT_SUCCESS;
}
