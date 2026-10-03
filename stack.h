#ifndef STACK_H
#define STACK_H

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

// #define STACK_DEBUG
// #define STACK_HASHES_ON
// #define STACK_CANARIES_ON
// #define STACK_UNSAFE_TRY_KILL_ON

#undef ON_DBG
#ifdef STACK_DEBUG
    #undef NDEBUG
    #define ON_DBG(...) __VA_ARGS__
#else
    #define ON_DBG(...)
#endif

#include <assert.h>

#undef ON_HSHS
#ifdef STACK_HASHES_ON
    #define ON_HSHS(...) __VA_ARGS__
    #define HASH_START_DJB2 5381
#else
    #define ON_HSHS(...)
#endif

#undef ON_CNRS
#ifdef STACK_CANARIES_ON
    #ifndef CANARIES_AMOUNT
        #define CANARIES_AMOUNT 2
    #endif
    #define ON_CNRS(...) __VA_ARGS__
    #define STACK_STRUCT_LEFT_CANARY 0xBEBADEDA
    #define STACK_STRUCT_RIGHT_CANARY 0xDEDABEBA
    #define PRINT_UNSAFE_DUMP_TRY(log_file, ptr, ...) \
        fprintf(log_file, "\n|||||||||| UNSAFE DUMP TRY CAN KILL THE PROCESS ||||||||||\n" \
            "|||||||||| CURRENT DUMP TRY IS USED FOR:\n" \
            "%p - stack pointer\n%p - all data pointer\n" \
            "%p - left canary pointer\n%p - right canary pointer ||||||||||\n", \
            ptr __VA_ARGS__)
    #define PRINT_UNSAFE_DESTRUCTION_TRY(log_file, ptr, ...) \
        fprintf(log_file, "\n|||||||||| UNSAFE DESTRUCTION TRY CAN KILL THE PROCESS ||||||||||\n" \
            "|||||||||| CURRENT DESTRUCTION TRY IS USED FOR:\n" \
            "%p - stack pointer\n%p - all data pointer\n" \
            "%p - left canary pointer\n%p - right canary pointer ||||||||||\n", \
            ptr __VA_ARGS__)
#else
    #define ON_CNRS(...)
    #define PRINT_UNSAFE_DUMP_TRY(log_file, ptr) \
        fprintf(log_file, "\n|||||||||| UNSAFE DUMP TRY CAN KILL THE PROCESS ||||||||||\n" \
            "|||||||||| CURRENT DUMP TRY IS USED FOR:\n" \
            "%p - stack pointer ||||||||||\n", ptr)
    #define PRINT_UNSAFE_DESTRUCTION_TRY(log_file, ptr) \
        fprintf(log_file, "\n|||||||||| UNSAFE DESTRUCTION TRY CAN KILL THE PROCESS ||||||||||\n" \
            "|||||||||| CURRENT DESTRUCTION TRY IS USED FOR:\n" \
            "%p - stack pointer ||||||||||\n", ptr)
#endif // STACK_CANARIES_ON

#undef ON_NSF_TR
#ifdef STACK_UNSAFE_TRY_KILL_ON
    #define ON_NSF_TR(...) __VA_ARGS__
#else
    #define ON_NSF_TR(...)
#endif

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#elif defined(__unix__)
    #ifndef _POSIX_VERSION
        #include <unistd.h>
    #endif
#endif

#ifdef DEF_STACK_TYPE
    typedef DEF_STACK_TYPE elem_t;
#else
    #define DEFAULT_STACK_TYPE int
    typedef DEFAULT_STACK_TYPE elem_t;
#endif // DEF_STACK_TYPE

#define STACK_IS_OK 0
#define POISON_BYTE 0xAA

typedef enum {
    EGENERALLY_STACK_OK = STACK_IS_OK,
    ESIZE_UPPER_CAPACITY = 1 << 1,
    ECAPACITY_IS_ZERO = 1 << 2,
    EALREADY_INIT = 1 << 3,
    ECALLOC = 1 << 4,
    EREALLOC = 1 << 5,
    EEMPTY_POP = 1 << 6,
    EBAD_STACK_PTR = 1 << 7,
    ECAPACITY_EXTR_CHANGE = 1 << 8,
    ON_CNRS(ESIZE_NO_MATCH = 1 << 9,
    ELEFT_CANARY_DAMAGED = 1 << 10,
    ERIGHT_CANARY_DAMAGED = 1 << 11,
    ELEFT_STRUCT_DAMAGE = 1 << 12,
    ERIGHT_STRUCT_DAMAGE = 1 << 13,)
    ON_HSHS(EWRONG_HASH = 1 << 14,)
    EUNKNOWN = 1 << 15,
} err_types_t;

ON_CNRS(
typedef enum {
    STRUCT_CANARIES_OK = 0,
    DATA_CANARIES_OK = 1 << 1,
    STRUCT_CANARIES_DAMAGED = 1 << 2,
    DATA_CANARIES_DAMAGED = 1 << 3,
} canaries_e_types_t;
)

typedef enum {
    STATUS_ACTIVE,
    STATUS_DESTROYED,
    STATUS_EMPTY,
} status_t;

typedef void (*frmttd_print_t)(FILE *log_file, const void *element);

typedef struct {
    ON_CNRS(uint64_t left_struct_canary;)
    ON_DBG(const char *f_name;
    const char *val_name;
    int line;
    FILE *file_log;
    frmttd_print_t LogPrinterFunc;)
    elem_t *stck;
    ON_CNRS(elem_t *all_data_allocated_buf_with_canaries;
    elem_t *left_stck_canary;
    elem_t *right_stck_canary;
    size_t full_data_size;)
    size_t pos_stck;
    size_t capacity;
    size_t reserve_capacity;
    ON_HSHS(uint32_t hash_djb2;)
    int cur_err;
    int status_stck;
    ON_CNRS(uint64_t right_struct_canary;)
} stack_t;

#define PRINT_BOUNDARY(log_file) \
    fprintf(log_file, "--------------------------------------------------" \
        "--------------------------------------------------\n")

#define BYTE_SIZE 256
#define DEFAULT_RAM_SIZE 1'073'741'824
#define MAX_SAFE_CAPACITY ((DEFAULT_RAM_SIZE) / 2 - 1)

// TODO Look for |= for errors method
// TODO IMPLEMENT HASHES

size_t GetRAMFreeSize(void);
size_t GetRAMTotalSize(void);
int StackGeneralCheck(stack_t *st);
ON_CNRS(
    int CheckDataCanaries(stack_t *st);
    int CheckStructCanaries(stack_t *st);
    void MakeCanaries(stack_t *st);
)
ON_HSHS(
    uint32_t GetHashDJB2(stack_t *st, size_t byted_size);
    void CheckHashDJB2(stack_t *st, size_t byted_size, uint32_t ref_hash);
)
elem_t *MakePoison(stack_t *st);
int StackInit(stack_t *st, size_t capac 
    ON_DBG(, const char *name_f, const char *name_v, int ln, FILE *logy_f, frmttd_print_t LogPrinter));
ON_DBG(
    void DbgInitRoutine(stack_t *st, const char *name_f, const char *name_v, 
        int ln, FILE *logy_f, frmttd_print_t LogPrinter);
)
#if defined(_WIN32) || defined(_WIN64)
    int InitCheck(stack_t *st);
#endif // _WIN32 || _WIN64
int InitCalloc(stack_t *st);
int StackPush(stack_t *st, elem_t val);
void PushDoubler(stack_t *st);
elem_t StackPop(stack_t *st);
void PopDivide(stack_t *st, size_t *hyster_offset);
int PopRealloc(stack_t *st);
FILE *LogOpen(const char *f_name, const char *f_mode);
int StackDestroy(stack_t *st 
    ON_DBG(, const char *name_f, int ln));
void LogStackDump(FILE *log_file, stack_t *st, frmttd_print_t PrinterFunc);
void DumpGuardPrint(stack_t *st, FILE *log_file, const char *func_name);
void DumpGeneralInfo(stack_t *st, FILE *log_file);
void DumpStackContent(stack_t *st, FILE *log_file, frmttd_print_t PrinterFunc);
const char *StackErrGet(int st_err);
const char *StackStatusGet(int st_status);
void StackStatsPrint(stack_t *st);

size_t GetRAMFreeSize(void) {
    #if defined(_WIN32) || defined(_WIN64)
        MEMORYSTATUSEX mem_stats = {};
        mem_stats.dwLength = sizeof(mem_stats);
        if (GlobalMemoryStatusEx(&mem_stats)) {
            return mem_stats.ullAvailPhys;
        }
        fprintf(stderr, "FAILED TRY TO GET FREE MEMORY SIZE\n");
        return DEFAULT_RAM_SIZE;
    #elif defined(__unix__)
        int64_t page_size = sysconf(_SC_PAGE_SIZE);
        int64_t available_page_amount = sysconf(_SC_AVPHYS_PAGES);
        if (page_size > 0 && available_page_amount > 0) {
            return page_size * available_page_amount;
        }
        return DEFAULT_RAM_SIZE;
    #else
        return DEFAULT_RAM_SIZE;
    #endif
}

size_t GetRAMTotalSize(void) {
    #if defined(_WIN32) || defined(_WIN64)
        MEMORYSTATUSEX mem_stats = {};
        mem_stats.dwLength = sizeof(mem_stats);
        if (GlobalMemoryStatusEx(&mem_stats)) {
            return mem_stats.ullTotalPhys;
        }
        fprintf(stderr, "FAILED TRY TO GET FREE MEMORY SIZE\n");
        return DEFAULT_RAM_SIZE;
    #elif defined(__unix__)
        int64_t page_size = sysconf(_SC_PAGE_SIZE);
        int64_t page_amount = sysconf(_SC_PHYS_PAGES);
        if (page_size > 0 && available_page_amount > 0) {
            return (size_t)(page_size * page_amount);
        }
        fprintf(stderr, "FAILED TRY TO GET FREE MEMORY SIZE\n");
        return DEFAULT_RAM_SIZE;
    #else
        return DEFAULT_RAM_SIZE;
    #endif
}

int StackGeneralCheck(stack_t *st) {
    assert(st);

    if (st->status_stck != STATUS_DESTROYED) {
        ON_CNRS(
            if (CheckDataCanaries(st) == DATA_CANARIES_DAMAGED) {
                return st->cur_err;
            }
            if (CheckStructCanaries(st) == STRUCT_CANARIES_DAMAGED) {
                return st->cur_err;
            }
        )
        if (st->stck == NULL ON_CNRS(|| 
            st->stck - CANARIES_AMOUNT / 2 != st->all_data_allocated_buf_with_canaries)) {
            st->cur_err = EBAD_STACK_PTR;
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
        ON_CNRS(
            if (st->full_data_size - CANARIES_AMOUNT != st->capacity || 
                st->reserve_capacity != st->full_data_size - CANARIES_AMOUNT) {
                st->cur_err = ESIZE_NO_MATCH;
                return ESIZE_NO_MATCH;
            }
        )
    }
    return EGENERALLY_STACK_OK;
}

ON_CNRS(
    int CheckDataCanaries(stack_t *st) {
        assert(st);

        if (st->left_stck_canary != st->all_data_allocated_buf_with_canaries || 
            *st->left_stck_canary != *st->all_data_allocated_buf_with_canaries) {
            st->cur_err = ELEFT_CANARY_DAMAGED;
            return DATA_CANARIES_DAMAGED;
        }
        if (st->right_stck_canary != 
            st->all_data_allocated_buf_with_canaries + st->full_data_size - CANARIES_AMOUNT / 2 || 
            *st->right_stck_canary != 
            *(st->all_data_allocated_buf_with_canaries + st->full_data_size - CANARIES_AMOUNT / 2)) {
            st->cur_err = ERIGHT_CANARY_DAMAGED;
            return DATA_CANARIES_DAMAGED;
        }

        return DATA_CANARIES_OK;
    }

    int CheckStructCanaries(stack_t *st) {
        assert(st);

        if (st->left_struct_canary != STACK_STRUCT_LEFT_CANARY) {
            st->cur_err = ELEFT_STRUCT_DAMAGE;
            return STRUCT_CANARIES_DAMAGED;
        }
        if (st->right_struct_canary != STACK_STRUCT_RIGHT_CANARY) {
            st->cur_err = ERIGHT_STRUCT_DAMAGE;
            return STRUCT_CANARIES_DAMAGED;
        }

        return STRUCT_CANARIES_OK;
    }

    void MakeCanaries(stack_t *st) {
        assert(st);
        srand(time(NULL));

        memset(st->all_data_allocated_buf_with_canaries, 0, 1);
        memset(st->all_data_allocated_buf_with_canaries + st->full_data_size - 1, 0, 1);

        size_t el_size_without_byte = sizeof(elem_t) - 1;
        if (el_size_without_byte > 0) {
            memset((uint8_t *)st->all_data_allocated_buf_with_canaries + 1, 
                (unsigned)rand() % BYTE_SIZE, el_size_without_byte);
            memset((uint8_t *)st->all_data_allocated_buf_with_canaries + ((st->full_data_size - 1) * sizeof(elem_t)) + 1, 
                (unsigned)rand() % BYTE_SIZE, el_size_without_byte);
        }
        
        st->left_stck_canary = st->all_data_allocated_buf_with_canaries;
        st->right_stck_canary = st->all_data_allocated_buf_with_canaries + st->full_data_size - 1;
    }
)

ON_HSHS(
    uint32_t GetHashDJB2(stack_t *st, size_t byted_size) {
        assert(st);
        
        st->hash_djb2 = 0;
        const uint8_t *byted_data = (const uint8_t *)st;
        uint32_t hash = HASH_START_DJB2;
        for (size_t i = 0; i < byted_size; i++) {
            hash = ((hash << 5) + hash) + byted_data[i];
        }
        st->hash_djb2 = hash;
        return hash;
    }

    void CheckHashDJB2(stack_t *st, size_t byted_size, uint32_t ref_hash) {
        assert(st);

        uint32_t new_hash = GetHashDJB2(st, byted_size);
        if (ref_hash != new_hash) {
            st->cur_err = EWRONG_HASH;
        }
    }
)

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

    if (capac == 0) {
        st->cur_err = ECAPACITY_IS_ZERO;
        return ECAPACITY_IS_ZERO;
    }
    
    ON_DBG(DbgInitRoutine(st, name_f, name_v, ln, logy_f, LogPrinter);)

    #if defined(_WIN32) || defined(_WIN64)
        if (InitCheck(st) != STACK_IS_OK) {
            return st->cur_err;
        }
    #endif // _WIN32 || _WIN64

    st->pos_stck = 0;
    st->capacity = st->reserve_capacity =  capac;
    ON_CNRS(st->full_data_size = st->capacity + CANARIES_AMOUNT;)
    #ifdef STACK_CANARIES_ON
        elem_t *tmp_ptr = (elem_t *)calloc(st->full_data_size, sizeof(elem_t));
    #else
        elem_t *tmp_ptr = (elem_t *)calloc(st->capacity, sizeof(elem_t));
    #endif // STACK_CANARIES_ON
    if (tmp_ptr == NULL) {
        st->cur_err = ECALLOC;
        return ECALLOC;
    }

    if (InitCalloc(st) != EGENERALLY_STACK_OK) {
        ON_DBG(LogStackDump(st->file_log, st, st->LogPrinterFunc);)
        return ECALLOC;
    }

    ON_CNRS(st->all_data_allocated_buf_with_canaries = tmp_ptr;)
    st->stck = tmp_ptr ON_CNRS(+ (CANARIES_AMOUNT / 2));

    MakePoison(st);
    ON_CNRS(
        MakeCanaries(st);
        st->left_struct_canary = STACK_STRUCT_LEFT_CANARY, st->right_struct_canary = STACK_STRUCT_RIGHT_CANARY;
    )
    st->status_stck = STATUS_EMPTY;
    st->cur_err = STACK_IS_OK;
    return st->cur_err;
}

ON_DBG(
    void DbgInitRoutine(stack_t *st, const char *name_f, const char *name_v, 
    int ln, FILE *logy_f, frmttd_print_t LogPrinter) {
    assert(st);
    assert(name_f);
    assert(name_v);
    assert(logy_f);
    assert(LogPrinter);

    st->f_name = name_f;
    st->val_name = name_v;
    st->line = ln;
    st->file_log = logy_f;
    st->LogPrinterFunc = LogPrinter;
}
)

#if defined(_WIN32) || defined(_WIN64)
    int InitCheck(stack_t *st) {
        assert(st);

        struct _heapinfo heap_inf = {};
        int heap_status = 0;
        while ((heap_status = _heapwalk(&heap_inf)) != _HEAPEND && heap_status != _HEAPEMPTY) {
            if ((elem_t *)heap_inf._pentry == st->stck) {
                st->cur_err = EALREADY_INIT;
                ON_DBG(LogStackDump(st->file_log, st, st->LogPrinterFunc);)            
                return EALREADY_INIT;
            }
        }
        return STACK_IS_OK;
    }
#endif // _WIN32 || _WIN64

int InitCalloc(stack_t *st) {
    assert(st);

    #ifdef STACK_CANARIES_ON
        elem_t *tmp_ptr = (elem_t *)calloc(st->full_data_size, sizeof(elem_t));
    #else
        elem_t *tmp_ptr = (elem_t *)calloc(st->capacity, sizeof(elem_t));
    #endif // STACK_CANARIES_ON
    if (tmp_ptr == NULL) {
        st->cur_err = ECALLOC;
        return ECALLOC;
    }
    return st->cur_err;
}

int StackPush(stack_t *st, elem_t val) {
    assert(st);

    if (StackGeneralCheck(st) != EGENERALLY_STACK_OK) {
        ON_DBG(LogStackDump(st->file_log, st, st->LogPrinterFunc);)
        return st->cur_err;
    }

    if (st->pos_stck + 2 > st->capacity && st->capacity * 2 ON_CNRS(+ CANARIES_AMOUNT) < MAX_SAFE_CAPACITY) {
        PushDoubler(st); 
        #ifdef STACK_CANARIES_ON
            elem_t *ptr_realloc = (elem_t *)realloc(st->all_data_allocated_buf_with_canaries, 
                st->full_data_size * sizeof(elem_t));
        #else
            elem_t *ptr_realloc = (elem_t *)realloc(st->stck, 
                st->capacity * sizeof(elem_t));
        #endif // STACK_CANARIES_ON
        if (ptr_realloc == NULL) {
            st->cur_err = EREALLOC;
        }
        
        ON_CNRS(st->all_data_allocated_buf_with_canaries = ptr_realloc;)
        st->stck = ptr_realloc ON_CNRS(+ CANARIES_AMOUNT / 2);
        MakePoison(st);
        ON_CNRS(MakeCanaries(st);)
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
    ON_CNRS(st->full_data_size = st->capacity + CANARIES_AMOUNT;)
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
        if (PopRealloc(st) != EGENERALLY_STACK_OK) {
            ON_DBG(LogStackDump(st->file_log, st, st->LogPrinterFunc);)
            return (elem_t)0;
        }
    }

    elem_t reserve = st->stck[--st->pos_stck];
    memset(st->stck + st->pos_stck, POISON_BYTE, sizeof(st->stck[0]));
    if (st->pos_stck == 0) {
        st->status_stck = STATUS_EMPTY;
    }
    
    return reserve;
}

void PopDivide(stack_t *st, size_t *hyster_offset) {
    assert(st);
    assert(hyster_offset);

    st->capacity /= 2;
    ON_CNRS(st->full_data_size = st->capacity + CANARIES_AMOUNT;)
    st->reserve_capacity /= 2;
    *hyster_offset = (st->capacity % 2) ? st->capacity / 2 + 1 : st->capacity / 2;
}

int PopRealloc(stack_t *st) {
    assert(st);

    #ifdef STACK_CANARIES_ON
        elem_t *ptr_realloc = (elem_t *)realloc(st->all_data_allocated_buf_with_canaries, 
            st->full_data_size * sizeof(elem_t));
    #else
        elem_t *ptr_realloc = (elem_t *)realloc(st->stck, st->capacity * sizeof(elem_t));
    #endif // STACK_CANARIES_ON
    if (ptr_realloc == NULL) {
        st->cur_err = EREALLOC;
        return EREALLOC;
    }

    ON_CNRS(st->all_data_allocated_buf_with_canaries = ptr_realloc;)
    st->stck = ptr_realloc ON_CNRS(+ CANARIES_AMOUNT / 2);
    ON_CNRS(MakeCanaries(st);)
    return st->cur_err;
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

    if (st->cur_err == EBAD_STACK_PTR 
        ON_CNRS(|| st->cur_err == ELEFT_CANARY_DAMAGED || st->cur_err == ERIGHT_CANARY_DAMAGED)) {
        PRINT_UNSAFE_DESTRUCTION_TRY(stderr, st->stck 
            ON_CNRS(, st->all_data_allocated_buf_with_canaries, st->left_stck_canary, st->right_stck_canary));
        ON_NSF_TR(return st->cur_err;)
    }

    MakePoison(st);
    #ifdef STACK_CANARIES_ON
        free(st->all_data_allocated_buf_with_canaries);
    #else
        free(st->stck);
    #endif // STACK_CANARIES_ON
    st->stck ON_CNRS(= st->all_data_allocated_buf_with_canaries) = NULL;
    ON_CNRS(st->left_stck_canary = st->right_stck_canary = NULL;)
    ON_CNRS(st->full_data_size =) st->capacity = st->reserve_capacity = st->pos_stck = 0;
    
    st->status_stck = STATUS_DESTROYED;
    return st->cur_err;
}

void LogStackDump(FILE *log_file, stack_t *st, frmttd_print_t PrinterFunc) {
    assert(log_file);
    assert(st);

    StackGeneralCheck(st);
    if (st->cur_err == EBAD_STACK_PTR 
        ON_CNRS(|| st->cur_err == ELEFT_CANARY_DAMAGED || st->cur_err == ERIGHT_CANARY_DAMAGED)) {
        DumpGuardPrint(st, log_file, __func__);
        ON_NSF_TR(return ;)
    }
    
    ON_DBG(fprintf(log_file, "Watching \"%s\" on line %d in %s:\n", st->val_name, st->line, st->f_name);)
    DumpGeneralInfo(st, log_file);
    
    if (st->status_stck != STATUS_DESTROYED) {
        ON_CNRS(
            fprintf(log_file, "        CL[|||] = %p: ", st->left_stck_canary);
            PrinterFunc(log_file, st->left_stck_canary);
            fprintf(log_file, "\n");
        )
        DumpStackContent(st, log_file, PrinterFunc);
        ON_CNRS(
            fprintf(log_file, "        CR[|||] = %p: ", st->right_stck_canary);
            PrinterFunc(log_file, st->right_stck_canary);
            fprintf(log_file, "\n");
        )
    }

    #ifdef STACK_CANARIES_ON
        fprintf(log_file, "Ends on %p adress (after last allocated elem_t element of all stack data)\n", 
            st->all_data_allocated_buf_with_canaries + st->full_data_size);
    #else
        fprintf(log_file, "Ends on %p adress (after last allocated elem_t element of all stack data)\n", 
            st->stck + st->capacity);
    #endif
    PRINT_BOUNDARY(log_file);
}

void DumpGuardPrint(stack_t *st, FILE *log_file, const char *func_name) {
    assert(st);
    assert(log_file);
    assert(func_name);

    PRINT_UNSAFE_DUMP_TRY(stderr, st->stck 
            ON_CNRS(, st->all_data_allocated_buf_with_canaries, st->left_stck_canary, st->right_stck_canary));
    ON_NSF_TR(fprintf(stderr, "STACK POINTER GUARD IS ON AND KILLS THE %s at %s on %s\n", 
        __func__, __TIME__, __DATE__);)
    PRINT_UNSAFE_DUMP_TRY(log_file, st->stck 
        ON_CNRS(, st->all_data_allocated_buf_with_canaries, st->left_stck_canary, st->right_stck_canary));
    ON_NSF_TR(fprintf(log_file, "STACK POINTER GUARD IS ON AND KILLS THE %s at %s on %s\n", 
        __func__, __TIME__, __DATE__);)
}

void DumpGeneralInfo(stack_t *st, FILE *log_file) {
    assert(st);
    assert(log_file);

    const char *err_got = StackErrGet(st->cur_err);
    const char *status_got = StackStatusGet(st->status_stck);
    assert(err_got);
    assert(status_got);

    fprintf(log_file, "Stack's status is %s and current error is %s\n", status_got, err_got);
    fprintf(log_file, "Stack has capacity = %llu, position = %llu, and pointer = %p\n", 
        st->capacity, st->pos_stck, st->stck);
    ON_CNRS(fprintf(log_file, "Debug and check information: full size = %llu, full data pointer = %p\n", 
        st->full_data_size, st->all_data_allocated_buf_with_canaries);)
}

void DumpStackContent(stack_t *st, FILE *log_file, frmttd_print_t PrinterFunc) {
    assert(st);
    assert(log_file);
    assert(PrinterFunc);

    for (size_t i = 0; i < st->capacity; i++) {
        fprintf(log_file, "        %s[%3llu] = ", 
            (i >= st->pos_stck) ? ((i == st->pos_stck) ? "->" : "  ") : "**", i);
        PrinterFunc(log_file, &st->stck[i]);
        if (i >= st->pos_stck) {
            fprintf(log_file, " (IMPLIED POISON)");
        }
        fprintf(log_file, "\n");
    }
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
    ON_CNRS(
        case ESIZE_NO_MATCH:
            return "FULL SIZE DOES NOT MATCH THE INFORMATIVE ONE";
        case ELEFT_CANARY_DAMAGED:
            return "LEFT CANARY HAS BEEN DAMAGED";
        case ERIGHT_CANARY_DAMAGED:
            return "RIGHT CANARY HAS BEEN DAMAGED";
        case ELEFT_STRUCT_DAMAGE:
            return "THE STRUCTURE HAS BEEN DAMAGED FROM THE LEFT";
        case ERIGHT_STRUCT_DAMAGE:
            return "THE STRUCTURE HAS BEEN DAMAGED FROM THE RIGHT";
    )
    ON_HSHS(
        case EWRONG_HASH:
            return "STRUCT HASH HAS BEEN UNPREDICTEDLY (EXTRINSICLY) CHANGED";
    )
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