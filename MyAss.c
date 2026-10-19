#include "Comms.h"

// TODO PUSH DOES NOT WORK CORRECTLY WITH [ABX] COMBINATION - IT WILL JUST TAKE THE NUMBER OF THE REGISTR

#define SIZEOF_COMMAND 50
#define LABEL_MAX 50

#define PRINT_SYNTAX_ERROR(com, line_ind) \
    fprintf(stderr, "Syntax Error: wrong %s command syntax on the line %llu\n", com, (line_ind) + 1)

#define PRINT_TOO_MANY_ARGS(com, line_ind) \
    fprintf(stderr, "Syntax Error: too many arguments after %s on the line %llu\n", com, line_ind + 1)

#define PRINT_BAD_ARGS(com, line_ind) \
    fprintf(stderr, "Syntax Error: bad argument for %s on the line %llu\n", com, line_ind + 1)

typedef enum {
    GOT_CODE_SUCCESSFULLY,
    READ_WRONG_COM,
    BAD_BUF_CALLOC,
    BAD_BUF_REALLOC,
    READ_F_REACHED_END,
    READ_WRONG_ARG,
    READ_WRONG_LABEL,
} buf_read_err_t;

typedef enum {
    CODE_IS_OK = 0,
    BAD_FILE_DESCRIPTOR = 1,
    BAD_R_FILE_SIZE = 1 << 1,
    BAD_CALLOC_PTR = 1 << 2,
    BAD_REALLOC_PTR = 1 << 3,
    BAD_READ_TRY = 1 << 4,
    BAD_BUF_PARSE = 1 << 5,
    BAD_LABEL_PLACEMENT = 1 << 6,
} code_err_t;

enum m_argc {
    NO_WHERE = 1,
    WHERE_READ,
    WHERE_READ_WRITE,
};

typedef enum {
    GOT_GOOD_LABEL,
    GOT_BAD_LABEL,
    GOT_NO_LABEL,
} label_info_t;

typedef struct {
    int fd_src;
    size_t src_size;
    size_t lines_amount;
    FILE *fp_dest;
    char *default_code;
    int *byted_code;
    size_t b_c_size;
    size_t d_c_size;
    int *labels;
    int cur_err;
} code_t;

// SECTION - Functions prototypes

int GetFDesc(const char *f_name, int flags, mode_t f_mode);
size_t GetFileSize(int fd);
int RunAss(code_t *my_code, int fd, FILE *w_fp);
int GetCode(code_t *my_code, int fd);
bool CheckFileErr(code_t *my_code, int fd);
bool CheckResDCodeBuf(code_t *my_code, char *res_buf);
bool GetLabelsPtr(code_t *my_code);
size_t PlaceLineEnds(char buf[], size_t transferred);
buf_read_err_t ReadBuf(code_t *co, size_t d_c_lines_amount);
bool CheckComHasArgs(int read_com);
int SecondRun(code_t *co, size_t lines_amount);
int ScanLabel(char *buf_ptr, int *lab, size_t ind_line);
bool ScanCom(char *buf_ptr, char *com, size_t ind_line);
bool LineIsEmptyComs(char *buf_ptr);
bool CheckComLine(command_t com, char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPushCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPushRegCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPopRegCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPopMemCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPushMemCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool ChecKJmpCom(char *buf_ptr, size_t line_ind, const char *jmp_name, int *check_arg);
command_t ReadCommand(const char *com, char *buf, size_t line_ind);
FILE *GetFPtr(const char *f_name, const char *f_mode);
int BytedFWrite(FILE *fp, code_t *co);
bool IsNotJmpCom(int read_com_num);
void AnalyzeGotCode(int err);

// !SECTION
// SECTION - main

int main(int argc, char const *argv[]) {
    code_t my_code = {};
    int fd = 0;
    FILE *w_fp = NULL;
    switch (argc) {
    case NO_WHERE:
        if ((fd = GetFDesc("CodeFile.txt", O_RDONLY, S_IRUSR)) < 0) {
            return 1;
        }
        if ((w_fp = GetFPtr("ByteCode.txt", "w")) == NULL) {
            return 1;
        }
        break;
    case WHERE_READ:
        if ((fd = GetFDesc(argv[1], O_RDONLY, S_IRUSR)) < 0) {
            return 1;
        }
        if ((w_fp = GetFPtr("ByteCode.txt", "w")) == NULL) {
            return 1;
        }
        break;
    case WHERE_READ_WRITE:
        if ((fd = GetFDesc(argv[1], O_RDONLY, S_IRUSR)) < 0) {
            return 1;
        }
        if ((w_fp = GetFPtr(argv[2], "w")) == NULL) {
            return 1;
        }
        break;
    default:
        fprintf(stderr, "WRONG PARAMETRS OF MAIN CALL\n");
        return 1;
    };
    return RunAss(&my_code, fd, w_fp);
    // int errs_got = GetCode(&my_code, fd);
    // if ((errs_got & ~CODE_IS_OK) != 0) {
    //     AnalyzeGotCode(errs_got);
    //     return 1;
    // }
    // my_code.fp_dest = w_fp;
    // printf("Finally: %d\n",(int)GetCode(&my_code, fd));
    // printf("err = %d, %llu, %p, %llu, %p, \nfd = %d, fp = %p, %llu\n", 
    //     my_code.cur_err, my_code.b_c_size, my_code.byted_code, my_code.d_c_size, my_code.default_code, 
    //     my_code.fd_src, my_code.fp_dest, my_code.src_size);
    // for (size_t i = 0; i < my_code.b_c_size; i++) {
    //     printf("%d ", my_code.byted_code[i]);
    // }
    // printf("\n");
    // for (size_t i = 0; i < LABEL_MAX; i++) {
    //     printf("%d ", my_code.labels[i]);
    // }
    // printf("\n");
    // BytedFWrite(w_fp, &my_code);
}

// !SECTION
// SECTION - Functions

FILE *GetFPtr(const char *f_name, const char *f_mode) {
    assert(f_name);
    assert(f_mode);

    FILE *fp = fopen(f_name, f_mode);
    if (fp == NULL) {
        fprintf(stderr, "File %s in %s mode has not been opened\n", f_name, f_mode);
        fprintf(stderr, "%d: %s\n", errno, strerror(errno));
        fclose(fp);
        return NULL;
    }
    return fp;
}

int RunAss(code_t *my_code, int fd, FILE *w_fp) {
    assert(my_code);
    assert(w_fp);

    int errs_got = GetCode(my_code, fd);
    if ((errs_got & ~CODE_IS_OK) != 0) {
        close(fd), fclose(w_fp);
        AnalyzeGotCode(errs_got);
        return 1;
    }
    BytedFWrite(w_fp, my_code);
    close(fd), fclose(w_fp);
    return 0;
}

int GetCode(code_t *my_code, int fd) {
    assert(my_code);

    if (!CheckFileErr(my_code, fd)) {
        return my_code->cur_err;
    }
    size_t buf_size = my_code->src_size + 1;

    char *buf = (char *)calloc(buf_size, sizeof(char));
    if (buf == NULL) {
        fprintf(stderr, "BAD CALLOC POINTER\n");
        my_code->cur_err |= BAD_CALLOC_PTR;
        return my_code->cur_err;
    }

    my_code->d_c_size = read(fd, buf, buf_size) + 1;
    if (my_code->d_c_size <= 0) {
        fprintf(stderr, "BAD READING TRY\n");
        my_code->cur_err |= BAD_READ_TRY;
        return my_code->cur_err;
    }

    char *res_buf = (char *)realloc(buf, my_code->d_c_size * sizeof(*buf));
    if (!CheckResDCodeBuf(my_code, res_buf)) {
        return my_code->cur_err;
    }
    my_code->default_code = res_buf;
    my_code->lines_amount = PlaceLineEnds(my_code->default_code, my_code->d_c_size);

    if (!GetLabelsPtr(my_code)) {
        return my_code->cur_err;
    }

    buf_read_err_t buf_r_err = ReadBuf(my_code, my_code->lines_amount); // TODO: ANALIZATOR OF READING PROCESS
    if (buf_r_err != GOT_CODE_SUCCESSFULLY) {
        fprintf(stderr, "GOT BAD READING BUF ATTEMPT\n");
        my_code->cur_err |= BAD_BUF_PARSE;
        return my_code->cur_err;
    }
    if (SecondRun(my_code, my_code->lines_amount) != CODE_IS_OK) {
        fprintf(stderr, "BAD SECOND RUN FOR LABELS\n");
    }
    return my_code->cur_err;
}

void AnalyzeGotCode(int err) {
    if ((err & BAD_FILE_DESCRIPTOR) != 0) {
        fprintf(stderr, "DID NOT MANAGE TO GET READ FILE DESCRIPTOR\n");
    }
    if ((err & BAD_R_FILE_SIZE) != 0) {
        fprintf(stderr, "FAILED GETTING FILE SIZE\n");
    }
    if ((err & BAD_CALLOC_PTR) != 0) {
        fprintf(stderr, "FAILED CALLOC ATTEMPT HAS BEEN DONE\n");
    }
    if ((err & BAD_REALLOC_PTR) != 0) {
        fprintf(stderr, "FAILED REALLOC ATTEMPT HAS BEEN DONE\n");
    }
    if ((err & BAD_READ_TRY) != 0) {
        fprintf(stderr, "READ FUNCTION FAILED WHILE READING FILE DESCRIPTOR\n");
    }
    if ((err & BAD_BUF_PARSE) != 0) {
        fprintf(stderr, "SYNTAX ERROR OR BAD RESULT WHILE PARSING BUFFER\n");
    }
    if ((err & BAD_LABEL_PLACEMENT) != 0) {
        fprintf(stderr, "SYNTAX ERROR OR SEMANCY OF LABELS\n");
    }
    return ;
}

int BytedFWrite(FILE *fp, code_t *co) {
    assert(fp);
    assert(co);

    for (size_t ind = 0; ind < co->b_c_size; ) {
        int offset = GetCodeOffset(co->byted_code[ind]);
        switch (offset) {
        case ONE_ARG_OFFSET:
            fprintf(fp, "%d %d\n", co->byted_code[ind], co->byted_code[ind + 1]);
            break;
        case NO_ARG_OFFSET:
            fprintf(fp, "%d\n", co->byted_code[ind]);
            break;
        default:
            fprintf(stderr, "UNKNOWN OFFSET GOT\n");
            return 0;
        };

        ind += offset;
    }
    return 1;
}

bool IsNotJmpCom(int read_com_num) {
    return (bool)(read_com_num != JMP_COM && 
        read_com_num != JA_COM && read_com_num != JAE_COM && 
        read_com_num != JB_COM && read_com_num != JBE_COM && 
        read_com_num != JE_COM && read_com_num != JNE_COM);
}

bool CheckFileErr(code_t *my_code, int fd) {
    assert(my_code);

    bool file_is_ok = true;
    if (fd < 0) {
        my_code->cur_err |= BAD_FILE_DESCRIPTOR;
        file_is_ok = false;
    }
    my_code->fd_src = fd;

    my_code->src_size = GetFileSize(fd);
    if (my_code->src_size == 0) {
        fprintf(stderr, "BAD FILE DESCRIPTOR\n");
        my_code->cur_err |= BAD_R_FILE_SIZE;
        file_is_ok = false;
    }
    
    return file_is_ok;
}

bool CheckResDCodeBuf(code_t *my_code, char *res_buf) {
    assert(my_code);
    assert(res_buf);

    if (res_buf == NULL) {
        fprintf(stderr, "BAD REALLOC TRY\n");
        my_code->cur_err |= BAD_REALLOC_PTR;
        return false;
    }
    return true;
}

bool GetLabelsPtr(code_t *my_code) {
    assert(my_code);

    int *labels_ptr = (int *)calloc(LABEL_MAX, sizeof(int));
    if (labels_ptr == NULL) {
        fprintf(stderr, "BAD CALLOC POINTER\n");
        my_code->cur_err |= BAD_CALLOC_PTR;
        return false;
    }
    my_code->labels = labels_ptr;
    return true;
}

size_t PlaceLineEnds(char buf[], size_t transferred) {
    assert(buf);

    char *last_found = buf;
    size_t i = 1;
    char *tmp = NULL;
    buf[transferred - 1] = '\0';
    for ( ; i < transferred; i++) {
        if ((tmp = strchr(last_found, '\n')) == NULL) {
            buf[strchr(last_found, '\0') - buf] = '\0'; 
            break;
        }
        buf[tmp - buf] = '\0';
        last_found = tmp + 1;
    }
    return i;
}

buf_read_err_t ReadBuf(code_t *co, size_t d_c_lines_amount) {
    assert(co);

    char *buf_ptr = co->default_code;
    size_t reserve_b_c_size = CODE_LINE_MULTIPLIER * d_c_lines_amount;
    char com[SIZEOF_COMMAND] = {};
    int *reserve_by_code = (int *)calloc(reserve_b_c_size, sizeof(int));
    if (reserve_by_code == NULL) {
        fprintf(stderr, "BAD CALLOC ATTEMPT FOR RESERVE BYTED CODE\n");
        return BAD_BUF_CALLOC;
    }
    size_t by_code_len = 0;
    bool hlt_found = false;

    for (size_t ind_line = 0; ind_line < d_c_lines_amount; ind_line++) {
        SpacesSkip(&buf_ptr);
        if (*buf_ptr == ';') {
            buf_ptr += strlen(buf_ptr) + 1;
            continue;
        }
        if (*buf_ptr == '\0') {
            buf_ptr++;
            continue;
        }

        int lab = 0, lab_scan_info = ScanLabel(buf_ptr, &lab, ind_line);
        switch (lab_scan_info) {
        case GOT_GOOD_LABEL:
            co->labels[lab] = by_code_len;
            buf_ptr += strlen(buf_ptr) + 1;
            continue;
        case GOT_BAD_LABEL:
            return READ_WRONG_LABEL;
        case GOT_NO_LABEL:
            break;
        default:
            continue;
        };

        if (!ScanCom(buf_ptr, com, ind_line)) {
            return READ_WRONG_COM;
        }
        command_t read_com = ReadCommand(com, buf_ptr, ind_line);
        if (read_com == WRONG_COMMAND_COM) {
            fprintf(stderr, "As bad command found, compilation stops on the line %llu\n", ind_line + 1);
            return READ_WRONG_COM;
        }
        reserve_by_code[by_code_len++] = (int)read_com;

        int arg = 0;
        if (!CheckComLine(read_com, buf_ptr, ind_line, &arg)) {
            return READ_WRONG_COM;
        }
        if (CheckComHasArgs(read_com)) {
            reserve_by_code[by_code_len++] = arg;
        }
        if (read_com == HLT_COM) {
            hlt_found = true;
        }
        buf_ptr += strlen(buf_ptr) + 1;
    }
    if (hlt_found) {
        int *res_byted = (int *)realloc(reserve_by_code, by_code_len * sizeof(*reserve_by_code));
        if (res_byted == NULL) {
            return BAD_BUF_REALLOC;
        }
        co->b_c_size = by_code_len, co->byted_code = res_byted;
        // for (size_t i = 0; i < co->b_c_size; i++) {
        //     printf("|%2d|\n", co->byted_code[i]);
        // }
        return GOT_CODE_SUCCESSFULLY;
    }
    fprintf(stderr, "IMPLIED: HLT HAS NOT BEEN FOUND\n");
    return READ_F_REACHED_END;
}

bool CheckComHasArgs(int read_com) {
    return (bool)(read_com == PUSH_COM || read_com == PUSHR_COM || read_com == PUSHM_COM || 
        read_com == POPR_COM || read_com == POPM_COM || 
        read_com == CALL_COM || 
        read_com == JMP_COM || 
        read_com == JA_COM || read_com == JAE_COM || 
        read_com == JB_COM || read_com == JBE_COM || 
        read_com == JE_COM || read_com == JNE_COM);
}

int SecondRun(code_t *co, size_t lines_amount) {
    assert(co);

    int pc = 0;
    char *buf_ptr = co->default_code;
    for (size_t line_ind = 0; line_ind < lines_amount; line_ind++) {
        SpacesSkip(&buf_ptr);
        if (*buf_ptr == ';') {
            buf_ptr += strlen(buf_ptr) + 1;
            continue;
        }
        if (*buf_ptr == '\0') {
            buf_ptr++;
            continue;
        }
        int lab = 0, lab_scan_info = ScanLabel(buf_ptr, &lab, line_ind);
        if ((size_t)lab >= co->b_c_size) {
            fprintf(stderr, "BAD LABEL GOT (TOO FAR JUMP)\n");
            co->cur_err |= BAD_LABEL_PLACEMENT;
            return co->cur_err;
        }
        if (lab_scan_info == GOT_GOOD_LABEL) {
            buf_ptr += strlen(buf_ptr) + 1;
            continue;
        }

        int arg = 0;
        // printf("buf: <%s>\n", buf_ptr);
        if (!CheckComLine((command_t)co->byted_code[pc], buf_ptr, line_ind, &arg)) {
            printf(buf_ptr);
            printf("Failed in: <%s>\n", buf_ptr);
            fprintf(stderr, "UNMATCHED COMMAND HAS BEEN FOUND\n");
            co->cur_err |= BAD_LABEL_PLACEMENT;
            return co->cur_err;
        }
        
        if (!IsNotJmpCom(co->byted_code[pc]) || co->byted_code[pc] == CALL_COM) {
            co->byted_code[++pc] = co->labels[arg];
            pc++;
            buf_ptr += strlen(buf_ptr) + 1;
            continue;
        }
        pc += GetCodeOffset(co->byted_code[pc]);;
        buf_ptr += strlen(buf_ptr) + 1;
    }
    // co->cur_err |= BAD_LABEL_PLACEMENT;
    return co->cur_err;
}

int ScanLabel(char *buf_ptr, int *lab, size_t ind_line) {
    assert(buf_ptr);
    assert(lab);

    if (*buf_ptr++ == ':') {
        SpacesSkip(&buf_ptr);
        if (sscanf(buf_ptr, "%d", lab) == 0) {
            fprintf(stderr, "SYNTAX ERROR: BAD LABEL on the line %llu\n", ind_line + 1);
            return GOT_BAD_LABEL;    
        }
        if (0 > *lab || *lab >= LABEL_MAX) {
            fprintf(stderr, "ASSEMBLE FAILED: TOO BIG LABEL on the line %llu\n", ind_line + 1);
            return GOT_BAD_LABEL;
        }
        GraphemeSkip(&buf_ptr);
        if (LineIsEmptyComs(buf_ptr)) {
            return GOT_GOOD_LABEL;
        }
        fprintf(stderr, "SYNTAX ERROR: BAD LABEL on the line %llu\n", ind_line + 1);
        return GOT_BAD_LABEL;
    }
    return GOT_NO_LABEL;
}

bool ScanCom(char *buf_ptr, char *com, size_t ind_line) {
    assert(buf_ptr);
    assert(com);

    if (sscanf(buf_ptr, "%s", com) == 0) {
        fprintf(stderr, "SYNTAX ERROR: ASSEMBLE FAILED on the line %llu\n", ind_line + 1);
        return false;
    }
    return true;
}

bool LineIsEmptyComs(char *buf_ptr) {
    assert(buf_ptr);

    SpacesSkip(&buf_ptr);
    if (*buf_ptr == '\0' || *buf_ptr == ';') {
        buf_ptr++;
        return true;
    }
    return false;
}

bool CheckComLine(command_t com, char *buf_ptr, size_t line_ind, int *check_arg) {
    assert(buf_ptr);
    assert(check_arg);

    switch (com) {
    case OUT_COM:
        buf_ptr += sizeof("OUT") - 1;
        if (LineIsEmptyComs(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("OUT", line_ind);
        return false;
    case HLT_COM:
        buf_ptr += sizeof("HLT") - 1;
        if (LineIsEmptyComs(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("HLT", line_ind);
        return false;
    case PUSH_COM:
        return CheckPushCom(buf_ptr, line_ind, check_arg);
    case PUSHR_COM:
        return CheckPushRegCom(buf_ptr, line_ind, check_arg);
    case PUSHM_COM:
        return CheckPushMemCom(buf_ptr, line_ind, check_arg);
    case POPR_COM:
        return CheckPopRegCom(buf_ptr, line_ind, check_arg);
    case POPM_COM:
        return CheckPopMemCom(buf_ptr, line_ind, check_arg);
    case ADD_COM:
        buf_ptr += sizeof("ADD") - 1;
        if (LineIsEmptyComs(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("ADD", line_ind);
        return false;
    case SUB_COM:
        buf_ptr += sizeof("SUB") - 1;
        if (LineIsEmptyComs(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("SUB", line_ind);
        return false;
    case MULT_COM:
        buf_ptr += sizeof("MULT") - 1;
        if (LineIsEmptyComs(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("MULT", line_ind);
        return false;
    case DIV_COM:
        buf_ptr += sizeof("DIV") - 1;
        if (LineIsEmptyComs(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("DIV", line_ind);
        return false;
    case JMP_COM:
        buf_ptr += sizeof("JMP") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "JMP", check_arg);
    case JA_COM:
        buf_ptr += sizeof("JA") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "JA", check_arg);
    case JAE_COM:
        buf_ptr += sizeof("JAE") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "JAE", check_arg);
    case JB_COM:
        buf_ptr += sizeof("JB") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "JB", check_arg);
    case JBE_COM:
        buf_ptr += sizeof("JBE") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "JBE", check_arg);
    case JE_COM:
        buf_ptr += sizeof("JE") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "JE", check_arg);
    case JNE_COM:
        buf_ptr += sizeof("JNE") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "JNE", check_arg);
    case IN_COM:
        buf_ptr += sizeof("IN") - 1;
        // printf(buf_ptr);
        // printf("|\n");
        if (LineIsEmptyComs(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("IN", line_ind);
        return false;
    case CALL_COM:
        buf_ptr += sizeof("CALL") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "CALL", check_arg);
    case RET_COM:
        buf_ptr += sizeof("RET") - 1;
        if (LineIsEmptyComs(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("RET", line_ind);
        return false;
    case WRONG_COMMAND_COM:
        return false;
    };
    fprintf(stderr, "Did not manage to get the information about the function on the line %llu", line_ind + 1);
    return false;
}

bool CheckPushCom(char *buf_ptr, size_t line_ind, int *check_arg) {
    assert(buf_ptr);
    assert(check_arg);

    buf_ptr += sizeof("PUSH") - 1;

    if (sscanf(buf_ptr, "%d", check_arg) == 0) {
        PRINT_BAD_ARGS("PUSH", line_ind);
        return false;
    }
    SpacesSkip(&buf_ptr);
    GraphemeSkip(&buf_ptr);
    if (LineIsEmptyComs(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS("PUSH", line_ind);
    return false;
}

bool CheckPushRegCom(char *buf_ptr, size_t line_ind, int *check_arg) {
    assert(buf_ptr);
    assert(check_arg);

    buf_ptr += sizeof("PUSH") - 1;
    SpacesSkip(&buf_ptr);

    *check_arg = 0;
    // printf("%d %d %d\n", buf_ptr[0] != '\0', isalpha(buf_ptr[0]), buf_ptr[0] != 'X');
    // printf("%d %d %d\n", buf_ptr[1] != '\0', isalpha(buf_ptr[1]), buf_ptr[1] != 'X');
    for (int i = 0; buf_ptr[i] != '\0' && isalpha(buf_ptr[i]) && buf_ptr[i] != 'X'; i++) {
        *check_arg += (buf_ptr[i] - 'A') + REGS_DEPTH;
    }
    *check_arg -= REGS_DEPTH;
    SpacesSkip(&buf_ptr);
    GraphemeSkip(&buf_ptr);
    if (LineIsEmptyComs(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS("PUSH", line_ind);
    return false;
}

bool CheckPushMemCom(char *buf_ptr, size_t line_ind, int *check_arg) {
    assert(buf_ptr);
    assert(check_arg);

    buf_ptr += sizeof("PUSH") - 1;
    SpacesSkip(&buf_ptr);

    if (*buf_ptr != '[') {
        PRINT_BAD_ARGS("PUSH", line_ind);
        return false;
    }
    buf_ptr++;
    SpacesSkip(&buf_ptr);
    *check_arg = 0;
    bool found = false;
    // printf("%d %d %d\n", buf_ptr[0] != '\0', isalpha(buf_ptr[0]), buf_ptr[0] != 'X');
    for (int i = 0; buf_ptr[i] != '\0' && isalpha(buf_ptr[i]) && buf_ptr[i] != 'X'; i++) {
        // printf("%s|\n", buf_ptr);
        *check_arg += buf_ptr[i] - 'A';
        found = true;
    }
    
    if (!found) {
        if (sscanf(buf_ptr, "%d", check_arg) == 0) {
            PRINT_BAD_ARGS("PUSH", line_ind);
            return false;
        }
    }
    SpacesSkip(&buf_ptr);
    GraphemeSkip(&buf_ptr);
    if (LineIsEmptyComs(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS("PUSH", line_ind);
    return false;
}

bool CheckPopRegCom(char *buf_ptr, size_t line_ind, int *check_arg) {
    assert(buf_ptr);
    assert(check_arg);

    buf_ptr += sizeof("POP") - 1;
    SpacesSkip(&buf_ptr);

    *check_arg = 0;
    for (int i = 0; buf_ptr[i] != '\0' && isalpha(buf_ptr[i]) && buf_ptr[i] != 'X'; i++) {
        *check_arg += (buf_ptr[i] - 'A') + REGS_DEPTH;
    }
    *check_arg -= REGS_DEPTH;
    SpacesSkip(&buf_ptr);
    GraphemeSkip(&buf_ptr);
    if (LineIsEmptyComs(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS("POP", line_ind);
    return false;
}

bool CheckPopMemCom(char *buf_ptr, size_t line_ind, int *check_arg) {
    assert(buf_ptr);
    assert(check_arg);

    buf_ptr += sizeof("POP") - 1;
    SpacesSkip(&buf_ptr);

    if (*buf_ptr != '[') {
        PRINT_BAD_ARGS("POP", line_ind);
        return false;
    }
    buf_ptr++;
    SpacesSkip(&buf_ptr);
    *check_arg = 0;
    bool found = false;
    for (int i = 0; buf_ptr[i] != '\0' && isalpha(buf_ptr[i]) && buf_ptr[i] != 'X'; i++) {
        *check_arg += buf_ptr[i] - 'A';
        found = true;
    }
    if (!found) {
        if (sscanf(buf_ptr, "%d", check_arg) == 0) {
            PRINT_BAD_ARGS("PUSH", line_ind);
            return false;
        }
    }
    SpacesSkip(&buf_ptr);
    GraphemeSkip(&buf_ptr);
    if (LineIsEmptyComs(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS("POP", line_ind);
    return false;
}

bool ChecKJmpCom(char *buf_ptr, size_t line_ind, const char *jmp_name, int *check_arg) {
    assert(buf_ptr);
    assert(jmp_name);
    assert(check_arg);

    SpacesSkip(&buf_ptr);
    if (*buf_ptr == ':') {
        buf_ptr++;
    }
    // printf("%s\n", (buf_ptr));
    if (sscanf(buf_ptr, "%d", check_arg) == 0) {
        PRINT_BAD_ARGS(jmp_name, line_ind);
        return false;
    }
    SpacesSkip(&buf_ptr);
    GraphemeSkip(&buf_ptr);
    if (LineIsEmptyComs(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS(jmp_name, line_ind);
    return false;
}

command_t ReadCommand(const char *com, char *buf, size_t line_ind) {
    assert(com);
    assert(buf);

    if (strcmp("OUT", com) == 0) {
        return OUT_COM;
    }
    if (strcmp("HLT", com) == 0) {
        return HLT_COM;
    }
    if (strcmp("PUSH", com) == 0) {
        SpacesSkip(&buf);
        GraphemeSkip(&buf);
        SpacesSkip(&buf);
        if (*buf == '[') {
            return PUSHM_COM;
        }
        if (isalpha(*buf)) {
            return PUSHR_COM;
        }
        return PUSH_COM;
    }
    if (strcmp("POP", com) == 0) {
        SpacesSkip(&buf);
        GraphemeSkip(&buf);
        SpacesSkip(&buf);
        if (*buf == '[') {
            return POPM_COM;
        }
        return POPR_COM;
    }
    if (strcmp("ADD", com) == 0) {
        return ADD_COM;
    }
    if (strcmp("SUB", com) == 0) {
        return SUB_COM;
    }
    if (strcmp("MULT", com) == 0) {
        return MULT_COM;
    }
    if (strcmp("DIV", com) == 0) {
        return DIV_COM;
    }
    if (strcmp("JMP", com) == 0) {
        return JMP_COM;
    }
    if (strcmp("JA", com) == 0) {
        return JA_COM;
    }
    if (strcmp("JAE", com) == 0) {
        return JAE_COM;
    }
    if (strcmp("JB", com) == 0) {
        return JB_COM;
    }
    if (strcmp("JBE", com) == 0) {
        return JBE_COM;
    }
    if (strcmp("JE", com) == 0) {
        return JE_COM;
    }
    if (strcmp("JNE", com) == 0) {
        return JNE_COM;
    }
    if (strcmp("IN", com) == 0) {
        return IN_COM;
    }
    if (strcmp("CALL", com) == 0) {
        return CALL_COM;
    }
    if (strcmp("RET", com) == 0) {
        return RET_COM;
    }
    fprintf(stderr, "Syntax Error: <%s> on the line %llu does not match the list of the commands\n", 
        com, line_ind + 1);
    return WRONG_COMMAND_COM;
}

// !SECTION