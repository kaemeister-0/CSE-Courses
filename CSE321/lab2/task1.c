#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    int n;
} FibParams;

typedef struct {
    int *fib_array;
    int fib_size;
    int *search_indices;
    int num_searches;
} SearchParams;

void* compute_fibonacci(void *arg) {
    FibParams *params = (FibParams*) arg;
    int n = params->n;
    int *fib = (int*)malloc((n + 1) * sizeof(int));
    if (fib == NULL) {
        perror("Failed to allocate memory for Fibonacci sequence");
        exit(EXIT_FAILURE);
    }
    if (n >= 0) fib[0] = 0;
    if (n >= 1) fib[1] = 1;
    for (int i = 2; i <= n; i++) {
        fib[i] = fib[i-1] + fib[i-2];
    }
    free(params);
    return (void*)fib;
}

void* search_fibonacci(void *arg) {
    SearchParams *params = (SearchParams*) arg;
    int *results = (int*)malloc(params->num_searches * sizeof(int));
    for (int i = 0; i < params->num_searches; i++) {
        int idx = params->search_indices[i];
        if (idx >= 0 && idx < params->fib_size) {
            results[i] = params->fib_array[idx];
        } else {
            results[i] = -1;
        }
    }
    free(params);
    return (void*)results;
}

int main() {
    int n;
    printf("Enter the term of fibonacci sequence: ");
    scanf("%d", &n);

    FibParams *fibParams = (FibParams*)malloc(sizeof(FibParams));
    fibParams->n = n;
    pthread_t fib_thread;
    pthread_create(&fib_thread, NULL, compute_fibonacci, fibParams);
    int *fib_array;
    pthread_join(fib_thread, (void**)&fib_array);

    for (int i = 0; i <= n; i++) {
        printf("a[%d] = %d\n", i, fib_array[i]);
    }

    int s;
    printf("How many numbers you are willing to search?: ");
    scanf("%d", &s);
    int *search_indices = (int*)malloc(s * sizeof(int));
    for (int i = 0; i < s; i++) {
        printf("Enter search %d: ", i + 1);
        scanf("%d", &search_indices[i]);
    }

    SearchParams *searchParams = (SearchParams*)malloc(sizeof(SearchParams));
    searchParams->fib_array = fib_array;
    searchParams->fib_size = n + 1;
    searchParams->search_indices = search_indices;
    searchParams->num_searches = s;

    pthread_t search_thread;
    pthread_create(&search_thread, NULL, search_fibonacci, searchParams);
    int *search_results;
    pthread_join(search_thread, (void**)&search_results);

    for (int i = 0; i < s; i++) {
        printf("result of search #%d = %d\n", i + 1, search_results[i]);
    }

    free(fib_array);
    free(search_indices);
    free(search_results);
    return 0;
}