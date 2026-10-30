#include "parallel_sum_lib.h"
#include <pthread.h>
#include <stdlib.h>

typedef struct {
    int *array;
    int begin;
    int end;
    long long partial_sum;
} ThreadArg;

void* thread_func(void *arg) {
    ThreadArg *t_arg = (ThreadArg *)arg;
    t_arg->partial_sum = 0;
    
    for (int i = t_arg->begin; i < t_arg->end; i++) {
        t_arg->partial_sum += t_arg->array[i];
    }
    return NULL;
}

long long parallel_sum(int *array, int array_size, int threads_num) {
    pthread_t *threads = malloc(sizeof(pthread_t) * threads_num);
    ThreadArg *args = malloc(sizeof(ThreadArg) * threads_num);
    
    if (threads == NULL || args == NULL) return -1;

    for (int i = 0; i < threads_num; i++) {
        args[i].array = array;
        args[i].begin = i * array_size / threads_num;
        args[i].end = (i + 1) * array_size / threads_num;
        args[i].partial_sum = 0;

        int result = pthread_create(&threads[i], NULL, thread_func, &args[i]);
        if (result != 0) {
            thread_func(&args[i]); // Если поток не создался, считаем синхронно
        }
    }

    long long total_sum = 0;
    for (int i = 0; i < threads_num; i++) {
        pthread_join(threads[i], NULL);
        total_sum += args[i].partial_sum;
    }

    free(threads);
    free(args);
    return total_sum;
}