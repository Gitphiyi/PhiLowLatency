#include <stdio.h>
#include <stdlib.h>
#include <arm_neon.h>
#include <string.h> 
#include <pthread.h>
#include <chrono>
#include <iostream>

using namespace std;
using namespace std::chrono;

const size_t n = 6*1000000;
pthread_mutex_t lck; 

void print_vec(int* a, int* b) {
    printf("a = {");
    for(int i = 0; i < n; i++) {
        printf("%d", a[i]);
        if(i < n-1) {
            printf(", ");
        }
    }
    printf("}\n");

    printf("b = {");
    for(int i = 0; i < n; i++) {
        printf("%d", b[i]);
        if(i < n-1) {
            printf(", ");
        }
    }
    printf( "}\n");
}

uint64_t dot(int* a, int* b) {
    auto start = high_resolution_clock::now();
    uint64_t dot = 0;
    for( int i = 0; i < n; i++) {
        dot += a[i] * b[i];
    }
    auto end = high_resolution_clock::now();
    auto dur = duration_cast<microseconds>(end - start);

    cout<< "normal dot product = " << dot << "\t duration: " << dur.count() << " microseconds" << endl;
    return dot;
}

typedef struct simd_args {
    int32x4_t* a;
    size_t start_window;
    size_t end_window;
    const size_t thread_count; // should be equal to stride
} simd_args;


//can optimize memory and speed more if I don't create a new temp array and just certain idices in the original temp array but do it later
void* simd_add(void* arg) {
    const simd_args* simd_arg = (simd_args*) arg;
    const int len = simd_arg->end_window - simd_arg->start_window + 1;
    int i;
    pthread_t threads[simd_arg->thread_count - 1];
    uint64_t res = 0;

    //base case
    if( len < simd_arg->thread_count ) // at this point it isn't worth creating new threads and simd adding on them
    {
        for (i = simd_arg->start_window; i < simd_arg->thread_count; ++i) {
            res += vgetq_lane_s32(temp[i], 0);
            res += vgetq_lane_s32(temp[i], 1);
            res += vgetq_lane_s32(temp[i], 2);
            res += vgetq_lane_s32(temp[i], 3);
        }
        return (void*) res;
    }

    //creates new temp array and populates it
    const size_t newlen = len / simd_arg->thread_count;
    int32x4_t* temp = (int32x4_t*) malloc(newlen);
    int ctr = 0;
    const size_t end = simd_arg->start_window + len % simd_arg->thread_count;

    //add simd registers 
    for (i = simd_arg->start_window; i < end; i+=2) {
        temp[ctr] = vaddq_s32(simd_arg->a[i], simd_arg->a[i+1]);
        ctr++;
    }

    //if array is not aligned then just add up rest simply
    if( len % simd_arg->thread_count != 0) {
        for (; i < len % simd_arg->thread_count; ++i) {
            res += vgetq_lane_s32(temp[i], 0);
            res += vgetq_lane_s32(temp[i], 1);
            res += vgetq_lane_s32(temp[i], 2);
            res += vgetq_lane_s32(temp[i], 3);
        }
    }

    //recursive step which are multithreaded
    const size_t step = newlen / simd_arg->thread_count;
    for(int i = 0; i < simd_arg->thread_count; i++) {
        simd_args arg = {
            temp, i*step, i*(step+1)
        };
        void* ret_val;
        pthread_create(&(threads[i]), NULL, simd_add, (void*) &arg);
    }
    for(int i = 0; i < simd_arg->thread_count; i++) {
        void* ret_val;
        pthread_join(threads[i], &ret_val); 
        res += (uint64_t) ret_val;
    } //final recursion step to add the stuff together
    free(temp);
    return (void*) res;  
}

uint64_t simd_dot(int* a, int* b) {
    auto start = high_resolution_clock::now();
    const size_t thread_count = 2; //threads including parent thread
    const int stride = 8;
    const int remainder = n % stride;
    const size_t len = n / 4 * sizeof(int32x4_t);
    int i = 0;
    int ctr = 0;
    int32x4_t* temp = (int32x4_t*) malloc(len);
    uint64_t result = 0;
    for(i = 0; i+stride-1 < n; i += stride) { //combining SIMD and loop unrolling
        int32x4_t a_reg = vld1q_s32(&arg->a[i]);
        int32x4_t b_reg = vld1q_s32(&arg->b[i]);
        int32x4_t prod = vmulq_s32(a_reg, b_reg);
        temp[ctr] = prod;
         a_reg = vld1q_s32(&arg->a[i+4]);
         b_reg = vld1q_s32(&arg->b[i+4]);
         prod = vmulq_s32(a_reg, b_reg);
        temp[ctr+1] = prod;
        ctr+=2;
    } //do the dot product
    for(; i < n; i++) {
        result += a[i] * b[i];
    } //add remainding dot product to result

    //recursively multithread add the simd dot product
    const size_t thread_num = 9;
    simd_args arg = {
        temp, 0, len, thread_num
    };
    result += (uint64_t) simd_add((void*) &arg);

    free(temp);
    auto end = high_resolution_clock::now();
    auto dur = duration_cast<microseconds>(end - start);
    cout << "simd dot product = " << result << "\t duration: " << dur.count() << " microseconds" << endl;
    return result;
}

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
int main() {
    int* a = (int*) malloc(n * sizeof(int));
    int* b = (int*) malloc(n * sizeof(int));
    for( int i = 0; i < n; i++) {
        a[i] = (i+1) % 10;
        b[i] = (i+1) % 10;
    }
    //print_vec(v1, v2);
    dot(a, b);
    simd_dot(a, b);
    free(a);
    free(b);
    return 0;
}