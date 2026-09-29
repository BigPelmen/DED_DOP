#ifndef STACK_H
#define STACK_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

// #define STACK_DEBUG

#ifdef STACK_DEBUG
    #define ON_DBG(...) __VA_ARGS__
#else
    #define ON_DBG(...)
#endif

#ifdef DEF_STACK_TYPE
    typedef DEF_STACK_TYPE elem_t;
#else
    typedef int elem_t;
#endif

#define STACK_IS_OK 0
#define POISON_BYTE 0xAA

#ifdef DEF_STACK_TYPE
    typedef DEF_STACK_TYPE elem_t;
#else
    typedef int elem_t;
#endif

typedef enum {
    EGENERALLY_STACK_OK = STACK_IS_OK,
    ESIZE_UPPER_CAPACITY,
    ECAPACITY_IS_ZERO,
    EALREADY_INIT,
    ECALLOC,
    EREALLOC,
    EEMPTY_POP,
    EBAD_STACK_PTR,
    ECAPACITY_EXTR_CHANGE,
    EUNKNOWN,
} err_types_t;

typedef enum {
    STATUS_ACTIVE,
    STATUS_DESTROYED,
    STATUS_EMPTY,
} status_t;

typedef void (*frmttd_print_t)(FILE *log_file, const void *element);

typedef struct {
    ON_DBG(const char *f_name;
    const char *val_name;
    int line;
    FILE *file_log;
    frmttd_print_t LogPrinterFunc;)
    elem_t *stck;
    size_t pos_stck;
    size_t capacity;
    size_t reserve_capacity;
    int cur_err;
    int status_stck;
} stack_t;

#define PRINT_BOUNDARY(log_file) \
    fprintf(log_file, "--------------------------------------------------" \
        "--------------------------------------------------\n")

const size_t MAX_SAFE_CAPACITY = ULLONG_MAX / 2 - 1;

int StackGeneralCheck(stack_t *st);
elem_t *MakePoison(stack_t *st);
int StackInit(stack_t *st, size_t capac 
    ON_DBG(, const char *name_f, const char *name_v, int ln, FILE *logy_f, frmttd_print_t LogPrinter));
int InitCheck(stack_t *st);
int StackPush(stack_t *st, elem_t val);
void PushDoubler(stack_t *st);
elem_t StackPop(stack_t *st);
void PopDivide(stack_t *st, size_t *hyster_offset);
FILE *LogOpen(const char *f_name, const char *f_mode);
int StackDestroy(stack_t *st 
    ON_DBG(, const char *name_f, int ln));
void LogStackDump(FILE *log_file, stack_t *st, frmttd_print_t PrinterFunc);
const char *StackErrGet(int st_err);
const char *StackStatusGet(int st_status);
void StackStatsPrint(stack_t *st);

int StackGeneralCheck(stack_t *st) {
    assert(st);

    if (st->status_stck != STATUS_DESTROYED) {
        if (st->stck == NULL) {
            return EBAD_STACK_PTR;
        }
        if (st->capacity <= 0) {
            st->cur_err = ECAPACITY_IS_ZERO;
            return ECAPACITY_IS_ZERO;
        }
        if (st->pos_stck >= st->capacity) {
            st->cur_err = ESIZE_UPPER_CAPACITY;
            return ESIZE_UPPER_CAPACITY;
        }
        if (st->reserve_capacity != st->capacity) {
            st->cur_err = ECAPACITY_EXTR_CHANGE;
            return ECAPACITY_EXTR_CHANGE;
        }
    }
    return EGENERALLY_STACK_OK;
}

elem_t *MakePoison(stack_t *st) {
    assert(st);

    if (st->cur_err != EGENERALLY_STACK_OK || st->status_stck == STATUS_DESTROYED) {
        return NULL;
    }

    elem_t *mems_check = (elem_t *)memset(st->stck + st->pos_stck, POISON_BYTE, 
        (st->capacity - st->pos_stck) * sizeof(st->stck[0]));
    return mems_check;
}
int StackInit(stack_t *st, size_t capac 
    ON_DBG(, const char *name_f, const char *name_v, int ln, FILE *logy_f, frmttd_print_t LogPrinter)) {
    assert(st);
    ON_DBG(assert(name_f);
    assert(name_v);)

    ON_DBG(st->f_name = name_f;
    st->val_name = name_v;
    st->line = ln;
    st->file_log = logy_f;
    st->LogPrinterFunc = LogPrinter;)
    
    if (InitCheck(st) != STACK_IS_OK) {
        return st->cur_err;
    }
    st->pos_stck = 0;
    st->capacity = st->reserve_capacity =  capac;
    st->stck = (elem_t *)calloc(capac, sizeof(elem_t));
    if (st->stck == NULL) {
        st->cur_err = ECALLOC;
        return ECALLOC;
    }

    MakePoison(st);
    st->status_stck = STATUS_EMPTY;
    st->cur_err = STACK_IS_OK;
    return st->cur_err;
}

int InitCheck(stack_t *st) {
    assert(st);

    struct _heapinfo heap_inf = {};
    int heap_status = 0;
    while ((heap_status = _heapwalk(&heap_inf)) != _HEAPEND && heap_status != _HEAPEMPTY) {
        if ((elem_t *)heap_inf._pentry == st->stck) {
            st->cur_err = EALREADY_INIT;
            return EALREADY_INIT;
        }
    }
    return STACK_IS_OK;
}

int StackPush(stack_t *st, elem_t val) {
    assert(st);

    if (StackGeneralCheck(st) != EGENERALLY_STACK_OK) {
        ON_DBG(LogStackDump(st->file_log, st, st->LogPrinterFunc);)
        return st->cur_err;
    }

    if (st->pos_stck == MAX_SAFE_CAPACITY) {
        fprintf(stderr, "Warning: too large size\n");
    }

    if (st->pos_stck + 2 > st->capacity && st->capacity * 2 < MAX_SAFE_CAPACITY) {
        PushDoubler(st);
        elem_t *ptr_realloc = (elem_t *)realloc(st->stck, st->capacity * sizeof(elem_t));
        if (ptr_realloc == NULL) {
            st->cur_err = EREALLOC;
        }
        st->stck = ptr_realloc;
        MakePoison(st);
    }

    if (st->pos_stck == 0) {
        st->status_stck = STATUS_ACTIVE;
    }
    st->stck[st->pos_stck++] = val;
    
    return st->cur_err;
}

void PushDoubler(stack_t *st) {
    assert(st);

    st->capacity *= 2;
    st->reserve_capacity *= 2;
}

elem_t StackPop(stack_t *st) {
    assert(st);

    if (StackGeneralCheck(st) != EGENERALLY_STACK_OK) {
        ON_DBG(LogStackDump(st->file_log, st, st->LogPrinterFunc);)
        return (elem_t)0;
    }

    if (st->pos_stck < 1 || st->status_stck == STATUS_EMPTY) {
        st->cur_err = EEMPTY_POP;
        ON_DBG(LogStackDump(st->file_log, st, st->LogPrinterFunc);)
        return (elem_t)0;
    }
    size_t hyster_offset = (st->capacity % 2) ? st->capacity / 2 + 1 : st->capacity / 2;
    if (st->pos_stck < hyster_offset && st->capacity > 1) {
        PopDivide(st, &hyster_offset);
        elem_t *ptr_realloc = (elem_t *)realloc(st->stck, st->capacity * sizeof(elem_t));
        if (ptr_realloc == NULL) {
            st->cur_err = EREALLOC;
        }
        st->stck = ptr_realloc;   
    }

    memset(st->stck + st->pos_stck, POISON_BYTE, sizeof(st->stck[0]));
    if (st->pos_stck == 1) {
        memset(st->stck, POISON_BYTE, sizeof(st->stck[0]));
        st->status_stck = STATUS_EMPTY;
    }
    return st->stck[--st->pos_stck];
}

void PopDivide(stack_t *st, size_t *hyster_offset) {
    assert(st);
    assert(hyster_offset);

    st->capacity /= 2;
    st->reserve_capacity /= 2;
    *hyster_offset = (st->capacity % 2) ? st->capacity / 2 + 1 : st->capacity / 2;
}

FILE *LogOpen(const char *f_name, const char *f_mode) {
    assert(f_name);
    assert(f_mode);

    FILE *log_f = fopen(f_name, f_mode);
    if (log_f == NULL) {
        fprintf(stderr, "File %s in %s mode has not been opened or created\n" 
            "Got a mistake and FAILED\n", 
            f_name, f_mode);
        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return stderr;
    }
    return log_f;
}

int StackDestroy(stack_t *st 
    ON_DBG(, const char *name_f, int ln)) {
    assert(st);
    ON_DBG(assert(name_f);)
    
    StackGeneralCheck(st);

    ON_DBG(st->line = ln;
    st->f_name = name_f;
    st->LogPrinterFunc = NULL;)
    
    MakePoison(st);
    free(st->stck);

    st->stck = NULL;

    st->capacity = st->reserve_capacity = st->pos_stck =  0;
    
    st->status_stck = STATUS_DESTROYED;
    return st->cur_err;
}

void LogStackDump(FILE *log_file, stack_t *st, frmttd_print_t PrinterFunc) {
    assert(log_file);
    assert(st);

    StackGeneralCheck(st);

    const char *err_got = StackErrGet(st->cur_err);
    const char *status_got = StackStatusGet(st->status_stck);
    assert(err_got);
    assert(status_got);
    
    ON_DBG(fprintf(log_file, "Watching \"%s\" on line %d in %s:\n", st->val_name, st->line, st->f_name);
        fprintf(log_file, "Its status is %s and current error is %s\n", status_got, err_got);)

    fprintf(log_file, "Stack has capacity = %llu, position = %llu, and pointer = %p\n", 
        st->capacity, st->pos_stck, st->stck);
    for (size_t i = 0; i < st->capacity; i++) {
        fprintf(log_file, "        %s[%3llu] = ", 
            (i >= st->pos_stck) ? ((i == st->pos_stck) ? "->" : "  ") : "**", i);
        PrinterFunc(log_file, &st->stck[i]);
        if (i >= st->pos_stck) {
            fprintf(log_file, " (IMPLIED POISON)");
        }
        putc('\n', log_file);
    }
    PRINT_BOUNDARY(log_file);
}

const char *StackErrGet(int st_err) {
    switch (st_err) {
    case EGENERALLY_STACK_OK:
        return "STACK IS OK";
    case ESIZE_UPPER_CAPACITY:
        return "POSITION IS LARGER THAN CAPACITY";
    case ECAPACITY_IS_ZERO:
        return "CAPACITY IS ZERO";
    case EALREADY_INIT:
        return "THIS STACK HAS ALREADY BEEN INITIALIZED";
    case ECALLOC:
        return "CALLOC DURING INITIALIZATION FAILED";
    case EREALLOC:
        return "REALLOC DURING STACK RESIZING FAILED";
    case EEMPTY_POP:
        return "POP() FROM EMPTY STACK HAS BEEN TRIED";
    case EUNKNOWN:
        return "UNIDENTIFIED ERROR";
    case EBAD_STACK_PTR:
        return "BAD STACK POINTER";
    case ECAPACITY_EXTR_CHANGE:
        return "CAPACITY HAS BEEN UNPREDICTEDLY (EXTRINSICLY) CHANGED";
    default:
        return "UNKNOWN NUMBER OF ERROR";
    };
}

const char *StackStatusGet(int st_status) {
    switch (st_status) {
    case STATUS_ACTIVE:
        return "STACK IS ACTIVE AND HAS ELEMENT(-S) INSIDE";    
    case STATUS_DESTROYED:
        return "STACK HAS BEEN DESTROYED";
    case STATUS_EMPTY:
        return "STACK IS EMPTY";
    default:
        return "STACK HAS UNKNOWN STATUS";
    };
}

void StackStatsPrint(stack_t *st) {
    assert(st);

    StackGeneralCheck(st);

    fprintf(stderr, "status = %d, error = %d, pos = %llu, capacity = %llu, pointer = %p" 
        ON_DBG(", file = %s, name = %s, line = %d") "\n", 
        st->status_stck, st->cur_err, st->pos_stck, st->capacity, st->stck
        ON_DBG(, st->f_name, st->val_name, st->line));
}

#endif // STACK_H