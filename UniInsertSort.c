#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <inttypes.h>

#define DBL_CMP_EPS 1e-5
// #define MAXLEN 1000

unsigned getbits(unsigned int x, int p, int n);
void AnyArrPrint(void *data, size_t mas_size, size_t el_size);
void Swap(void *a, void *b, size_t el_size);
int CmpInt(const void* value_a, const void* value_b);
int CmpDbl(const void* value_a, const void* value_b);
bool DoubleZeroCheck(double dbl);
void MySort(void *arr, size_t arr_size, size_t el_size, 
            int (*Comparator)(const void *left_value, const void *right_value));

int main() {
    int massive_int[] = {12, 21, 21, -23, 3, 4, 10, 1};
    double massive_dbl[] = {3.21, -21.0, 12e-3, 0.0, 1.0, -1.0};
    MySort(massive_dbl, sizeof(massive_dbl) / sizeof(massive_dbl[0]), sizeof(massive_dbl[0]), &CmpDbl);
    for (size_t i = 0; i < sizeof(massive_dbl) / sizeof(massive_dbl[0]); i++) {
        printf("%lg ", massive_dbl[i]);
    }
    putchar('\n');
    MySort(massive_int, sizeof(massive_int) / sizeof(massive_int[0]), sizeof(massive_int[0]), &CmpInt);
    for (size_t i = 0; i < sizeof(massive_int) / sizeof(massive_int[0]); i++) {
        printf("%d ", massive_int[i]);
    }
    putchar('\n');
    return 0;
}

unsigned getbits(unsigned int x, int p, int n) {
    return (x >> (p + 1 - n)) & ~(~0 << n);
}

void AnyArrPrint(void *data, size_t mas_size, size_t el_size) {
    // char s[MAXLEN] = {};
    for (size_t i = 0; i < mas_size; i++) {
        printf("%d ", *(int *)((size_t)data + el_size * i));
    }
    putchar('\n');
}

void MySort(void *arr, size_t arr_size, size_t el_size, int (*Comparator)(const void *left_value, const void *right_value)) {
    for (size_t i = 1; i < arr_size; i++) {
        size_t k = i;
        while (k > 0) {
            void *left_ptr = (void *)((size_t)arr + el_size * (k - 1));
            void *right_ptr = (void *)((size_t)arr + el_size * k);
            if ((*Comparator)(left_ptr, right_ptr) <= 0) {
                break;
            }
            else {
                Swap(left_ptr, right_ptr, el_size);
                k--;
            }
        }
    }
}

void Swap(void *a, void *b, size_t el_size) {
    uint8_t tmp = '\0';
    for (size_t i = 0; i < el_size; i++) {
        tmp = *(uint8_t *)((size_t)a + i);
        *(uint8_t *)((size_t)a + i) = *(uint8_t *)((size_t)b + i);
        *(uint8_t *)((size_t)b + i) = tmp;
    }
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