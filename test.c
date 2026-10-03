#define DEF_STACK_TYPE double
#define STACK_DEBUG
#define STACK_HASHES_ON
#define STACK_CANARIES_ON
#define STACK_UNSAFE_TRY_KILL_ON
#include "stack.h"

#define GET_NAME(var) #var

void IntStackPrint(FILE *f_log, const void *element);
void DblStackPrint(FILE *f_log, const void *element);

int main() {
    FILE *file_log = LogOpen("LogFile.txt", "w");
    stack_t my_stck = {};
    StackInit(&my_stck, 3 ON_DBG(, __FILE__, GET_NAME(my_stck), __LINE__, file_log, &DblStackPrint));
    LogStackDump(file_log, &my_stck, &DblStackPrint);
    StackStatsPrint(&my_stck);
    for (size_t i = 0; i < 10; i++) {
        StackPush(&my_stck, (i + 1) * 10.01);
    }
    StackStatsPrint(&my_stck);
    LogStackDump(file_log, &my_stck, &DblStackPrint);
    elem_t tmp = 0;
    for (size_t i = 0; i < 10; i++) {
        tmp = StackPop(&my_stck);
    }
    printf("%lg\n", tmp);
    StackStatsPrint(&my_stck);
    LogStackDump(file_log, &my_stck, &DblStackPrint);
    StackStatsPrint(&my_stck);
    StackDestroy(&my_stck ON_DBG(, __FILE__, __LINE__));
    StackStatsPrint(&my_stck);

    LogStackDump(file_log, &my_stck, &DblStackPrint);
    fclose(file_log);
    return 0;
}

void IntStackPrint(FILE *f_log, const void *element) {
    fprintf(f_log, "%d", *(int *)element);
}

void DblStackPrint(FILE *f_log, const void *element) {
    fprintf(f_log, "%lg", *(double *)element);
}