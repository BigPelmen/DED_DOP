#define DEF_STACK_TYPE int
#include "stack.h"

int main() {
    stack_t my_stck = {};
    StackInit(&my_stck, 12);
    for (size_t i = 0; i < 25; i++) {
        StackPush(&my_stck, (i + 1) * 10);
    }
    for (size_t i = 0; i < my_stck.pos_stck; i++) {
        printf("%d ", my_stck.stck[i]);
    }
    putchar('\n');
    for (size_t i = 0; i < 25; i++) {
        StackPop(&my_stck);
    }
    // printf("pos = %llu, last element has been %d\n", my_stck.pos_stck, tmp);
    for (size_t i = 0; i < my_stck.capacity; i++) {
        printf("%d ", my_stck.stck[i]);
    }
    
    return 0;
}