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
	struct line_info *lines;
	int r_file;
};

struct line_info {
    char *line;
    size_t line_len;
};

typedef int (*comparator_t)(const void *, const void *);

size_t GetLinesAmount(struct text_info *text);
int GetFileInfoBuf(const char *name_f, int flags, mode_t f_mode, struct text_info *examined_text);
int GetCheckedFile(const char *f_name, int flags, mode_t f_mode);
size_t GetFileSize(int fd);
void GetLinePtrs(struct text_info *text);
void MySort(void *arr, size_t arr_size, size_t el_size, comparator_t Comparator);
int MyStrCmpAlphas(const void *left, const void *right);
int MyStrCmpAlphasBacked(const void *left, const void *right);
void Swap(void *a, void *b, size_t el_size);
void PrintText(char *line_ptrs[], int NLines, int mode, FILE *where_write);
void WriteFile(char *line_ptrs[], int NLines, FILE *w_file);

int main() {
    struct text_info text1 = {};
    if (GetFileInfoBuf("C:\\Users\\PelmenDi\\Desktop\\DED_DOP\\Onegin_to_read.txt", 
        O_RDONLY, S_IRUSR, &text1) != 0) {
			return 1;
	}
    
	text1.lines_amount = GetLinesAmount(&text1);
    text1.lines = (struct line_info *)calloc(text1.lines_amount, sizeof(struct line_info));
	assert(text1.lines);
    GetLinePtrs(&text1);

    struct line_info *changable_text = (struct line_info *)calloc(text1.lines_amount, sizeof(struct line_info));
    assert(changable_text);
    memcpy(changable_text, text1.lines, sizeof(changable_text[0]) * text1.lines_amount);

    MySort(changable_text, text1.lines_amount, sizeof(changable_text[0]), &MyStrCmpAlphasBacked);
    fwrite(changable_text[100].line, sizeof(char), changable_text[0].line_len, stdout);
    for (size_t i = 0; i < text1.lines_amount; i++) {
        fwrite(changable_text[i].line, sizeof(char), changable_text[i].line_len, stdout);
    }

    qsort(changable_text, text1.lines_amount, sizeof(changable_text[0]), &MyStrCmpAlphasBacked);
    for (size_t i = 0; i < text1.lines_amount; i++) {
        fwrite(changable_text[i].line, sizeof(char), changable_text[i].line_len, stdout);
    }

    MY_FREE(text1.lines);
    MY_FREE(text1.lines);
    MY_FREE(changable_text);

    return 0;
}

size_t GetLinesAmount(struct text_info *text) {
    assert(text);

	size_t n_lines = 0;
    for (size_t i = 0; i < text->informative_size; i++) {
		if (text->buffer[i] == '\n') {
			n_lines++;
		}
	}
    return ++n_lines;
}

int GetFileInfoBuf(const char *name_f, int flags, mode_t f_mode, struct text_info *examined_text) {
    examined_text->r_file = GetCheckedFile(name_f, flags, f_mode);
    size_t got_file_size = GetFileSize(examined_text->r_file) + 1;
    if (got_file_size == 1 || examined_text->r_file < 0) {
        fprintf(stderr, "Getting file information has gone wrong\n");
        return errno;
    }

    examined_text->real_file_size = got_file_size;
    examined_text->buffer = (char *)calloc(examined_text->real_file_size + 1, sizeof(char));
    assert(examined_text->buffer);

    int transferred_syms = read(examined_text->r_file, examined_text->buffer, 
        examined_text->real_file_size / sizeof(*examined_text->buffer));
    if (transferred_syms <= 0) {
        fprintf(stderr, "Read() has got a mistake %d: %s\n", errno, strerror(errno));
        examined_text->informative_size = 0;
        examined_text->buffer[0] = '\0';
        close(examined_text->r_file);
        return errno;
    }
    
    examined_text->informative_size = (size_t)transferred_syms + 1;
    examined_text->buffer[examined_text->informative_size - 1] = '\0';
    
    close(examined_text->r_file);
    return 0;
}

// void Swappy(void *left_ptr, void *right_ptr)

int GetCheckedFile(const char *f_name, int flags, mode_t f_mode) {
	assert(f_name);

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

void GetLinePtrs(struct text_info *text) {
	assert(text);

    char *last_found = text->buffer;
    char *tmp = NULL;
    size_t i = 0;
    for ( ; i < text->lines_amount; i++) {
        if ((tmp = strchr((const char *)(last_found), '\n')) == NULL) {
            // printf("Inside f: %p\n", text->lines[0].line);
            text->lines[i].line = last_found;
			size_t offset = strchr((const char *)(last_found), '\0') - last_found + 1;
			text->lines[i].line_len = offset;
            break;
        }
        text->lines[i].line_len = ++tmp - last_found;
        text->lines[i].line = last_found;
        last_found = tmp;
    }
}

void MySort(void *arr, size_t arr_size, size_t el_size, comparator_t Comparator) {
    assert(arr);

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
    assert(a);
    assert(b);

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
    assert(left);
    assert(right);

    struct line_info *stru1 = (struct line_info *)left;
    struct line_info *stru2 = (struct line_info *)right;
	const char *s1 = stru1->line;
	const char *s2 = stru2->line;
    size_t i = 0, j = 0;
    size_t len1 = stru1->line_len, len2 = stru2->line_len;
	char sym1 = '\0', sym2 = '\0';
    while (true) {
        while (i < len1 && !isalpha(s1[i])) {
            i++;
        }
        while (j < len2 && !isalpha(s2[j])) {
            j++;
        }
        if (i == len1 && j == len2) {
            return 0;
        }
        if (i == len1) {
            return -1;
        }
        if (j == len2) {
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
    assert(left);
    assert(right);

    struct line_info *stru1 = (struct line_info *)left;
    struct line_info *stru2 = (struct line_info *)right;
	const char *s1 = stru1->line;
	const char *s2 = stru2->line;
    size_t i = stru1->line_len - 1, j = stru2->line_len - 1;
	char sym1 = '\0', sym2 = '\0';
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
    assert(line_ptrs);
    assert(where_write);
    
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
    assert(w_file);

    for (int i = 0; i < NLines; i++) {
        fprintf(w_file, "%s\n", line_ptrs[i]);
    }
    PRINT_BOUND(w_file);
}
