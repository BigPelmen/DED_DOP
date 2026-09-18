#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define SIZEX 4
#define SIZEY 3
#define TEAMS 10

void PrintMatrix(int *Mat, size_t sx, size_t sy);
void PrintTriangle(int *Matches, size_t teams_amount);

int main() {
    int Matrix[SIZEY][SIZEX] = {
        {10, 11, 12, 13},
        {20, 21, 22, 23},
        {30, 31, 32, 33}
    };
    PrintMatrix((int *)Matrix, SIZEX, SIZEY);

    // int *Matches = (int *)calloc((size_t)(TEAMS * (TEAMS + 1) / 2), sizeof(int));
    int Matches[] = {
        1,
        2, -3,
        4, -5, -10,
        12, 20, 25, 0
    };
    int array[] = {0, 1, 2, 3, 4};
    printf("Matrix = %llu\n", sizeof((int *)Matrix));
    printf("Pointer = %llu\n", sizeof(Matches));
    printf("Array = %llu\n", sizeof(array));
    printf("Array pointer = %llu\n", sizeof((int *)array));
    size_t matches_size = sizeof(Matches) / sizeof(Matches[0]);
    int di = 1 + 8 * matches_size;
    size_t teammy = (-1 + sqrt(di)) / 2;
    PrintTriangle(Matches, teammy);
    // free(Matches);
}

void PrintMatrix(int *Mat, size_t sx, size_t sy) {
    assert(Mat);

    for (size_t y = 0; y < sy; y++) {
        for (size_t x = 0; x < sx; x++) {
            printf("%d ", *(Mat + y * sx + x));
        }
        putchar('\n');
    }
}

void PrintTriangle(int *Matches, size_t teams_amount) {
    assert(Matches);

    for (size_t i = 0; i < teams_amount; i++) {
        size_t area_above = i * (i + 1) / 2, mathes_in_line = i + 1;
        for (size_t match_num = 0; match_num < mathes_in_line; match_num++) {
            printf("%d ", Matches[area_above + match_num]);
        }
        putchar('\n');
    }
}