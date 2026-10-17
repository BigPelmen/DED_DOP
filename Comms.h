#ifndef COMMS_H
#define COMMS_H

// TODO PUSH DOES NOT WORK CORRECTLY WITH [ABX] COMBINATION - IT WILL JUST TAKE THE NUMBER OF THE REGISTR

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>

typedef enum {
    OUT_COM,
    HLT_COM,
    PUSH_COM,
    PUSHR_COM,
    PUSHM_COM,
    POPR_COM,
    POPM_COM,
    ADD_COM,
    SUB_COM,
    MULT_COM,
    DIV_COM,
    JMP_COM,
    JA_COM,
    JAE_COM,
    JB_COM,
    JBE_COM,
    JE_COM,
    JNE_COM,
    // CALL_COM,
    // RET_COM,
    WRONG_COMMAND_COM,
} command_t;

#define HLT_COM_OFFSET 255
#define ONE_ARG_OFFSET 2
#define NO_ARG_OFFSET 1
#define JMP_OFFSET 0
#define CODE_LINE_MULTIPLIER 5
#define REGS_DEPTH 4

#undef GET_NAME
#define GET_NAME(var) #var

int GetFDesc(const char *f_name, int flags, mode_t f_mode);
size_t GetFileSize(int fd);
int GetCodeOffset(int com);
void SpacesSkip(char **buf);
void GraphemeSkip(char **buf);

int GetFDesc(const char *f_name, int flags, mode_t f_mode) {
    assert(f_name);

    int fd = open(f_name, flags, f_mode);
    if (fd < 0) {
        fprintf(stderr, "File %s in %#x mode has not been opened\n", f_name, f_mode);
        fprintf(stderr, "%d: %s\n", errno, strerror(errno));
        close(fd);
        return fd;
    }
    return fd;
}

size_t GetFileSize(int fd) {
    struct stat file_stat = {};
    if (fstat(fd, &file_stat) < 0) {
        fprintf(stderr, "%d: %s\n", errno, strerror(errno));
        fprintf(stderr, "Checking size of file has failed\n");
        return 0;
    }
    return file_stat.st_size;
}

int GetCodeOffset(int com) {
    switch (com) {
    case OUT_COM:
        return NO_ARG_OFFSET;
    case HLT_COM:
        return HLT_COM_OFFSET;
    case PUSH_COM:
        return ONE_ARG_OFFSET;
    case PUSHR_COM:
        return ONE_ARG_OFFSET;
    case PUSHM_COM:
        return ONE_ARG_OFFSET;
    case POPR_COM:
        return ONE_ARG_OFFSET;
    case POPM_COM:
        return ONE_ARG_OFFSET;
    case ADD_COM:
        return NO_ARG_OFFSET;
    case SUB_COM:
        return NO_ARG_OFFSET;
    case MULT_COM:
        return NO_ARG_OFFSET;
    case DIV_COM:
        return NO_ARG_OFFSET;
    case JMP_COM:
        return JMP_OFFSET;
    case JA_COM:
        return JMP_OFFSET;
    case JAE_COM:
        return JMP_OFFSET;
    case JB_COM:
        return JMP_OFFSET;
    case JBE_COM:
        return JMP_OFFSET;
    case JE_COM:
        return JMP_OFFSET;
    case JNE_COM:
        return JMP_OFFSET;
    case WRONG_COMMAND_COM:
        return NO_ARG_OFFSET;
    }
    return NO_ARG_OFFSET;
}

void SpacesSkip(char **buf) {
    assert(buf);

    while (isspace(**buf) && **buf != '\0') {
        (*buf)++;
    }
}

void GraphemeSkip(char **buf) {
    assert(buf);

    while (!isspace(**buf) && **buf != '\0') {
        (*buf)++;
    }
}

#endif // COMMS_H