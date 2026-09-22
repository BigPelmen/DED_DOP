#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// #define MAX_BUF 256
#define MAX_LINES 4048
// #define GET_EOF 0

#define MY_FREE(_array_) do { \
    free(_array_); \
    _array_ = NULL; \
} while (0)

#define PRINT_BOUND(_fily_) \
    fprintf(_fily_, "------------------------------------------------------------" \
        "----------------------------------------------------------------\n")

enum mode_print {
    TO_STDOUT,
    TO_DEBUG,
    TO_WRITE
};

struct text_info {
    size_t real_file_size;
    size_t informative_size;
    size_t lines_amount;
    char *buffer;
    char **lines;
};

// int ReadLines(FILE *fp, char *line_ptrs[]);
// int FGetLine(char *line, size_t max_len, FILE *fp);
// void ClearDynamicMemory(char *line_ptrs[], int NLines);
// char *MyStrDup(const char *buf, int len);

struct text_info GetFileInfoBuf(const char *name_f, int flags, mode_t f_mode);
int GetCheckedFile(const char *f_name, int flags, mode_t f_mode);
size_t GetFileSize(int fd);
size_t GetLinePtrs(char buf[], size_t buf_size, char *line_ptrs[]);
void MySort(void *arr, size_t arr_size, size_t el_size, 
            int (*Comparator)(const void *left_value, const void *right_value));
            int MyStrCmpAlphas(const void *left, const void *right);
int MyStrCmpAlphasBacked(const void *left, const void *right);
void Swap(void *a, void *b, size_t el_size);
void PrintText(char *line_ptrs[], int NLines, int mode, FILE *where_write);
void WriteFile(char *line_ptrs[], int NLines, FILE *w_file);

int main() {
    struct text_info text1 = GetFileInfoBuf("C:\\Users\\PelmenDi\\Desktop\\DED_DOP\\Onegin_to_read.txt", 
        O_RDONLY, S_IRUSR);
    if (text1.informative_size == 0) {
        return 1;
    }
    text1.lines = (char **)calloc(MAX_LINES, sizeof(char *));
    text1.lines_amount = GetLinePtrs(text1.buffer, text1.informative_size, text1.lines);

    char **changable_text = (char **)calloc(text1.lines_amount, sizeof(char *));
    memcpy(changable_text, text1.lines, sizeof(changable_text[0]) * text1.lines_amount);
    FILE *w_file = fopen("C:\\Users\\PelmenDi\\Desktop\\DED_DOP\\Onegin_to_write.txt", "w");
    if (w_file == NULL) {
        fprintf(stderr, "File (C:\\Users\\PelmenDi\\Desktop\\DED_DOP\\Onegin_to_write.txt) "
            "to write (\"w\") has not been opend\n");
        fclose(w_file);
        return 1;
    }

    MySort(changable_text, text1.lines_amount, sizeof(changable_text[0]), &MyStrCmpAlphas);
    PrintText(changable_text, text1.lines_amount, TO_WRITE, w_file);
    
    qsort(changable_text, text1.lines_amount, sizeof(changable_text[0]), &MyStrCmpAlphasBacked);
    PrintText(changable_text, text1.lines_amount, TO_WRITE, w_file);
    
    PrintText(text1.lines, text1.lines_amount, TO_WRITE, w_file);
    
    MY_FREE(text1.lines);
    MY_FREE(text1.lines);
    MY_FREE(changable_text);

    return 0;
}

struct text_info GetFileInfoBuf(const char *name_f, int flags, mode_t f_mode) {
    int r_file = GetCheckedFile(name_f, flags, f_mode);
    size_t got_file_size = GetFileSize(r_file) + 1;
    assert(got_file_size != 1);
    struct text_info examined_text = {};
    examined_text.real_file_size = got_file_size;
    examined_text.buffer = (char *)calloc(examined_text.real_file_size, sizeof(char));
    int transferred_syms = read(r_file, examined_text.buffer, 
        examined_text.real_file_size / sizeof(*examined_text.buffer)) + 1;
    if (transferred_syms <= 0) {
        fprintf(stderr, "Read() has got a mistake %d: %s\n", errno, strerror(errno));
        examined_text.informative_size = 0;
        examined_text.buffer[0] = '\0';
        close(r_file);
        return examined_text;
    }
    examined_text.informative_size = (size_t)transferred_syms;
    examined_text.buffer[examined_text.informative_size - 1] = '\0';
    return examined_text;
}

int GetCheckedFile(const char *f_name, int flags, mode_t f_mode) {
    int fd = open(f_name, flags, f_mode);
    if (fd < 0) {
        fprintf(stderr, "File %s in %#x mode has not been opened\n", f_name, f_mode);
        fprintf(stderr, "%d: %s\n", errno, strerror(errno));
        return fd;
    }
    return fd;
}

size_t GetFileSize(int fd) {
    struct stat file_stat = {};
    if (fstat(fd, &file_stat) < 0) {
        fprintf(stderr, "%d: %s\n", errno, strerror(errno));
        printf("Checking size of file has failed\n");
        return 0;
    }
    return file_stat.st_size;
}

size_t GetLinePtrs(char buf[], size_t buf_size, char *line_ptrs[]) {
    line_ptrs[0] = buf;
    char *last_found = buf;
    size_t i = 1;
    char *tmp = NULL;
    for ( ; i < buf_size; i++) {
        if ((tmp = strchr((const char *)(last_found), '\n')) == NULL) {
            line_ptrs[i] = last_found;
            buf[strchr((const char *)(last_found), '\0') - buf] = '\0';
            break;
        }
        buf[tmp - buf] = '\0';
        last_found = line_ptrs[i] = tmp + 1;
        // printf("%llu) <%s>", i, line_ptrs[i]);
    }
    return i;
}

void MySort(void *arr, size_t arr_size, size_t el_size, int (*Comparator)(const void *left_value, const void *right_value)) {
    // printf("%s\n", *(char **)(arr));
    for (size_t i = 1; i < arr_size; i++) {
        int64_t k = i;
        // printf("k = %lli\n", k);
        // printf("%s\n", *(char **)(arr));
        while (k > 0) {
            void *left_ptr = (void *)(((uint8_t *)arr) + el_size * (k - 1));
            void *right_ptr = (void *)(((uint8_t *)arr) + el_size * k);
            // void *left_ptr = (void *)((size_t)arr + el_size * (k - 1));
            // void *right_ptr = (void *)((size_t)arr + el_size * k);
            if ((*Comparator)(left_ptr, right_ptr) <= 0) {
                // printf("k broke = %lli\n", k);
                break;
            }
            else {
                // printf("got k = %lli\n", k);
                Swap(left_ptr, right_ptr, el_size);
                k--;
            }
        }
    }
}

void Swap(void *a, void *b, size_t el_size) {
    uint8_t tmp = '\0';
    uint8_t *a_ptr = (uint8_t *)(a);
    uint8_t *b_ptr = (uint8_t *)(b);
    for (size_t i = 0; i < el_size; i++) {
        tmp = a_ptr[i];
        a_ptr[i] = b_ptr[i];
        b_ptr[i] = tmp;
    }
}

int MyStrCmpAlphas(const void *left, const void *right) {
    const char *s1 = *(const char **)left;
    const char *s2 = *(const char **)right;
    size_t i = 0, j = 0;
    int sym1 = '\0', sym2 = '\0';
    while (true) {
        while (s1[i] != '\0' && !isalpha(s1[i])) {
            i++;
        }
        while (s2[j] != '\0' && !isalpha(s2[j])) {
            j++;
        }
        if (s1[i] == '\0' && s2[j] == '\0') {
            return 0;
        }
        if (s1[i] == '\0') {
            return -1;
        }
        if (s2[j] == '\0') {
            return 1;
        }
        sym1 = tolower(s1[i++]);
        sym2 = tolower(s2[j++]);
        if (sym1 != sym2) {
            return (sym1 < sym2) ? -1 : 1;
        }
    }
}

int MyStrCmpAlphasBacked(const void *left, const void *right) {
    const char *s1 = *(const char **)left;
    const char *s2 = *(const char **)right;
    size_t i = strlen(s1) - 1, j = strlen(s2) - 1;
    int sym1 = '\0', sym2 = '\0';
    while (true) {
        while (!isalpha(s1[i]) && i > 0) {
            i--;
        }
        while (!isalpha(s2[j]) && j > 0) {
            j--;
        }
        if (i <= 0 && j <= 0) {
            return 0;
        }
        if (i <= 0) {
            return -1;
        }
        if (j <= 0) {
            return 1;
        }
        sym1 = tolower(s1[i--]);
        sym2 = tolower(s2[j--]);
        if (sym1 != sym2) {
            return (sym1 < sym2) ? -1 : 1;
        }
    }
}

void PrintText(char *line_ptrs[], int NLines, int mode, FILE *where_write) {
    switch (mode) {
    case TO_STDOUT:
        for (int i = 0; i < NLines; i++) {
            printf("%s\n", line_ptrs[i]);
        }
        break;
    case TO_DEBUG:
        printf("Lines read: %d\n", NLines);
        for (int i = 0; i < NLines; i++) {
            printf("%p: %p -> <%s> -> %p\n", 
                line_ptrs + i, line_ptrs[i], line_ptrs[i], line_ptrs[i] + sizeof(line_ptrs[i][0]) * strlen(line_ptrs[i]));
        }
        break;
    case TO_WRITE:
        WriteFile(line_ptrs, NLines, where_write);
        break;
    }
}

void WriteFile(char *line_ptrs[], int NLines, FILE *w_file) {
    for (int i = 0; i < NLines; i++) {
        fprintf(w_file, "%s\n", line_ptrs[i]);
    }
    PRINT_BOUND(w_file);
}

// char *MyStrDup(const char *buf, int len) {
//     char *line_ptr = (char *)calloc(len + 1, sizeof(buf[0]));
//     strcpy(line_ptr, buf);
//     return line_ptr;
// }

// int MyStrCmpAlphas(const void *left, const void *right) {
//     const char *s1 = *(const char **)left;
//     const char *s2 = *(const char **)right;
//     size_t i = 0, j = 0;
//     bool s1_end_found = false, s2_end_found = false;
//     bool s1_alpha_found = false, s2_alpha_found = false;
//     // printf("i = %2llu, j = %2llu, s1[i] = %c | %2d, s2[j] = %c | %2d\n", i, j, s1[i], s1[i], s2[j], s2[j]);
//     while (s1[i] != '\0' && s2[j] != '\0') {
//         // printf("i = %2llu, j = %2llu, s1[i] = %c | %2d, s2[j] = %c | %2d\n", i, j, s1[i], s1[i], s2[j], s2[j]);
//         while (isalpha(s1[i]) == 0 && s1[i] != '\0') {
//             i++;
//         }
//         if (!s1_alpha_found) {
//             s1_alpha_found = true;
//         }
//         while (isalpha(s2[j]) == 0 && s2[j] != '\0') {
//             j++;
//         }
//         if (!s2_alpha_found) {
//             s2_alpha_found = true;
//         }
//         s1_end_found = (s1[i] == '\0'), s2_end_found = (s2[j] == '\0');
//         if ((s1_end_found && s2_end_found) || tolower(s1[i++]) != tolower(s2[j++])) {
//             break;
//         }
//     }
//     if (s1_alpha_found && s2_alpha_found) {
//         if (tolower(s1[--i]) < tolower(s2[--j])) {
//             return -1;
//         }
//         else if (tolower(s1[i]) == tolower(s2[j])) {
//             return 0;
//         }
//         else {
//             return 1;
//         }
//     }
//     // else if (!s1_alpha_found) {
//     //     return 1;
//     // }
//     else {
//         return 1;
//     }    
// }

// int MyStrCmpAlphasBacked(const void *left, const void *right) {
//     const char *s1 = *(const char **)left;
//     const char *s2 = *(const char **)right;
//     size_t i = strlen(s1) - 1, j = strlen(s2) - 1;
//     bool s1_end_found = false, s2_end_found = false;
//     bool s1_alpha_found = false, s2_alpha_found = false;
//     while (s1[i] != '\0' && s2[j] != '\0') {
//         // printf("i = %2llu, j = %2llu, s1[i] = %c | %2d, s2[j] = %c | %2d\n", i, j, s1[i], s1[i], s2[j], s2[j]);
//         while (isalpha(s1[i]) == 0 && s1[i] != '\0') {
//             i--;
//         }
//         if (!s1_alpha_found) {
//             s1_alpha_found = true;
//         }
//         while (isalpha(s2[j]) == 0 && s2[j] != '\0') {
//             j--;
//         }
//         if (!s2_alpha_found) {
//             s2_alpha_found = true;
//         }
//         s1_end_found = (s1[i] == '\0'), s2_end_found = (s2[j] == '\0');
//         if ((s1_end_found && s2_end_found) || tolower(s1[i--]) != tolower(s2[j--])) {
//             break;
//         }
//     }
//     if (s1_alpha_found && s2_alpha_found) {
//         if (tolower(s1[++i]) < tolower(s2[++j])) {
//             return -1;
//         }
//         else if (tolower(s1[i]) == tolower(s2[j])) {
//             return 0;
//         }
//         else {
//             return 1;
//         }
//     }
//     // else if (!s1_alpha_found) {
//     //     return 1;
//     // }
//     else {
//         return 1;
//     }
// }

// int FGetLine(char *line, size_t max_len, FILE *fp) {
//     size_t i = 0;
//     char buf_ch = '\0';
//     while ((buf_ch = fgetc(fp)) != '\n' && buf_ch != EOF && i < max_len - 1) {
//         line[i++] = buf_ch;
//     }
//     line[i++] = '\0';
//     if (buf_ch == '\n'){
//         return i;
//     }
//     else if (buf_ch == EOF) {
//         return GET_EOF;
//     }
//     else {
//         return -1;
//     }
// }


// int ReadLines(FILE *fp, char *line_ptrs[]) {
//     if (fp == NULL) {
//         // fprintf(stderr, "File %s has not been opened\n",);
//         fprintf(stderr, "%d: %s\n", errno, strerror(errno));
//         return -errno;
//     }
//     char buffer[MAX_BUF] = {};
//     int len = 0, NLines = 0;
//     while ((len = FGetLine(buffer, MAX_BUF, fp)) > 0) {
//         line_ptrs[NLines++] = MyStrDup(buffer, len);
//     }
//     fclose(fp);
//     return NLines;
// }

// void ClearDynamicMemory(char *line_ptrs[], int NLines) {
//     for (int i = 0; i < NLines; i++) {
//         free(line_ptrs[i]);
//     }
// }

    // ClearDynamicMemory(text1.buffer, text1.real_file_size);
    // ClearDynamicMemory(changable_text, text1.informative_size);
    // for (int i = 0; i < 10; i++) {
    //     for (int j = 0; *(changable_text[i] + j) != '\n'; j++) {
    //         putchar(changable_text[i][j]);
    //     }
    //     putchar('\n');
    //     // printf("%s", changable_text[i]);
    // }
    // int all_lines = ReadLines(text_ptrs, "C:\\Users\\PelmenDi\\Desktop\\DED_DOP\\Test_file_to_read.txt");
    // if (all_lines < 1) {
    //     return 1;
    // }
    // // printf("Cmp = %d\n", MyStrCmpAlphas((const void *)"          abc, de .f /", (const void *)"AbC __ - De   ."));
    // char *changable_text[MAX_LINES] = {};
    // memcpy(changable_text, text_ptrs, MAX_LINES * sizeof(text_ptrs[0]));
    // // printf("strcmp = %d\n", strcmp("aabc, de .", "AbC __ - De   ."));
    // // PrintText(text_ptrs, all_lines, TO_DEBUG);
    // // PrintText(changable_text, 20, TO_DEBUG);
    // MySort(changable_text, all_lines, sizeof(changable_text[0]), &MyStrCmpAlphas);
    // // PrintText(changable_text, all_lines, TO_STDOUT);
    // FILE *a_file = fopen("C:\\Users\\PelmenDi\\Desktop\\DED_DOP\\Onegin_to_write.txt", "w");
    // assert(a_file);
    // WriteFile(changable_text, all_lines, a_file);
    // fprintf(a_file, "\n---------------------------------------------------------------\n");
    // qsort(changable_text, all_lines, sizeof(changable_text[0]), &MyStrCmpAlphasBacked);
    // WriteFile(changable_text, all_lines, a_file);
    // fprintf(a_file, "\n---------------------------------------------------------------\n");
    // WriteFile(text_ptrs, all_lines, a_file);
    // ClearDynamicMemory(text_ptrs, all_lines);