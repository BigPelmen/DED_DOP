#define PROC_IFS

// TODO PUSH DOES NOT WORK CORRECTLY WITH [ABX] COMBINATION - IT WILL JUST TAKE THE NUMBER OF THE REGISTR

// #define DEF_STACK_TYPE int
// #define STACK_DEBUG
// #define STACK_HASHES_ON
// #define STACK_CANARIES_ON
// #define STACK_STRICT_PTR
#include "stack.h"
#include "Comms.h"

#ifdef PROC_IFS
    #define ON_PRC_IFS(...) __VA_ARGS__
#else
    #define ON_PRC_IFS(...)
#endif

typedef enum {
    NORMAL_PERFORM = 0,
    PUSH_ERROR_GOT = 1 << 0,
    POP_ERROR_GOT = 1 << 1,
    ADD_ERROR_GOT = 1 << 2,
    SUB_ERROR_GOT = 1 << 3,
    MULT_ERROR_GOT = 1 << 4,
    DIV_ERROR_GOT = 1 << 5,
    JMP_ERROR_GOT = 1 << 6,
    OUT_ERROR_GOT = 1 << (sizeof(int) * 8 - 2),
    HLT_ERROR_GOT = 1 << (sizeof(int) * 8 - 1),
} exit_code_t;

typedef enum {
    OPERATION_FAIL = 0,
    OPERATION_OK = 1,
} got_operation_res_t;

#define REGS_AMOUNT 8
#define RAM_PRC_SIZE 100
#define RAM_COEF 2

#define PRINT_PUSH_COM_FAIL(which_function) \
    fprintf(stderr, "%s function FAILED while pushing\n", which_function) 

// typedef struct {
//     int ax, bx, cx, dx;
// } regstrs_t;

typedef struct {
    int *byte_code;
    int pc;
    int *rgstr;
    int *ram;
    stack_t proc_stck;
} procsr_t;

int GetFDesc(const char *f_name, int flags, mode_t f_mode);
size_t GetFileSize(int fd);
int *ReadBytedBuf(char *buf, size_t lines_amount, size_t *real_len);
void ProcInit(procsr_t *proc, int *b_code ON_DBG(, int ln, FILE *file_log));
int RunCode(procsr_t *prc);
int RunOperation(procsr_t *prc);
int JmpCom(procsr_t *prc, int jmp_label);
int JaCom(procsr_t *prc, int jmp_label);
int JaeCom(procsr_t *prc, int jmp_label);
int JbCom(procsr_t *prc, int jmp_label);
int JbeCom(procsr_t *prc, int jmp_label);
int JeCom(procsr_t *prc, int jmp_label);
int JneCom(procsr_t *prc, int jmp_label);
int PushCom(stack_t *st, elem_t el);
int PoprCom(procsr_t *prc);
int PopmCom(procsr_t *prc);
int Multiply(stack_t *st);
int Divide(stack_t *st);
int Add(stack_t *st);
int Subtract(stack_t *st);
int OutputRes(stack_t *st);
int SoftHalt(stack_t *st);
void ProcDestroy(procsr_t *prc ON_DBG(, const char *f_name, int ln));
void IntStackPrint(FILE *f_log, const void *element);

int main() {
    procsr_t proc = {};
    FILE *file_log = LogOpen("LogFile.txt", "w");
    if (file_log == NULL) {
        return 1;
    }
    int fd_read = GetFDesc("ByteCode.txt", O_RDONLY, S_IRUSR);
    if (fd_read < 0) {
        return 1;
    }
    size_t f_read_size = GetFileSize(fd_read);
    if (f_read_size == 0) {
        return 1;
    }
    char *f_buf = (char *)calloc(f_read_size + 1, sizeof(char));
    if (f_buf == NULL) {
        return 1;
    }
    int transferred = read(fd_read, f_buf, f_read_size + 1) + 1;
    if (transferred <= 0) {
        return 1;
    }
    char *res_buf_ptr = (char *)realloc(f_buf, transferred);
    if (res_buf_ptr == NULL) {
        return 1;
    }
    res_buf_ptr[transferred - 1] = '\0';
    size_t real_len = 0;
    int *by_co = ReadBytedBuf(res_buf_ptr, 100, &real_len);
    if (by_co == NULL) {
        return 1;
    }
    ProcInit(&proc, by_co ON_DBG(, __LINE__, file_log));
    RunCode(&proc);
    ProcDestroy(&proc ON_DBG(, __FILE__, __LINE__));
    return 0;
}

int *ReadBytedBuf(char *buf, size_t lines_amount, size_t *real_len) {
    assert(buf);
    assert(real_len);

    *real_len = 0;
    size_t max_len = CODE_LINE_MULTIPLIER * lines_amount;
    int *b_c_tmp = (int *)calloc(max_len, sizeof(int));
    if (b_c_tmp == NULL) {
        fprintf(stderr, "CALLOC FAILED WHILE TRYING TO CREATE THE BYTE CODE BUFFER\n");
        return NULL;
    }
    while (*buf != '\0') {
        SpacesSkip(&buf);
        if (*buf == '\0') {
            break;
        }
        int sign = 1;
        if (*buf == '-') {
            sign = -1;
            buf++;
        }
        if (!isdigit(*buf)) {
            fprintf(stderr, "BAD BUFFER SYMBOL HAS BEEN FOUND \"%c\" on %p\n", *buf, buf);
            free(b_c_tmp);
            return NULL;
        }
        int com = 0;
        while (isdigit(*buf)) {
            com = com * 10 + ((*buf++) - '0');
        }
        if (*real_len >= max_len) {
            printf("%llu\n", *real_len);
            fprintf(stderr, "TOO LONG CODE IN BUFFER\n");
            free(b_c_tmp);
            return NULL;
        }
        b_c_tmp[(*real_len)++] = com * sign;
    }
    if (*real_len == 0) {
        fprintf(stderr, "DID NOT MANAGE TO READ THE BUFFER AND IMPLIED ITS LENGTH IS 0\n");
        free(b_c_tmp);
        return NULL;
    }

    int *b_c_res = (int *)realloc(b_c_tmp, (*real_len) * sizeof(int));
    if (b_c_res == NULL) {
        fprintf(stderr, "FAILED CREATING THE BYTED CODE FINAL MASSIVE\n");
        free(b_c_tmp);
        return NULL;
    }
    // printf("REAL LEN: %llu\n", *real_len);
    return b_c_res;
}

void ProcInit(procsr_t *prc, int *b_code ON_DBG(, int ln, FILE *file_log)) {
    assert(prc);
    ON_DBG(assert(file_log);)
    assert(b_code);

    StackInit(&prc->proc_stck, 10 
        ON_DBG(, __FILE__, GET_NAME(prc), ln, file_log, &IntStackPrint));
    prc->pc = 0;
    int *rgstry = (int *)calloc(REGS_AMOUNT, sizeof(int));
    if (rgstry == NULL) {
        prc->byte_code = NULL;
        prc->rgstr = NULL;
        return ;
    }
    for (size_t i = 0; i < REGS_AMOUNT; i++) {
        rgstry[i] = (i + 1) * 100;
    }
    int *ramy = (int *)calloc(RAM_PRC_SIZE, sizeof(int));
    if (ramy == NULL) {
        prc->ram = NULL;
        prc->byte_code = NULL;
        prc->rgstr = NULL;
        return ;
    }
    prc->ram = ramy;
    prc->rgstr = rgstry;
    prc->byte_code = b_code;
    return ;
}

void ProcDestroy(procsr_t *prc ON_DBG(, const char *f_name, int ln)) {
    assert(prc);
    ON_DBG(assert(f_name);)

    if (StackDestroy(&prc->proc_stck ON_DBG(, f_name, ln)) != EGENERALLY_STACK_OK) {
        fprintf(stderr, "PROCESS ENDED WITH UNNORMAL STACK\n");
    }
    free(prc->byte_code);
    prc->byte_code = NULL;
    free(prc->rgstr);
    prc->rgstr = NULL;
    free(prc->ram);
    prc->ram = NULL;
    return ;
}

int RunCode(procsr_t *prc) {
    assert(prc);

    int offset = 0;
    int err = 0;
    while (true) {
        offset = GetCodeOffset(prc->byte_code[prc->pc]);
        if (offset == HLT_COM_OFFSET) {
            if ((err & ~NORMAL_PERFORM) != 0) {
                fprintf(stderr, "PROCESS ENDED WITH EXIT CODE: %#x", err);
            }
            return 0;
        }
        err |= RunOperation(prc);
        prc->pc += offset;
    }
}

int RunOperation(procsr_t *prc) {
    assert(prc);

    switch (prc->byte_code[prc->pc]) {
    case OUT_COM:
        return (OutputRes(&prc->proc_stck)) ? NORMAL_PERFORM : OUT_ERROR_GOT;
    case HLT_COM:
        return (SoftHalt(&prc->proc_stck)) ? NORMAL_PERFORM : HLT_ERROR_GOT;
    case PUSH_COM:
        return (PushCom(&prc->proc_stck, prc->byte_code[prc->pc + 1])) ? NORMAL_PERFORM : PUSH_ERROR_GOT;
    case PUSHR_COM:
        return (PushCom(&prc->proc_stck, prc->rgstr[prc->byte_code[prc->pc + 1]])) ? NORMAL_PERFORM : PUSH_ERROR_GOT;
    case PUSHM_COM:
        return (PushCom(&prc->proc_stck, prc->ram[prc->byte_code[prc->pc + 1]])) ? NORMAL_PERFORM : PUSH_ERROR_GOT;
    case POPR_COM:
        return (PoprCom(prc)) ? NORMAL_PERFORM : POP_ERROR_GOT;
    case POPM_COM:
        return (PopmCom(prc)) ? NORMAL_PERFORM : POP_ERROR_GOT;
    case ADD_COM:
        return (Add(&prc->proc_stck)) ? NORMAL_PERFORM : ADD_ERROR_GOT;
    case SUB_COM:
        return (Subtract(&prc->proc_stck)) ? NORMAL_PERFORM : SUB_ERROR_GOT;
    case MULT_COM:
        return (Multiply(&prc->proc_stck)) ? NORMAL_PERFORM : MULT_ERROR_GOT;
    case DIV_COM:
        return (Divide(&prc->proc_stck)) ? NORMAL_PERFORM : DIV_ERROR_GOT;
    case JMP_COM:
        return (JmpCom(prc, prc->byte_code[prc->pc + 1])) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JA_COM:
        return (JaCom(prc, prc->byte_code[prc->pc + 1])) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JAE_COM:
        return (JaeCom(prc, prc->byte_code[prc->pc + 1])) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JB_COM:
        return (JbCom(prc, prc->byte_code[prc->pc + 1])) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JBE_COM:
        return (JbeCom(prc, prc->byte_code[prc->pc + 1])) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JE_COM:
        return (JeCom(prc, prc->byte_code[prc->pc + 1])) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JNE_COM:
        return (JneCom(prc, prc->byte_code[prc->pc + 1])) ? NORMAL_PERFORM : JMP_ERROR_GOT;    
    case WRONG_COMMAND_COM:
        return HLT_ERROR_GOT;
    };
    return HLT_ERROR_GOT;
}

int JmpCom(procsr_t *prc, int jmp_label) {
    assert(prc);

    prc->pc = jmp_label;
    ON_PRC_IFS(
        if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
            fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
            return OPERATION_FAIL;
        }
    )
    return OPERATION_OK;
}

int JaCom(procsr_t *prc, int jmp_label) {
    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 > el2) {
        prc->pc = jmp_label;
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += 2;
    }
    return OPERATION_OK;
}

int JaeCom(procsr_t *prc, int jmp_label) {
    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 >= el2) {
        prc->pc = jmp_label;
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += 2;
    }
    return OPERATION_OK;
}

int JbCom(procsr_t *prc, int jmp_label) {
    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 < el2) {
        prc->pc = jmp_label;
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += 2;
    }
    return OPERATION_OK;
}

int JbeCom(procsr_t *prc, int jmp_label) {
    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 <= el2) {
        prc->pc = jmp_label;
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += 2;
    }
    return OPERATION_OK;
}

int JeCom(procsr_t *prc, int jmp_label) {
    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 == el2) {
        prc->pc = jmp_label;
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += 2;
    }
    return OPERATION_OK;
}

int JneCom(procsr_t *prc, int jmp_label) {
    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 != el2) {
        prc->pc = jmp_label;
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += 2;
    }
    return OPERATION_OK;
}

int PushCom(stack_t *st, elem_t el) {
    assert(st);

    ON_PRC_IFS(
        if (StackPush(st, el) != EGENERALLY_STACK_OK || 
            st->cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            return OPERATION_FAIL;
        }
    )

    return OPERATION_OK;
}

int PoprCom(procsr_t *prc) {
    assert(prc);

    int el = StackPop(&prc->proc_stck);
    prc->rgstr[prc->byte_code[prc->pc + 1]] = el;
    return OPERATION_OK;
}

int PopmCom(procsr_t *prc) {
    assert(prc);

    int el = StackPop(&prc->proc_stck);
    prc->ram[prc->byte_code[prc->pc + 1]] = el;
    return OPERATION_OK;
}

int Multiply(stack_t *st) {
    assert(st);
    
    elem_t el1 = StackPop(st), el2 = StackPop(st);
    ON_PRC_IFS(
        if (StackPush(st, el1 * el2) != EGENERALLY_STACK_OK || 
            st->cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            return OPERATION_FAIL;
        }
    )
    return OPERATION_OK;
}

int Divide(stack_t *st) {
    assert(st);
    
    elem_t el1 = StackPop(st), el2 = StackPop(st);
    if (el1 == 0) {
        fprintf(stderr, "ERROR: division by zero. %d / %d\n", el2, el1);
        StackPush(st, INT_MAX);
        return OPERATION_FAIL;
    }
    ON_PRC_IFS(
        if (StackPush(st, el2 / el1) != EGENERALLY_STACK_OK || 
            st->cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            return OPERATION_FAIL;
        }
    )
    return OPERATION_OK;
}

int Add(stack_t *st) {
    assert(st);
    
    elem_t el1 = StackPop(st), el2 = StackPop(st);
    ON_PRC_IFS(
        if (StackPush(st, el1 + el2) != EGENERALLY_STACK_OK || 
            st->cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            return OPERATION_FAIL;
        }
    )
    return OPERATION_OK;
}

int Subtract(stack_t *st) {
    assert(st);
    
    elem_t el1 = StackPop(st), el2 = StackPop(st);
    ON_PRC_IFS(
        if (StackPush(st, el2 - el1) != EGENERALLY_STACK_OK || 
            st->cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            return OPERATION_FAIL;
        }
    )
    return OPERATION_OK;    
}

int OutputRes(stack_t *st) {
    assert(st);
    
    elem_t res_el = StackPop(st);
    ON_PRC_IFS(
        if (st->cur_err != EGENERALLY_STACK_OK) {
            fprintf(stderr, "Unsafe pop try in %s function while getting output\n", __func__);
            ON_NSF_TR(return OPERATION_FAIL;) 
        }
    )
    fprintf(stdout, "|OUT: %5d|\n", res_el);
    return OPERATION_OK;
}

int SoftHalt(stack_t *st) {
    assert(st);
    
    if (StackDestroy(st ON_DBG(, __func__, __LINE__)) == EGENERALLY_STACK_OK) {
        return OPERATION_OK;
    }
    fprintf(stderr, "PROCESS MAY HAS GOT THE FATAL ERROR AND FINISHED UNNORMALLY\n");
    return OPERATION_FAIL;
}

void IntStackPrint(FILE *f_log, const void *element) {
    assert(f_log);
    assert(element);

    fprintf(f_log, "%d", *(int *)element);
}