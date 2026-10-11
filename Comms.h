#ifndef COMMS_H
#define COMMS_H

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
    ADD_COM,
    SUB_COM,
    MULT_COM,
    DIV_COM,
    WRONG_COMMAND_COM,
} command_t;

#define HLT_COM_OFFSET 0
#define CODE_LINE_MULTIPLIER 5

#undef GET_NAME
#define GET_NAME(var) #var

int GetFDesc(const char *f_name, int flags, mode_t f_mode);
size_t GetFileSize(int fd);

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

#endif // COMMS_H