#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

pthread_mutex_t lck; 

typedef struct sum_args {
    int s;
    int e;
    int* sum;
} sum_args;

void* sum(void* args) {
    sum_args* a = (sum_args*) args;
    pthread_mutex_lock(&lck);
    for(int i = a->s; i <= a->e; i++) {
        *a->sum += i;
    }
    pthread_mutex_unlock(&lck);
    return NULL;
}

int main () {
    int aout=0;
    int bout=0;
    sum_args arg1 = {0, 10, &aout};
    sum_args arg2 = {11, 20, &bout};
    pthread_t t1;
    if(pthread_mutex_init(&lck, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        return 1; 
    } 
    
    pthread_create(&t1, NULL, sum, (void*) &arg1);
    sum((void*) &arg2);
    pthread_join(t1, NULL);
    pthread_mutex_destroy(&lck); 
    printf("sum = %d\n", aout+bout);
}