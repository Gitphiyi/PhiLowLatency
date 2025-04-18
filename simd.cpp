#include <arm_neon.h>
#include <string.h> 
#include <iostream>
#include <vector>
#include <chrono>
#include <climits>

using namespace std;
using namespace std::chrono;

//constexpr size_t n = 6*100000;
constexpr size_t n = 6*100000;

void print_vec(vector<int>& a, vector<int>& b) {
    cout << "a = {";
    for(int i = 0; i < n; i++) {
        cout<< a[i];
        if(i < n-1) {
            cout << ", ";
        }
    }
    cout << "}" << endl;

    cout << "b = {";
    for(int i = 0; i < n; i++) {
        cout<< b[i];
        if(i < n-1) {
            cout << ", ";
        }
    }
    cout << "}" << endl;
}
uint64_t dot(array<int, n>& a, array<int, n>& b) {
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

uint64_t simd_dot(array<int, n> a, array<int, n> b) {
    auto start = high_resolution_clock::now();
    int i = 0; 
    uint64_t result = 0;
    int32x4_t temp[n / 4];
    int ctr = 0;
    for(i = 0; i+7 < n; i += 8) { //combining SIMD and loop unrolling
        int32x4_t a_reg = vld1q_s32(&a[i]);
        int32x4_t b_reg = vld1q_s32(&b[i]);
        int32x4_t prod = vmulq_s32(a_reg, b_reg);
        temp[ctr] = prod;
         a_reg = vld1q_s32(&a[i+4]);
         b_reg = vld1q_s32(&b[i+4]);
         prod = vmulq_s32(a_reg, b_reg);
        temp[ctr+1] = prod;
        ctr+=2;

    }
    for (int j = 0; j < i/4; ++j) {
        result += vgetq_lane_s32(temp[j], 0);
        result += vgetq_lane_s32(temp[j], 1);
        result += vgetq_lane_s32(temp[j], 2);
        result += vgetq_lane_s32(temp[j], 3);
    }    
    for(; i < n; i++) {
        result += a[i] * b[i];
    }
    auto end = high_resolution_clock::now();
    auto dur = duration_cast<microseconds>(end - start);
    cout << "simd dot product = " << result << "\t duration: " << dur.count() << " microseconds" << endl;
    return result;
}

//Given a string
int non_simd_character_search(string s) {

    return 1;
}
int main() {
    array<int, n> a;
    array<int, n> b;
    for( int i = 0; i < n; i++) {
        a[i] = i+1 % 10;
        b[i] = i+1 % 10;
    }
    //print_vec(v1, v2);
    dot(a, b);
    simd_dot(a, b);
    return 0;
}