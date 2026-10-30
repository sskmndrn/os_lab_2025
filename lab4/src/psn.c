#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/time.h>
#include "utils.h"             
#include "parallel_sum_lib.h"  

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int threads_num = -1;

    while (1) {
        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"threads_num", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        seed = atoi(optarg);
                        if (seed <= 0) { printf("seed must be positive\n"); return 1; }
                        break;
                    case 1:
                        array_size = atoi(optarg);
                        if (array_size <= 0) { printf("array_size must be positive\n"); return 1; }
                        break;
                    case 2:
                        threads_num = atoi(optarg);
                        if (threads_num <= 0) { printf("threads_num must be positive\n"); return 1; }
                        break;
                }
                break;
            default:
                return 1;
        }
    }

    if (seed == -1 || array_size == -1 || threads_num == -1) {
        printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
        return 1;
    }

    // 1. Генерация массива (ДО замера времени)
    int *array = malloc(sizeof(int) * array_size);
    if (array == NULL) {
        perror("malloc");
        return 1;
    }
    GenerateArray(array, array_size, seed);

    // 2. Замер времени только для подсчета суммы
    struct timeval start_time, finish_time;
    gettimeofday(&start_time, NULL);

    long long total_sum = parallel_sum(array, array_size, threads_num);

    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    // 3. Вывод результатов
    printf("Sum: %lld\n", total_sum);
    printf("Elapsed time (sum only): %f ms\n", elapsed_time);

    free(array);
    return 0;
}