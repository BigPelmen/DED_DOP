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

#define STACK_IS_OK 0
#define MAX_REALLOC_CNTR 4
#define POISON_BYTE 0xAA

const size_t MAX_SAFE_CAPACITY = ULLONG_MAX / 2 - 1;

typedef enum {
    ESIZE_UPPER_CAPACITY = 1,
    ESIZE_IS_NEGATIVE,
    ECAPACITY_IS_NEGATIVE,
    EALREADY_INIT,
    ECALLOC,
    EREALLOC,
    EEMPTY_POP,
} err_types_t;

typedef enum {
    STATUS_ACTIVE,
    STATUS_DESTROYED,
    STATUS_EMPTY,
} status_t;

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

typedef struct {
    ON_DBG(const char *f_name;
    const char *val_name;
    int line;)
    elem_t *stck;
    size_t pos_stck;
    size_t capacity;
    int cur_err;
    int status_stck;
} stack_t;

int Check(err_types_t reason);
elem_t *MakePoison(stack_t *st);
int StackInit(stack_t *st, size_t capac 
    ON_DBG(, const char *name_f, const char *name_v, int ln));
int InitCheck(stack_t *st);
int StackPush(stack_t *st, elem_t val);
elem_t StackPop(stack_t *st);
int LogOpen(const char *f_name, int flags, mode_t mode);
int StackDestroy(stack_t *st 
    ON_DBG(, const char name_f, int ln));

int Check(err_types_t reason) {
    
    switch (reason) {
    case EEMPTY_POP:
        break;
    default:
        break;
    };
    return 0;
}

elem_t *MakePoison(stack_t *st) {
    assert(st);

    return (elem_t *)memset(st->stck + st->pos_stck + 1, POISON_BYTE, 
        (st->capacity - st->pos_stck) * sizeof(st->stck[0]));
}

int StackInit(stack_t *st, size_t capac 
    ON_DBG(, const char *name_f, const char *name_v, int ln)) {
    assert(st);

    if (InitCheck(st) != STACK_IS_OK) {
        return st->cur_err;
    }
    st->pos_stck = 0;
    st->capacity = capac;
    st->stck = (elem_t *)calloc(capac, sizeof(elem_t));
    if (st->stck == NULL) {
        st->cur_err = ECALLOC;
        return st->cur_err;
    }
    ON_DBG(st->f_name = name_f;
    st->val_name = name_v;
    st->line = ln;
    // MakePoison(st);
)
    MakePoison(st);
    st->status_stck = STATUS_EMPTY;
    st->cur_err = STACK_IS_OK;
    return st->cur_err;
}

int InitCheck(stack_t *st) {
    struct _heapinfo heap_inf = {};
    int heap_status = 0;
    while ((heap_status = _heapwalk(&heap_inf)) != _HEAPEND && heap_status != _HEAPEMPTY) {
        if (heap_inf._pentry == st->stck) {
            st->cur_err = EALREADY_INIT;
            return EALREADY_INIT;
        }
    }
    return STACK_IS_OK;
}

// int PushCheck(stack_t *st) {
//     return ;
// }

// int PopCheck(stack_t *st) {
//     return ;
// }

int StackPush(stack_t *st, elem_t val) {
    assert(st);

    if (st->pos_stck == MAX_SAFE_CAPACITY) {
        fprintf(stderr, "Warning: too large size\n");
    }
    if (st->pos_stck + 2 > st->capacity) {
        // printf("Increase. pos: %llu -> %llu, c: %llu -> %llu\n", st->pos_stck, st->pos_stck + 1, st->capacity, st->capacity * 2);
        int realloc_cntr = 0;
        while (st->pos_stck + 2 > st->capacity && 
            realloc_cntr++ < MAX_REALLOC_CNTR && 
            st->capacity * 2 < MAX_SAFE_CAPACITY) {
            st->capacity *= 2;
        }
        st->stck = (elem_t *)realloc(st->stck, st->capacity * sizeof(elem_t));
        if (st->stck == NULL) {
            st->cur_err = EREALLOC;
        }
        MakePoison(st);
    }
    if (st->pos_stck == 0) {
        st->status_stck = STATUS_ACTIVE;
    }
    st->stck[st->pos_stck++] = val;
    
    return st->cur_err;
}

elem_t StackPop(stack_t *st) {
    assert(st);

    if (st->pos_stck < 1 || st->status_stck == STATUS_EMPTY) {
        st->cur_err = EEMPTY_POP;
        return st->stck[0];
    }
    size_t hyster_offset = (st->capacity % 2) ? st->capacity / 2 + 1 : st->capacity / 2;
    if (st->pos_stck < hyster_offset && st->capacity > 1) {
        int realloc_cntr = 0;
        // printf("hyster = %3llu, pos = %3llu, c = %3llu| Before\n", hyster_offset, st->pos_stck, st->capacity);
        while (st->pos_stck < hyster_offset && 
            realloc_cntr++ < MAX_REALLOC_CNTR && 
            st->capacity > 1) {
            st->capacity /= 2;
            hyster_offset = (st->capacity % 2) ? st->capacity / 2 + 1 : st->capacity / 2;
        }
        // printf("hyster = %3llu, pos = %3llu, c = %3llu| After\n", hyster_offset, st->pos_stck, st->capacity);
        st->stck = (elem_t *)realloc(st->stck, st->capacity * sizeof(elem_t));
        if (st->stck == NULL) {
            st->cur_err = EREALLOC;
        }
    }
    // printf("Now poped %d, pos: %llu -> %llu\n", st->stck[st->pos_stck - 1], st->pos_stck, st->pos_stck - 1);
    memset(st->stck + st->pos_stck, POISON_BYTE, sizeof(st->stck[0]));
    // if (st->pos_stck == 10) {
    //     for (size_t i = 0; i < st->capacity; i++) {
    //         printf("%d ", st->stck[i]);
    //     }
    //     putchar('\n');
    // }
    if (st->pos_stck == 1) {
        memset(st->stck, POISON_BYTE, sizeof(st->stck[0]));
        st->status_stck = STATUS_EMPTY;
    }
    return st->stck[--st->pos_stck];
}

int LogOpen(const char *f_name, int flags, mode_t mode) {
    int log_fd = open(f_name, flags, mode);
    if (log_fd < 0) {
        fprintf(stderr, "File %s with %#x flags and %#x has not been opened or created\n" 
            "Got a mistake and FAILED\n", 
            f_name, flags, mode);
        fprintf(stderr, "ERROR %d: %s\n", errno, strerror(errno));
        return -errno;
    }
    return log_fd;
}

int StackDestroy(stack_t *st 
    ON_DBG(, const char name_f, int ln)) {
    assert(st);

    ON_DBG(st->line = ln;
    st->f_name = name_f);

    MakePoison(st);
    free(st->stck);
    st->stck = NULL;

    st->capacity = 0;
    st->pos_stck = 0;

    st->status_stck = STATUS_DESTROYED;
    return st->cur_err;
}

// int LogStackDump(int log_fd, err_types_t err) {
//     return ;
// }

#endif // STACK_H