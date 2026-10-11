#include "Comms.h"

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

int GetFDesc(const char *f_name, int flags, mode_t f_mode);
size_t GetFileSize(int fd);
int GetCode(code_t *my_code, int fd);
size_t PlaceLineEnds(char buf[], size_t transferred);
buf_read_err_t ReadBuf(code_t *co, size_t d_c_lines_amount);
int SecondRun(code_t *co, size_t lines_amount);
int ScanLabel(char *buf_ptr, int *lab, size_t ind_line);
bool ScanCom(char *buf_ptr, char *com, size_t ind_line);
bool ScanPushArg(char *buf_ptr, size_t ind_line, int *arg);
bool LineIsEmpty(char *buf_ptr);
bool CheckComLine(command_t com, char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPushCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool ChecKJmpCom(char *buf_ptr, size_t line_ind, const char *jmp_name, int *check_arg);
command_t ReadCommand(const char com[], size_t line_ind);
FILE *GetFPtr(const char *f_name, const char *f_mode);
int BytedFWrite(FILE *fp, code_t *co);
void AnalyzeGotCode(int err);

int main(int argc, char const *argv[]) {
    switch (argc) {
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    default:
        fprintf(stderr, "WRONG PARAMETRS OF MAIN CALL\n");
        return 1;
    };
    int fd = GetFDesc("CodeFile.txt", O_RDONLY, S_IRUSR);
    if (fd < 0) {
        return 1;
    }
    code_t my_code = {};
    int errs_got = GetCode(&my_code, fd);
    if ((errs_got & ~CODE_IS_OK) != 0) {
        AnalyzeGotCode(errs_got);
        return 1;
    }
    FILE *w_fp = GetFPtr("ByteCode.txt", "w");
    if (w_fp == NULL) {
        return 1;
    }
    my_code.fp_dest = w_fp;
    // printf("Finally: %d\n",(int)GetCode(&my_code, fd));
    printf("err = %d, %llu, %p, %llu, %p, \nfd = %d, fp = %p, %llu\n", 
        my_code.cur_err, my_code.b_c_size, my_code.byted_code, my_code.d_c_size, my_code.default_code, 
        my_code.fd_src, my_code.fp_dest, my_code.src_size);
    for (size_t i = 0; i < my_code.b_c_size; i++) {
        printf("%d ", my_code.byted_code[i]);
    }
    printf("\n");
    for (size_t i = 0; i < LABEL_MAX; i++) {
        printf("%d ", my_code.labels[i]);
    }
    printf("\n");
    BytedFWrite(w_fp, &my_code);
    return 0;
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

int BytedFWrite(FILE *fp, code_t *co) {
    assert(fp);
    assert(co);

    int *reserve_b_c = co->byted_code;
    for (size_t ind = 0; ind < co->b_c_size; ) {
        if (co->byted_code[ind] != JMP_COM && 
            co->byted_code[ind] != JA_COM && co->byted_code[ind] != JAE_COM && 
            co->byted_code[ind] != JB_COM && co->byted_code[ind] != JBE_COM && 
            co->byted_code[ind] != JE_COM && co->byted_code[ind] != JNE_COM) {
            int offset = GetCodeOffset(co->byted_code[ind]);
            switch (offset) {
            case ONE_ARG_OFFSET:
                for (int j = 0; j < offset; j++) {
                    fprintf(fp, "%d%s", reserve_b_c[j], (j != offset - 1) ? " " : "\n");
                }
                break;
            case NO_ARG_OFFSET:
                fprintf(fp, "%d\n", *reserve_b_c);
                break;
            case HLT_COM_OFFSET:
                fprintf(fp, "%d ", *reserve_b_c++);
                ind++;
                break;
            default:
                fprintf(stderr, "UNKNOWN OFFSET GOT\n");
                return 0;
            };
            
            ind += offset;
            reserve_b_c += offset;
        }
        else {
            for (int j = 0; j < 2; j++) {
                fprintf(fp, "%d%s", reserve_b_c[j], (j != 2 - 1) ? " " : "\n");
            }
            ind += 2, reserve_b_c += 2;
        }
    }
    return 1;
}

int GetCode(code_t *my_code, int fd) {
    assert(my_code);

    if (fd < 0) {
        my_code->cur_err |= BAD_FILE_DESCRIPTOR;
        return my_code->cur_err;
    }
    my_code->fd_src = fd;

    my_code->src_size = GetFileSize(fd);
    if (my_code->src_size == 0) {
        fprintf(stderr, "BAD FILE DESCRIPTOR\n");
        my_code->cur_err |= BAD_R_FILE_SIZE;
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
    if (res_buf == NULL) {
        fprintf(stderr, "BAD REALLOC TRY\n");
        my_code->cur_err |= BAD_REALLOC_PTR;
        return my_code->cur_err;
    }
    my_code->default_code = res_buf;
    my_code->lines_amount = PlaceLineEnds(my_code->default_code, my_code->d_c_size);

    int *labels_ptr = (int *)calloc(LABEL_MAX, sizeof(int));
    if (labels_ptr == NULL) {
        fprintf(stderr, "BAD CALLOC POINTER\n");
        my_code->cur_err |= BAD_CALLOC_PTR;
        return my_code->cur_err;
    }
    my_code->labels = labels_ptr;

    buf_read_err_t buf_r_err = ReadBuf(my_code, my_code->lines_amount);
    if (buf_r_err != GOT_CODE_SUCCESSFULLY) {
        // TODO: ANALIZATOR OF READING PROCESS
        fprintf(stderr, "GOT BAD READING BUF ATTEMPT\n");
        my_code->cur_err |= BAD_BUF_PARSE;
        return my_code->cur_err;
    }
    if (SecondRun(my_code, my_code->lines_amount) != CODE_IS_OK) {
        fprintf(stderr, "BAD SECOND RUN FOR LABELS\n");
    }
    return my_code->cur_err;
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

    for (size_t ind_line = 0; ind_line < d_c_lines_amount; ind_line++) {
        while (isspace(*buf_ptr) && *buf_ptr != '\0') {
            buf_ptr++;
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
        command_t read_com = ReadCommand(com, ind_line);
        if (read_com == WRONG_COMMAND_COM) {
            fprintf(stderr, "As bad command found, compilation stops on the line %llu\n", ind_line + 1);
            return READ_WRONG_COM;
        }
        reserve_by_code[by_code_len++] = (int)read_com;

        int arg = 0;
        if (!CheckComLine(read_com, buf_ptr, ind_line, &arg)) {
            return READ_WRONG_COM;
        }
        
        // if (read_com == PUSH_COM) {
        //     reserve_by_code[by_code_len++] = arg;
        // }
        // if (read_com == JMP_COM || 
        //     read_com == JA_COM || read_com == JAE_COM || 
        //     read_com == JB_COM || read_com == JBE_COM || 
        //     read_com == JE_COM || read_com == JNE_COM) {
        //     by_code_len++;
        // }
        if (read_com == PUSH_COM || 
            read_com == JMP_COM || 
            read_com == JA_COM || read_com == JAE_COM || 
            read_com == JB_COM || read_com == JBE_COM || 
            read_com == JE_COM || read_com == JNE_COM) {
            if (read_com == JBE_COM) {
                printf("arg = %d\n", arg);
            }
            reserve_by_code[by_code_len++] = arg;
        }
        if (read_com == HLT_COM) {
            int *res_byted = (int *)realloc(reserve_by_code, by_code_len * sizeof(*reserve_by_code));
            if (res_byted == NULL) {
                return BAD_BUF_REALLOC;
            }
            co->b_c_size = by_code_len, co->byted_code = res_byted;
            return GOT_CODE_SUCCESSFULLY; 
        }
        buf_ptr += strlen(buf_ptr) + 1;
    }
    fprintf(stderr, "IMPLIED: HLT HAS NOT BEEN FOUND\n");
    return READ_F_REACHED_END;
}

int SecondRun(code_t *co, size_t lines_amount) {
    int offset = 0, pc = 0;

    char *buf_ptr = co->default_code;
    for (size_t line_ind = 0; line_ind < lines_amount; line_ind++) {
        while (isspace(*buf_ptr) && *buf_ptr != '\0') {
            buf_ptr++;
        }
        if (*buf_ptr == '\0') {
            buf_ptr++;
            continue;
        }
        
        int lab = 0, lab_scan_info = ScanLabel(buf_ptr, &lab, line_ind);
        // printf("lab = %d, ", lab);
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
        // printf("unmatched: %d\n", co->byted_code[pc]);
        printf("buf: <%s>\n", buf_ptr);
        if (!CheckComLine((command_t)co->byted_code[pc], buf_ptr, line_ind, &arg)) {
            fprintf(stderr, "UNMATCHED COMMAND HAS BEEN FOUND\n");
            co->cur_err |= BAD_LABEL_PLACEMENT;
            return co->cur_err;
        }
        if (co->byted_code[pc] == JMP_COM || 
            co->byted_code[pc] == JA_COM || co->byted_code[pc] == JAE_COM || 
            co->byted_code[pc] == JB_COM || co->byted_code[pc] == JBE_COM || 
            co->byted_code[pc] == JE_COM || co->byted_code[pc] == JNE_COM) {
            // printf("currend label: %d\n", arg);
            co->byted_code[++pc] = co->labels[arg];
            pc++;
            buf_ptr += strlen(buf_ptr) + 1;
            continue;
        }
        offset = GetCodeOffset(co->byted_code[pc]);
        
        if (offset == HLT_COM_OFFSET) {
            return CODE_IS_OK;
        }
        pc += offset;
        buf_ptr += strlen(buf_ptr) + 1;
    }
    co->cur_err |= BAD_LABEL_PLACEMENT;
    return BAD_LABEL_PLACEMENT;
}

int ScanLabel(char *buf_ptr, int *lab, size_t ind_line) {
    assert(buf_ptr);
    assert(lab);

    if (*buf_ptr++ == ':') {
        while (isspace(*buf_ptr) && *buf_ptr != '\0') {
            buf_ptr++;
        }
        if (sscanf(buf_ptr, "%d", lab) == 0) {
            fprintf(stderr, "SYNTAX ERROR: BAD LABEL on the line %llu\n", ind_line + 1);
            return GOT_BAD_LABEL;    
        }
        if (0 > *lab || *lab >= LABEL_MAX) {
            fprintf(stderr, "ASSEMBLE FAILED: TOO BIG LABEL on the line %llu\n", ind_line + 1);
            return GOT_BAD_LABEL;
        }
        while (!isspace(*buf_ptr) && *buf_ptr != '\0') {
            buf_ptr++;
        }
        if (LineIsEmpty(buf_ptr)) {
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

bool ScanPushArg(char *buf_ptr, size_t ind_line, int *arg) {
    if (sscanf(buf_ptr + sizeof("PUSH") - 1, "%d", arg) == 0) {
        fprintf(stderr, "Bad argumet got on the line %llu\n", ind_line + 1);
        return false;
    }
    return true;
}

bool LineIsEmpty(char *buf_ptr) {
    while (isspace(*buf_ptr) && *buf_ptr != '\0') {
        buf_ptr++;
    }
    if (*buf_ptr == '\0') {
        buf_ptr++;
        return true;
    }
    return false;
}

bool CheckComLine(command_t com, char *buf_ptr, size_t line_ind, int *check_arg) {
    assert(buf_ptr);
    assert(check_arg);

    if (com == JBE_COM) {
        printf("before begore: <%s>\n", buf_ptr);
    }
    
    switch (com) {
    case OUT_COM:
        buf_ptr += sizeof("OUT") - 1;
        if (LineIsEmpty(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("OUT", line_ind);
        return false;
    case HLT_COM:
        buf_ptr += sizeof("HLT") - 1;
        if (LineIsEmpty(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("HLT", line_ind);
        return false;
    case PUSH_COM:
        return CheckPushCom(buf_ptr, line_ind, check_arg);
    case ADD_COM:
        buf_ptr += sizeof("ADD") - 1;
        if (LineIsEmpty(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("ADD", line_ind);
        return false;
    case SUB_COM:
        buf_ptr += sizeof("SUB") - 1;
        if (LineIsEmpty(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("SUB", line_ind);
        return false;
    case MULT_COM:
        buf_ptr += sizeof("MULT") - 1;
        if (LineIsEmpty(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("MULT", line_ind);
        return false;
    case DIV_COM:
        buf_ptr += sizeof("DIV") - 1;
        if (LineIsEmpty(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("DIV", line_ind);
        return false;
    case JMP_COM:
        buf_ptr += sizeof("JMP") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "JMP", check_arg);
        return false;
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
        // printf("JBE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

        buf_ptr += sizeof("JBE") - 1;
        printf("befor enter: <%s>\n", buf_ptr);
        return ChecKJmpCom(buf_ptr, line_ind, "JBE", check_arg);
    case JE_COM:
        buf_ptr += sizeof("JE") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "JE", check_arg);
    case JNE_COM:
        buf_ptr += sizeof("JNE") - 1;
        return ChecKJmpCom(buf_ptr, line_ind, "JNE", check_arg);
    case WRONG_COMMAND_COM:
        return false;
    };
    fprintf(stderr, "Did not manage to get the information about the function on the line%llu", line_ind + 1);
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
    while (isspace(*buf_ptr) && *buf_ptr != '\0') {
        buf_ptr++;
    }
    while (!isspace(*buf_ptr) && *buf_ptr != '\0') {
        buf_ptr++;
    }
    if (LineIsEmpty(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS("PUSH", line_ind);
    return false;
}

bool ChecKJmpCom(char *buf_ptr, size_t line_ind, const char *jmp_name, int *check_arg) {
    assert(buf_ptr);
    assert(jmp_name);
    assert(check_arg);

    while (isspace(*buf_ptr) && *buf_ptr != '\0') {
        buf_ptr++;
    }
    if (*buf_ptr == ':') {
        buf_ptr++;
    }
    printf("%s\n", (buf_ptr));
    if (sscanf(buf_ptr, "%d", check_arg) == 0) {
        PRINT_BAD_ARGS(jmp_name, line_ind);
        return false;
    }
    while (isspace(*buf_ptr) && *buf_ptr != '\0') {
        buf_ptr++;
    }
    while (!isspace(*buf_ptr) && *buf_ptr != '\0') {
        buf_ptr++;
    }
    if (LineIsEmpty(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS(jmp_name, line_ind);
    return false;
}

command_t ReadCommand(const char com[], size_t line_ind) {
    if (strcmp("OUT", com) == 0) {
        return OUT_COM;
    }
    if (strcmp("HLT", com) == 0) {
        return HLT_COM;
    }
    if (strcmp("PUSH", com) == 0) {
        return PUSH_COM;
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
    fprintf(stderr, "Syntax Error: <%s> on the line %llu does not match the list of the commands\n", 
        com, line_ind + 1);
    return WRONG_COMMAND_COM;
}