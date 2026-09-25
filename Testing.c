#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

#define DBL_CMP_EPS 1e-5

typedef int (*comparator_t)(const void *, const void *);

void IntrinsicQuickSort(void *arr, size_t el_size, 
    ssize_t left, ssize_t right, comparator_t Comparator);
void MyQuickSort(void *arr, size_t arr_size, size_t el_size, comparator_t Comparator);
int CmpInt(const void* value_a, const void* value_b);
int CmpDbl(const void* value_a, const void* value_b);
bool DoubleZeroCheck(double dbl);
void Swap(void *a, void *b, size_t el_size);

int main() {
    int massive_int[] = {12, 21, 21, -23, 3, 4, 10, 1};
    double massive_dbl[] = {3.21, -21.0, 12e-3, 0.0, 1.0, -1.0};
    MyQuickSort(massive_dbl, sizeof(massive_dbl) / sizeof(massive_dbl[0]), sizeof(massive_dbl[0]), &CmpDbl);
    for (size_t i = 0; i < sizeof(massive_dbl) / sizeof(massive_dbl[0]); i++) {
        printf("%lg ", massive_dbl[i]);
    }
    putchar('\n');
    MyQuickSort(massive_int, sizeof(massive_int) / sizeof(massive_int[0]), sizeof(massive_int[0]), &CmpInt);
    for (size_t i = 0; i < sizeof(massive_int) / sizeof(massive_int[0]); i++) {
        printf("%d ", massive_int[i]);
    }
    putchar('\n');
    return 0;
}

int CmpInt(const void* value_a, const void* value_b) {
    int a = *(const int*)value_a, b = *(const int*)value_b;
    return (a == b) ? 0 : ((a > b) ? 1 : -1);
}

int CmpDbl(const void* value_a, const void* value_b) {
    double a = *(const double*)value_a, b = *(const double*)value_b;
    bool are_equal = DoubleZeroCheck(a - b);
    if (are_equal) {
        return 0;
    }
    else if (a > b) {
        return 1;
    }
    else {
        return -1;
    }
}

bool DoubleZeroCheck(double dbl) {
    return fabs(dbl) < DBL_CMP_EPS;
}

void MyQuickSort(void *arr, size_t arr_size, size_t el_size, comparator_t Comparator) {
    ssize_t left = 0, right = (ssize_t)arr_size - 1;
    IntrinsicQuickSort(arr, el_size, left, right, Comparator);
}

void IntrinsicQuickSort(void *arr, size_t el_size, 
    ssize_t left, ssize_t right, comparator_t Comparator) {
    if (left >= right) {
        return;
    }
    ssize_t pivo = (left + right) / 2;
    ssize_t last = left;
    void *pivo_ptr = (void *)(((uint8_t *)arr) + el_size * pivo);
    for (ssize_t i = left; i < right + 1; i++) {
        void *left_ptr = (void *)(((uint8_t *)arr) + el_size * i);
        if ((*Comparator)(left_ptr, pivo_ptr) < 0) {
            Swap(left_ptr, (void *)(((uint8_t *)arr) + el_size * (last++)), el_size);
        }
    }
    Swap((void *)(((uint8_t *)arr) + el_size * (++last)), pivo_ptr, el_size);
    if (left < last - 1) {
        IntrinsicQuickSort(arr, el_size, left, last - 1, Comparator);
    }
    if (last + 1 < right) {
        IntrinsicQuickSort(arr, el_size, last + 1, right, Comparator);
    }
}

void Swap(void *a, void *b, size_t el_size) {
    uint8_t tmp = '\0';
    uint8_t *a_ptr = (uint8_t *)(a);
    uint8_t *b_ptr = (uint8_t *)(b);
    for (size_t i = 0; i < el_size; i++) {
        tmp = a_ptr[i];
        a_ptr[i] = b_ptr[i];
        b_ptr[i] = tmp;
    }
}