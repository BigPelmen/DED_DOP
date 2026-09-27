#define DEF_STACK_TYPE int
// #define STACK_DEBUG
#include "stack.h"

int main() {
    stack_t my_stck = {};
    StackInit(&my_stck, 12);
    StackStatsPrint(&my_stck);
    for (size_t i = 0; i < 25; i++) {
        StackPush(&my_stck, (i + 1) * 10);
    }
    StackStatsPrint(&my_stck);
    for (size_t i = 0; i < my_stck.pos_stck; i++) {
        printf("%d ", my_stck.stck[i]);
    }
    putchar('\n');
    for (size_t i = 0; i < 25; i++) {
        StackPop(&my_stck);
    }
    StackStatsPrint(&my_stck);
    StackDestroy(&my_stck);
    StackStatsPrint(&my_stck);
    return 0;
}