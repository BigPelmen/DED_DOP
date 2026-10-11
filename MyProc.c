#define PROC_IFS

#define DEF_STACK_TYPE int
#define STACK_DEBUG
// #define STACK_HASHES_ON
// #define STACK_CANARIES_ON
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
    ADD_ERROR_GOT = 1 << 1,
    SUB_ERROR_GOT = 1 << 2,
    MULT_ERROR_GOT = 1 << 3,
    DIV_ERROR_GOT = 1 << 4,
    OUT_ERROR_GOT = 1 << (sizeof(int) * 8 - 2),
    HLT_ERROR_GOT = 1 << (sizeof(int) * 8 - 1),
} exit_code_t;

typedef enum {
    OPERATION_FAIL,
    OPERATION_OK,
} got_operation_res_t;

#define PRINT_PUSH_COM_FAIL(which_function) \
    fprintf(stderr, "%s function FAILED while pushing\n", which_function) 

typedef struct {
    int ax, bx, cx, dx;
} regstrs_t;

typedef struct {
    int *byte_code;
    int pc;
    regstrs_t rgstr;
    stack_t proc_stck;
} procsr_t;

int GetFDesc(const char *f_name, int flags, mode_t f_mode);
size_t GetFileSize(int fd);
int *ReadBytedBuf(char *buf, size_t lines_amount, size_t *real_len);
void ProcInit(procsr_t *proc, int *b_code ON_DBG(, int ln, FILE *file_log));
int RunCode(procsr_t *prc);
int CodeOffset(int com);
int RunOperation(procsr_t *prc);
int PushCom(stack_t *st, elem_t el);
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
    int *by_co = ReadBytedBuf(res_buf_ptr, 3, &real_len);
    if (by_co == NULL) {
        return 1;
    }
    ProcInit(&proc, by_co ON_DBG(, __LINE__, file_log));
    RunCode(&proc);
    ProcDestroy(&proc, __FILE__, __LINE__);
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
        while (isspace(*buf) && *buf != '\0') {
            buf++;
        }
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
    return b_c_res;
}

void ProcInit(procsr_t *prc, int *b_code ON_DBG(, int ln, FILE *file_log)) {
    assert(prc);
    ON_DBG(assert(file_log);)

    StackInit(&prc->proc_stck, 32 
        ON_DBG(, __FILE__, GET_NAME(prc), ln, file_log, &IntStackPrint));
    prc->pc = 0;
    prc->rgstr.ax = 0, prc->rgstr.bx = 0, prc->rgstr.cx = 0, prc->rgstr.dx = 0;
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
    return ;
}

int RunCode(procsr_t *prc) {
    assert(prc);

    int offset = 0;
    int err = 0;
    while (true) {
        offset = CodeOffset(prc->byte_code[prc->pc]);
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
        return (OutputRes(&prc->proc_stck) == OPERATION_FAIL) ? OUT_ERROR_GOT : NORMAL_PERFORM;
    case HLT_COM:
        return (SoftHalt(&prc->proc_stck) == OPERATION_FAIL) ? HLT_ERROR_GOT : NORMAL_PERFORM;
    case PUSH_COM:
        return (PushCom(&prc->proc_stck, prc->byte_code[prc->pc + 1]) == OPERATION_FAIL) ? 
            PUSH_ERROR_GOT : NORMAL_PERFORM;
    case ADD_COM:
        return (Add(&prc->proc_stck) == OPERATION_FAIL) ? ADD_ERROR_GOT : NORMAL_PERFORM;
    case SUB_COM:
        return (Subtract(&prc->proc_stck) == OPERATION_FAIL) ? SUB_ERROR_GOT : NORMAL_PERFORM;
    case MULT_COM:
        return (Multiply(&prc->proc_stck) == OPERATION_FAIL) ? MULT_ERROR_GOT : NORMAL_PERFORM;
    case DIV_COM:
        return (Divide(&prc->proc_stck) == OPERATION_FAIL) ? DIV_ERROR_GOT : NORMAL_PERFORM;
    case WRONG_COMMAND_COM:
        return HLT_ERROR_GOT;
    }
    return HLT_ERROR_GOT;
}

int CodeOffset(int com) {
    switch (com) {
    case OUT_COM:
        return 1;
    case HLT_COM:
        return HLT_COM_OFFSET;
    case PUSH_COM:
        return 2;
    case ADD_COM:
        return 1;
    case SUB_COM:
        return 1;
    case MULT_COM:
        return 1;
    case DIV_COM:
        return 1;
    case WRONG_COMMAND_COM:
        return 1;
    }
    return 1;
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