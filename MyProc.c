#define PROC_IFS

// TODO PUSH DOES NOT WORK CORRECTLY WITH [ABX] COMBINATION - IT WILL JUST TAKE THE NUMBER OF THE REGISTR

#define DEF_STACK_TYPE int
#define STACK_DEBUG
#define STACK_HASHES_ON
#define STACK_CANARIES_ON
#define STACK_STRICT_PTR
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
    IN_ERROR_GOT = 1 << 7,
    CALL_ERROR_GOT = 1 << 8,
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

typedef struct {
    int *byte_code;
    int pc;
    int *rgstr;
    int *ram;
    int prc_err;
    bool is_active;
    stack_t proc_stck;
    stack_t funcs_stck;
} procsr_t;

// SECTION - Functions prototypes

bool FilesAreWrong(FILE **f_log, int *fd_read, size_t *f_read_size);
int GetFDesc(const char *f_name, int flags, mode_t f_mode);
size_t GetFileSize(int fd);
int *ReadBytedBuf(char *buf, size_t lines_amount, size_t *real_len);
void ProcInit(procsr_t *proc, int *b_code ON_DBG(, int ln, FILE *file_log));
int RunCode(procsr_t *prc);
int RunOperation(procsr_t *prc);
int JmpCom(procsr_t *prc);
int JaCom(procsr_t *prc);
int JaeCom(procsr_t *prc);
int JbCom(procsr_t *prc);
int JbeCom(procsr_t *prc);
int JeCom(procsr_t *prc);
int JneCom(procsr_t *prc);
int PushCom(procsr_t *prc);
int PushrCom(procsr_t *prc);
int PushmCom(procsr_t *prc);
int PoprCom(procsr_t *prc);
int PopmCom(procsr_t *prc);
int Multiply(procsr_t *prc);
int Divide(procsr_t *prc);
int Add(procsr_t *prc);
int Subtract(procsr_t *prc);
int OutputRes(procsr_t *prc);
int InputRes(procsr_t *prc);
int CallCom(procsr_t *prc);
int RetCom(procsr_t *prc);
int SoftHalt(procsr_t *prc);
void ProcDestroy(procsr_t *prc ON_DBG(, const char *f_name, int ln));
void IntStackPrint(FILE *f_log, const void *element);

// !SECTION
// SECTION - main

int main() {
    procsr_t proc = {};
    FILE *file_log = NULL;
    int fd_read = -1;
    size_t f_read_size = 0;
    if (FilesAreWrong(&file_log, &fd_read, &f_read_size)) {
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
    int *by_co = ReadBytedBuf(res_buf_ptr, 200, &real_len);
    if (by_co == NULL) {
        return 1;
    }
    ProcInit(&proc, by_co ON_DBG(, __LINE__, file_log));
    RunCode(&proc);
    ProcDestroy(&proc ON_DBG(, __FILE__, __LINE__));
    close(fd_read), fclose(file_log);
    return 0;
}

// !SECTION
// SECTION - Reader functions

bool FilesAreWrong(FILE **f_log, int *fd_read, size_t *f_read_size) {
    assert(f_log);
    assert(fd_read);
    assert(f_read_size);

    *f_log = LogOpen("LogFile.txt", "w");
    if (*f_log == NULL) {
        fprintf(stderr, "Log file is not correct\n");
        return true;
    }
    *fd_read = GetFDesc("ByteCode.txt", O_RDONLY, S_IRUSR);
    if (*fd_read < 0) {
        fprintf(stderr, "Byte code file descriptor is wrong\n");
        return true;
    }
    *f_read_size = GetFileSize(*fd_read);
    if (*f_read_size == 0) {
        fprintf(stderr, "Did not manage to get byte code file size\n");
        return true;
    }
    return false;
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

// !SECTION
// SECTION - Processor procedure functions

void ProcInit(procsr_t *prc, int *b_code ON_DBG(, int ln, FILE *file_log)) {
    assert(prc);
    ON_DBG(assert(file_log);)
    assert(b_code);

    StackInit(&prc->proc_stck, 64 
        ON_DBG(, __FILE__, GET_NAME(proc_stck), ln, file_log, &IntStackPrint));
    StackInit(&prc->funcs_stck, 64 
        ON_DBG(, __FILE__, GET_NAME(funcs_stck), ln, file_log, &IntStackPrint));
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
    prc->prc_err = NORMAL_PERFORM;
    prc->is_active = false;
    return ;
}

void ProcDestroy(procsr_t *prc ON_DBG(, const char *f_name, int ln)) {
    assert(prc);
    ON_DBG(assert(f_name);)

    if (StackDestroy(&prc->proc_stck ON_DBG(, f_name, ln)) != EGENERALLY_STACK_OK) {
        fprintf(stderr, "PROCESS ENDED WITH UNNORMAL STACK\n");
    }
    if (StackDestroy(&prc->funcs_stck ON_DBG(, f_name, ln)) != EGENERALLY_STACK_OK) {
        fprintf(stderr, "PROCESS ENDED WITH UNNORMAL FUNCTIONS STACK\n");
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

    prc->is_active = true;
    while (prc->is_active) {
        prc->prc_err |= RunOperation(prc);
    }
    if ((prc->prc_err & ~NORMAL_PERFORM) != 0) {
        fprintf(stderr, "PROCESS ENDED WITH EXIT CODE: %#x", prc->prc_err);
    }
    return 0;
}

int RunOperation(procsr_t *prc) {
    assert(prc);

    switch (prc->byte_code[prc->pc]) {
    case OUT_COM:
        return (OutputRes(prc)) ? NORMAL_PERFORM : OUT_ERROR_GOT;
    case HLT_COM:
        return (SoftHalt(prc)) ? NORMAL_PERFORM : HLT_ERROR_GOT;
    case PUSH_COM:
        return (PushCom(prc)) ? NORMAL_PERFORM : PUSH_ERROR_GOT;
    case PUSHR_COM:
        return (PushrCom(prc)) ? NORMAL_PERFORM : PUSH_ERROR_GOT;
    case PUSHM_COM:
        return (PushmCom(prc)) ? NORMAL_PERFORM : PUSH_ERROR_GOT;
    case POPR_COM:
        return (PoprCom(prc)) ? NORMAL_PERFORM : POP_ERROR_GOT;
    case POPM_COM:
        return (PopmCom(prc)) ? NORMAL_PERFORM : POP_ERROR_GOT;
    case ADD_COM:
        return (Add(prc)) ? NORMAL_PERFORM : ADD_ERROR_GOT;
    case SUB_COM:
        return (Subtract(prc)) ? NORMAL_PERFORM : SUB_ERROR_GOT;
    case MULT_COM:
        return (Multiply(prc)) ? NORMAL_PERFORM : MULT_ERROR_GOT;
    case DIV_COM:
        return (Divide(prc)) ? NORMAL_PERFORM : DIV_ERROR_GOT;
    case JMP_COM:
        return (JmpCom(prc)) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JA_COM:
        return (JaCom(prc)) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JAE_COM:
        return (JaeCom(prc)) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JB_COM:
        return (JbCom(prc)) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JBE_COM:
        return (JbeCom(prc)) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JE_COM:
        return (JeCom(prc)) ? NORMAL_PERFORM : JMP_ERROR_GOT;
    case JNE_COM:
        return (JneCom(prc)) ? NORMAL_PERFORM : JMP_ERROR_GOT; 
    case IN_COM:
        return (InputRes(prc)) ? NORMAL_PERFORM : IN_ERROR_GOT;
    case CALL_COM:
        return (CallCom(prc)) ? NORMAL_PERFORM : CALL_ERROR_GOT;
    case RET_COM:
        return (RetCom(prc)) ? NORMAL_PERFORM : CALL_ERROR_GOT;
    case WRONG_COMMAND_COM:
        return HLT_ERROR_GOT;
    };
    return HLT_ERROR_GOT;
}

// !SECTION
// SECTION - JUMP functions

int JmpCom(procsr_t *prc) {

    prc->pc = prc->byte_code[prc->pc + 1];
    ON_PRC_IFS(
        if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
            fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
            return OPERATION_FAIL;
        }
    )
    return OPERATION_OK;
}

int JaCom(procsr_t *prc) {
    assert(prc);

    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 < el2) {
        prc->pc = prc->byte_code[prc->pc + 1];
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += ONE_ARG_OFFSET;
    }
    return OPERATION_OK;
}

int JaeCom(procsr_t *prc) {
    assert(prc);

    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 <= el2) {
        prc->pc = prc->byte_code[prc->pc + 1];
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += ONE_ARG_OFFSET;
    }
    return OPERATION_OK;
}

int JbCom(procsr_t *prc) {
    assert(prc);

    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 > el2) {
        prc->pc = prc->byte_code[prc->pc + 1];
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += ONE_ARG_OFFSET;
    }
    return OPERATION_OK;
}

int JbeCom(procsr_t *prc) {
    assert(prc);

    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 >= el2) {
        prc->pc = prc->byte_code[prc->pc + 1];
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += ONE_ARG_OFFSET;
    }
    return OPERATION_OK;
}

int JeCom(procsr_t *prc) {
    assert(prc);

    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 == el2) {
        prc->pc = prc->byte_code[prc->pc + 1];
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += ONE_ARG_OFFSET;
    }
    return OPERATION_OK;
}

int JneCom(procsr_t *prc) {
    assert(prc);

    int el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    if (el1 != el2) {
        prc->pc = prc->byte_code[prc->pc + 1];
        ON_PRC_IFS(
            if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
                fprintf(stderr, "WRONG LABEL: %d\n", prc->byte_code[prc->pc]);
                return OPERATION_FAIL;
            }
        )
    }
    else {
        prc->pc += ONE_ARG_OFFSET;
    }
    return OPERATION_OK;
}

// !SECTION
// SECTION - PUSH and POP functions

int PushCom(procsr_t *prc) {
    assert(prc);

    ON_PRC_IFS(
        if (StackPush(&prc->proc_stck, prc->byte_code[prc->pc + 1]) != 
            EGENERALLY_STACK_OK || prc->proc_stck.cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            prc->pc += ONE_ARG_OFFSET;
            return OPERATION_FAIL;
        }
    )
    prc->pc += ONE_ARG_OFFSET;
    return OPERATION_OK;
}

int PushrCom(procsr_t *prc) {
    assert(prc);

    ON_PRC_IFS(
        if (StackPush(&prc->proc_stck, prc->rgstr[prc->byte_code[prc->pc + 1]]) != 
            EGENERALLY_STACK_OK || prc->proc_stck.cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            prc->pc += ONE_ARG_OFFSET;
            return OPERATION_FAIL;
        }
    )
    prc->pc += ONE_ARG_OFFSET;
    return OPERATION_OK;
}

int PushmCom(procsr_t *prc) {
    assert(prc);

    ON_PRC_IFS(
        if (StackPush(&prc->proc_stck, prc->ram[prc->byte_code[prc->pc + 1]]) != 
            EGENERALLY_STACK_OK || prc->proc_stck.cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            prc->pc += ONE_ARG_OFFSET;
            return OPERATION_FAIL;
        }
    )
    prc->pc += ONE_ARG_OFFSET;
    return OPERATION_OK;
}

int PoprCom(procsr_t *prc) {
    assert(prc);

    prc->rgstr[prc->byte_code[prc->pc + 1]] = StackPop(&prc->proc_stck);
    prc->pc += ONE_ARG_OFFSET;
    return OPERATION_OK;
}

int PopmCom(procsr_t *prc) {
    assert(prc);

    prc->ram[prc->byte_code[prc->pc + 1]] = StackPop(&prc->proc_stck);
    prc->pc += ONE_ARG_OFFSET;
    return OPERATION_OK;
}

// !SECTION
// SECTION - Mathimatical functions

int Multiply(procsr_t *prc) {
    assert(prc);
    
    elem_t el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    prc->pc += NO_ARG_OFFSET;
    ON_PRC_IFS(
        if (StackPush(&prc->proc_stck, el1 * el2) != EGENERALLY_STACK_OK || 
            prc->proc_stck.cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            return OPERATION_FAIL;
        }
    )
    
    return OPERATION_OK;
}

int Divide(procsr_t *prc) {
    assert(prc);
    
    elem_t el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    prc->pc += NO_ARG_OFFSET;
    if (el1 == 0) {
        fprintf(stderr, "ERROR: division by zero. %d / %d\n", el2, el1);
        StackPush(&prc->proc_stck, INT_MAX);
        return OPERATION_FAIL;
    }
    ON_PRC_IFS(
        if (StackPush(&prc->proc_stck, el2 / el1) != EGENERALLY_STACK_OK || 
            prc->proc_stck.cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            return OPERATION_FAIL;
        }
    )
    
    return OPERATION_OK;
}

int Add(procsr_t *prc) {
    assert(prc);
    
    elem_t el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    prc->pc += NO_ARG_OFFSET;
    ON_PRC_IFS(
        if (StackPush(&prc->proc_stck, el1 + el2) != EGENERALLY_STACK_OK || 
            prc->proc_stck.cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            return OPERATION_FAIL;
        }
    )
    
    return OPERATION_OK;
}

int Subtract(procsr_t *prc) {
    assert(prc);
    
    elem_t el1 = StackPop(&prc->proc_stck), el2 = StackPop(&prc->proc_stck);
    prc->pc += NO_ARG_OFFSET;
    ON_PRC_IFS(
        if (StackPush(&prc->proc_stck, el2 - el1) != EGENERALLY_STACK_OK || 
            prc->proc_stck.cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            return OPERATION_FAIL;
        }
    )
    
    return OPERATION_OK;    
}

// !SECTION
// SECTION - System functions

int OutputRes(procsr_t *prc) {
    assert(prc);
    
    // printf("pop called from out\n");
    elem_t res_el = StackPop(&prc->proc_stck);
    ON_PRC_IFS(
        if (prc->proc_stck.cur_err != EGENERALLY_STACK_OK) {
            fprintf(stderr, "Unsafe pop try in %s function while getting output\n", __func__);
            ON_NSF_TR(return OPERATION_FAIL;) 
        }
    )
    fprintf(stdout, "|OUT: %5d|\n", res_el);
    prc->pc += NO_ARG_OFFSET;
    return OPERATION_OK;
}

int InputRes(procsr_t *prc) {
    assert(prc);

    elem_t res_el = 0;
    if (scanf("%d", &res_el)) {
        // ON_PRC_IFS(
        if (StackPush(&prc->proc_stck, res_el) != EGENERALLY_STACK_OK || 
            prc->proc_stck.cur_err != EGENERALLY_STACK_OK) {
            PRINT_PUSH_COM_FAIL(__func__);
            return OPERATION_FAIL;
        }
        // )
        prc->pc += NO_ARG_OFFSET;
        return OPERATION_OK;  
    }
    prc->pc += NO_ARG_OFFSET;
    return OPERATION_FAIL;
}

int CallCom(procsr_t *prc) {
    assert(prc);

    // printf("adr of ret <- %d\n", prc->pc + 2);
    if (StackPush(&prc->funcs_stck, prc->pc + 2) != EGENERALLY_STACK_OK || 
        prc->funcs_stck.cur_err != EGENERALLY_STACK_OK) {
        PRINT_PUSH_COM_FAIL(__func__);
        return OPERATION_FAIL;
    }
    prc->pc = prc->byte_code[prc->pc + 1];
    ON_PRC_IFS(
        if (0 > prc->byte_code[prc->pc] || prc->byte_code[prc->pc] >= WRONG_COMMAND_COM) {
            fprintf(stderr, "WRONG CALL LABEL: %d\n", prc->byte_code[prc->pc]);
            return OPERATION_FAIL;
        }
    )
    
    // LogStackDump(prc->funcs_stck.file_log, &prc->funcs_stck, &IntStackPrint);
    // LogStackDump(prc->funcs_stck.file_log, &prc->proc_stck, &IntStackPrint);
    return OPERATION_OK;
}

int RetCom(procsr_t *prc) {
    assert(prc);

    // printf("ret -> %d\n", ret_label);
    prc->pc = StackPop(&prc->funcs_stck);
    return OPERATION_OK;
}

int SoftHalt(procsr_t *prc) {
    assert(prc);
    
    prc->is_active = false;
    prc->pc += NO_ARG_OFFSET;
    if (prc->prc_err != NORMAL_PERFORM) {
        fprintf(stderr, "PROCESS MAY HAS GOT THE FATAL ERROR AND FINISHED UNNORMALLY\n");
        return OPERATION_FAIL;
    }

    LogStackDump(prc->funcs_stck.file_log, &prc->funcs_stck, &IntStackPrint);
    LogStackDump(prc->proc_stck.file_log, &prc->proc_stck, &IntStackPrint);
    return OPERATION_OK;
}

// !SECTION

void IntStackPrint(FILE *f_log, const void *element) {
    assert(f_log);
    assert(element);

    fprintf(f_log, "%d", *(int *)element);
}