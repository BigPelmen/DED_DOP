#define DEF_STACK_TYPE double
#define STACK_DEBUG
#include "stack.h"

#define GET_NAME(var) #var

void IntStackPrint(FILE *f_log, const void *element);
void DblStackPrint(FILE *f_log, const void *element);

int main() {
    FILE *file_log = LogOpen("LogFile.txt", "w");
    stack_t my_stck = {};
    StackInit(&my_stck, 3, __FILE__, GET_NAME(my_stck), __LINE__, file_log, &DblStackPrint);
    LogStackDump(file_log, &my_stck, &DblStackPrint);
    StackStatsPrint(&my_stck);
    my_stck.capacity = 15;

    for (size_t i = 0; i < 10; i++) {
        StackPush(&my_stck, (i + 1) * 10.01);
    }
    StackStatsPrint(&my_stck);
    LogStackDump(file_log, &my_stck, &DblStackPrint);
    for (size_t i = 0; i < 10; i++) {
        StackPop(&my_stck);
    }
    StackStatsPrint(&my_stck);
    LogStackDump(file_log, &my_stck, &DblStackPrint);
    StackStatsPrint(&my_stck);
    StackDestroy(&my_stck, __FILE__, __LINE__);
    LogStackDump(file_log, &my_stck, &DblStackPrint);
    StackStatsPrint(&my_stck);
    fclose(file_log);
    return 0;
}

void IntStackPrint(FILE *f_log, const void *element) {
    fprintf(f_log, "%d", *(int *)element);
}

void DblStackPrint(FILE *f_log, const void *element) {
    fprintf(f_log, "%lg", *(double *)element);
}