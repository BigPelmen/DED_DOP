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
    GOT_CYCLE_NORMALLY,
    GOT_COM_NORMALLY,
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

struct argcv_parse {
    int fd, bin_fd, args_amount;
    FILE *w_fp;
    char const *name_r, *name_w, *name_b;
};

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

struct buf_info {
    code_t *co;
    size_t d_c_lines_amount, by_code_len;
    bool hlt_found;
    int *reserve_by_code;
    char *buf_ptr;
};

const char DFLT_READ_FILE_NAME[] = "CodeFile.txt";
const char DFLT_WR_FILE_NAME[] = "ByteCode.txt";
const char DFLT_WR_BIN_F_NAME[] = "ByteCode.bin";
#define EXTRA_NAME_CHARS 10

// SECTION - Functions prototypes


bool IsBinFormat(char const *f_name);
int Fill_ACVP_structure(struct argcv_parse *acvp, char const *where_read, char const *where_write);
int ArgC_V_Parse(struct argcv_parse *acvp);
int GetFDesc(const char *f_name, int flags, mode_t f_mode);
size_t GetFileSize(int fd);
const char *GetFBinName(const char *bin_f_name, size_t bin_f_n_size);
int RunAss(code_t *my_code, struct argcv_parse *acvp);
void DestroyAss(code_t *my_code, struct argcv_parse *acvp);
int GetCode(code_t *my_code, int fd);
bool CheckFileErr(code_t *my_code, int fd);
int CreateBuffers(code_t *my_code, size_t buf_size, int fd);
bool CheckResDCodeBuf(code_t *my_code, char *res_buf);
bool GetLabelsPtr(code_t *my_code);
size_t PlaceLineEnds(char buf[], size_t transferred);
buf_read_err_t ReadBuf(code_t *co, size_t d_c_lines_amount);
buf_read_err_t GoThroughBuf(struct buf_info *binf);
bool GotCommentOrO(char **buf_ptr);
buf_read_err_t GetFullCommand(struct buf_info *binf, char *com, size_t ind_line);
buf_read_err_t HltCheckReturn(struct buf_info *binf);
// bool CheckComHasArgs(int read_com);
int SecondRun(code_t *co, size_t lines_amount);
int ScanLabel(char *buf_ptr, int *lab, size_t ind_line);
int GotBadJmp(code_t *co, char **buf_ptr, int *pc, size_t line_ind);
bool ScanCom(char *buf_ptr, char *com, size_t ind_line);
bool LineIsEmptyComs(char *buf_ptr);
bool CheckComLine(command_t com, char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPushCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPushRegCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPushMemCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPushRegMemCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPopRegCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPopMemCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool CheckPopRegMemCom(char *buf_ptr, size_t line_ind, int *check_arg);
bool ChecKJmpCom(char *buf_ptr, size_t line_ind, const char *jmp_name, int *check_arg);
command_t ReadCommand(const char *com, char *buf, size_t line_ind);
FILE *GetFPtr(const char *f_name, const char *f_mode);
int BytedFWrite(FILE *fp, code_t *co);
bool IsJmpCom(int read_com_num);
void AnalyzeGotCode(int err);

// !SECTION
// SECTION - main

int main(int argc, char const *argv[]) {
    code_t my_code = {};
    struct argcv_parse acvp = {.fd = -1, .bin_fd = -1, .args_amount = argc, 
        .w_fp = NULL, .name_r = NULL, .name_w = NULL, .name_b = NULL};
    switch (argc) {
    case NO_WHERE:
        if (Fill_ACVP_structure(&acvp, DFLT_READ_FILE_NAME, DFLT_WR_FILE_NAME)) {
            return 1;
        }
        break;
    case WHERE_READ:
        if (Fill_ACVP_structure(&acvp, argv[1], DFLT_WR_FILE_NAME)) {
            return 1;
        }
        break;
    case WHERE_READ_WRITE:
        if (Fill_ACVP_structure(&acvp, argv[1], argv[2])) {
            return 1;
        }
        break;
    default:
        return 1;
    }
    // parse argc argv 
    // construct code
    // runasm
    // deleter/destructor;

    return RunAss(&my_code, &acvp);
}

bool IsBinFormat(char const *f_name) {
    assert(f_name);
    
    size_t len = strlen(f_name) - 1;
    if (f_name[len] == 'n' && f_name[len - 1] == 'i' && f_name[len - 2] == 'b' && f_name[len - 3] == '.') {
        return true;
    }
    return false;
}

int Fill_ACVP_structure(struct argcv_parse *acvp, char const *where_read, char const *where_write) {
    assert(acvp);

    switch (acvp->args_amount) {
    case NO_WHERE:
        acvp->name_r = DFLT_READ_FILE_NAME;
        acvp->name_w = DFLT_WR_FILE_NAME;
        acvp->name_b = DFLT_WR_BIN_F_NAME;
        break;
    case WHERE_READ:
        acvp->name_r = where_read;
        acvp->name_w = DFLT_WR_FILE_NAME;
        acvp->name_b = GetFBinName(where_read, strlen(where_read) + 1);
        break;
    case WHERE_READ_WRITE:
        acvp->name_r = where_read;
        if (IsBinFormat(where_write)) {
            acvp->name_w = DFLT_WR_FILE_NAME;
            acvp->name_b = where_write;
        }
        else {
            acvp->name_w = where_write;
            acvp->name_b = GetFBinName(where_write, strlen(where_read) + 1);
        }
        break;
    default:
        fprintf(stderr, "WRONG PARAMETRS OF MAIN CALL\n");
        acvp->name_r = NULL;
        acvp->name_w = NULL;
        acvp->name_b = NULL;
        return 1;
    }
    return ArgC_V_Parse(acvp);
}

int ArgC_V_Parse(struct argcv_parse *acvp) {
    assert(acvp);
    
    if (acvp->name_r == NULL || acvp->name_w == NULL || acvp->name_b == NULL) {
        return 1;
    }
    if ((acvp->fd = GetFDesc(acvp->name_r, O_RDONLY, S_IRUSR)) < 0) {
        return 1;
    }
    if ((acvp->w_fp = GetFPtr(acvp->name_w, "w")) == NULL) {
        return 1;
    }
    if ((acvp->bin_fd = open(acvp->name_b, O_CREAT | O_RDWR | O_TRUNC | O_BINARY, S_IRWXU)) < 0) {
        return 1;
    }
    return 0;
}

// !SECTION
// SECTION - Functions

const char *GetFBinName(const char *bin_f_name, size_t bin_f_n_size) {
    assert(bin_f_name);

    char *name = (char *)calloc(bin_f_n_size + EXTRA_NAME_CHARS, sizeof(char));
    if (name == NULL) {
        return DFLT_WR_BIN_F_NAME;
    }
    size_t offset = 0;
    while (bin_f_name[bin_f_n_size - 2 - offset] != '.' && bin_f_n_size - 2 >= offset) {
        offset++;
    }
    if (bin_f_name[bin_f_n_size - 2 - offset] != '.') {
        fprintf(stderr, "Incorrect file-to-write name\n");
    }
    strncpy(name, bin_f_name, bin_f_n_size - 1 - (offset));
    name[bin_f_n_size - 1 - offset] = 'b', name[bin_f_n_size - offset] = 'i', name[bin_f_n_size - offset + 1] = 'n';
    return (const char *)name;
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

int RunAss(code_t *my_code, struct argcv_parse *acvp) {
    assert(my_code);
    assert(acvp->w_fp);

    my_code->cur_err = CODE_IS_OK;
    int errs_got = GetCode(my_code, acvp->fd);
    if ((errs_got & ~CODE_IS_OK) != 0) {
        close(acvp->fd), fclose(acvp->w_fp), close(acvp->bin_fd);
        AnalyzeGotCode(errs_got);
        return 1;
    }

    printf(acvp->name_w);
    BytedFWrite(acvp->w_fp, my_code);
    int written_bytes = -1;
    if (my_code->b_c_size != 
        (written_bytes = write(acvp->bin_fd, my_code->byted_code, my_code->b_c_size * sizeof(int))) / sizeof(int)) {
        fprintf(stderr, "Not all the commands were written to the .bin file: %lld out from %lld\n", 
            my_code->b_c_size, written_bytes / sizeof(int));
        return 1;
    }
    
    DestroyAss(my_code, acvp);
    return 0;
}

void DestroyAss(code_t *my_code, struct argcv_parse *acvp) {
    assert(my_code);
    assert(acvp);

    close(acvp->fd), fclose(acvp->w_fp), close(acvp->bin_fd);
    acvp->fd = -1, acvp->w_fp = NULL, acvp->bin_fd = -1;
    if (acvp->name_b != DFLT_WR_BIN_F_NAME) {
        free((char *)acvp->name_b), acvp->name_b = NULL;
    }
    free(my_code->byted_code), my_code->byted_code = NULL;
    free(my_code->default_code), my_code->default_code = NULL;
    free(my_code->labels), my_code->labels = NULL;
}

int GetCode(code_t *my_code, int fd) {
    assert(my_code);

    if (!CheckFileErr(my_code, fd)) {
        return my_code->cur_err;
    }

    if (CreateBuffers(my_code, my_code->src_size + 1, fd)) {
        return my_code->cur_err;
    }
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

int CreateBuffers(code_t *my_code, size_t buf_size, int fd) {
    assert(my_code);

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

bool IsJmpCom(int read_com_num) {
    return (bool)(read_com_num == JMP_COM || 
        read_com_num == JA_COM || read_com_num == JAE_COM || 
        read_com_num == JB_COM || read_com_num == JBE_COM || 
        read_com_num == JE_COM || read_com_num == JNE_COM);
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

// string_view
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

    int *reserve_by_code = (int *)calloc(CODE_LINE_MULTIPLIER * d_c_lines_amount, sizeof(int));
    if (reserve_by_code == NULL) {
        fprintf(stderr, "BAD CALLOC ATTEMPT FOR RESERVE BYTED CODE\n");
        return BAD_BUF_CALLOC;
    }
    struct buf_info buf_params = {.co = co, .d_c_lines_amount = d_c_lines_amount, .by_code_len = 0, 
        .hlt_found = false, .reserve_by_code = reserve_by_code, .buf_ptr = co->default_code};
        
    buf_read_err_t cycle_err = GOT_CYCLE_NORMALLY;
    if ((cycle_err = GoThroughBuf(&buf_params)) != GOT_CYCLE_NORMALLY) {
        return cycle_err;
    }

    return HltCheckReturn(&buf_params);
}

buf_read_err_t GoThroughBuf(struct buf_info *binf) {
    assert(binf);

    char com[SIZEOF_COMMAND] = {};
    for (size_t ind_line = 0; ind_line < binf->d_c_lines_amount; ind_line++) {
        SpacesSkip(&binf->buf_ptr);
        if (GotCommentOrO(&binf->buf_ptr)) {
            continue;
        }

        int lab = 0, lab_scan_info = ScanLabel(binf->buf_ptr, &lab, ind_line);
        switch (lab_scan_info) {
        case GOT_GOOD_LABEL:
            binf->co->labels[lab] = binf->by_code_len;
            binf->buf_ptr += strlen(binf->buf_ptr) + 1;
            continue;
        case GOT_BAD_LABEL:
            return READ_WRONG_LABEL;
        case GOT_NO_LABEL:
            break;
        default:
            continue;
        };

        if (GetFullCommand(binf, com, ind_line) != GOT_COM_NORMALLY) {
            return READ_WRONG_COM;
        }
    }
    return GOT_CYCLE_NORMALLY;
}

bool GotCommentOrO(char **buf_ptr) {
    assert(buf_ptr);

    if (**buf_ptr == ';') {
        (*buf_ptr) += strlen(*buf_ptr) + 1;
        return true;
    }
    if (**buf_ptr == '\0') {
        (*buf_ptr)++;
        return true;
    }
    return false;
}

buf_read_err_t GetFullCommand(struct buf_info *binf, char *com, size_t ind_line) {
    assert(binf);
    assert(com);

    if (!ScanCom(binf->buf_ptr, com, ind_line)) {
        return READ_WRONG_COM;
    }
    command_t read_com = ReadCommand(com, binf->buf_ptr, ind_line);
    if (read_com == WRONG_COMMAND_COM) {
        fprintf(stderr, "As bad command found, compilation stops on the line %llu\n", ind_line + 1);
        return READ_WRONG_COM;
    }
    binf->reserve_by_code[binf->by_code_len++] = (int)read_com;

    int arg = 0;
    if (!CheckComLine(read_com, binf->buf_ptr, ind_line, &arg)) {
        return READ_WRONG_COM;
    }
    if (GetCodeOffset(read_com) == ONE_ARG_OFFSET) {
        binf->reserve_by_code[binf->by_code_len++] = arg;
    }
    if (read_com == HLT_COM) {
        binf->hlt_found = true;
    }
    binf->buf_ptr += strlen(binf->buf_ptr) + 1;
    return GOT_COM_NORMALLY;
}

buf_read_err_t HltCheckReturn(struct buf_info *binf) {
    assert(binf);

    if (binf->hlt_found) {
        int *res_byted = (int *)realloc(binf->reserve_by_code, 
            binf->by_code_len * sizeof(*binf->reserve_by_code));
        if (res_byted == NULL) {
            return BAD_BUF_REALLOC;
        }
        binf->co->b_c_size = binf->by_code_len, binf->co->byted_code = res_byted;
        return GOT_CODE_SUCCESSFULLY;
    }
    fprintf(stderr, "IMPLIED: HLT HAS NOT BEEN FOUND\n");
    return READ_F_REACHED_END;
}

int SecondRun(code_t *co, size_t lines_amount) {
    assert(co);

    int pc = 0;
    char *buf_ptr = co->default_code;
    for (size_t line_ind = 0; line_ind < lines_amount; line_ind++) {
        SpacesSkip(&buf_ptr);
        if (GotCommentOrO(&buf_ptr)) {
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
        }
        else {
            if (GotBadJmp(co, &buf_ptr, &pc, line_ind)) {
                return co->cur_err;
            }
        }
    }
    return co->cur_err;
}

int GotBadJmp(code_t *co, char **buf_ptr, int *pc, size_t line_ind) {
    assert(co);
    assert(buf_ptr);

    int arg = 0;
    if (IsJmpCom(co->byted_code[*pc]) || co->byted_code[*pc] == CALL_COM) {
        GraphemeSkip(buf_ptr);
        if (!ChecKJmpCom(*buf_ptr, line_ind, "SECOND RUN JMP", &arg)) {
            return (co->cur_err |= BAD_LABEL_PLACEMENT);
        }
        co->byted_code[++(*pc)] = co->labels[arg];
        (*pc)++;
    }
    else {
        *pc += GetCodeOffset(co->byted_code[*pc]);;
    }
    *buf_ptr += strlen(*buf_ptr) + 1;
    return 0;
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
    case PUSHRM_COM:
        return CheckPushRegMemCom(buf_ptr, line_ind, check_arg);
    case POPR_COM:
        return CheckPopRegCom(buf_ptr, line_ind, check_arg);
    case POPM_COM:
        return CheckPopMemCom(buf_ptr, line_ind, check_arg);
    case POPRM_COM:
        return CheckPopRegMemCom(buf_ptr, line_ind, check_arg);
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
    case DRAW_COM:
        buf_ptr += sizeof("DRAW") - 1;
        if (LineIsEmptyComs(buf_ptr)) {
            return true;
        }
        PRINT_SYNTAX_ERROR("DRAW", line_ind);
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
    SpacesSkip(&buf_ptr);

    if (sscanf(buf_ptr, "%d", check_arg) == 0) {
        PRINT_BAD_ARGS("PUSH", line_ind);
        return false;
    }
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
    for (int i = 0; buf_ptr[i] != '\0' && isalpha(buf_ptr[i]) && buf_ptr[i] != 'X'; i++) {
        *check_arg += (buf_ptr[i] - 'A') + REGS_DEPTH;
    }
    *check_arg -= REGS_DEPTH;

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
    if (sscanf(buf_ptr, "%d", check_arg) == 0) {
        PRINT_BAD_ARGS("PUSH", line_ind);
        return false;
    }
    while (*buf_ptr != ']' && *buf_ptr != '\0') {
        buf_ptr++;
    }
    if (*buf_ptr != ']') {
        PRINT_SYNTAX_ERROR("PUSH", line_ind);
        return false;
    }
    buf_ptr++;

    if (LineIsEmptyComs(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS("PUSH", line_ind);
    return false;
}

bool CheckPushRegMemCom(char *buf_ptr, size_t line_ind, int *check_arg) {
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
        *check_arg += (buf_ptr[i] - 'A') + REGS_DEPTH;
        found = true;
    }
    *check_arg -= REGS_DEPTH;
    GraphemeSkip(&buf_ptr);
    if (!found) {
        PRINT_BAD_ARGS("PUSH", line_ind);
        return false;
    }
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
    GraphemeSkip(&buf_ptr);
    if (LineIsEmptyComs(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS("POP", line_ind);
    return false;
}
//               0     1  2   3   4  5     6    7
// object<int> poison 1 323 123 543 123 6454 2323
// next         0     3   2   4  2        6    0
// prev         0     0       1  3    2    5


// err_t ListInsertAfter(list_t* list, size_t index) {
    // check if ok 
    // if not ok return err
    // AllocateObject 
    // ConectNodes
// }

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
    if (sscanf(buf_ptr, "%d", check_arg) == 0) {
        PRINT_BAD_ARGS("PUSH", line_ind);
        return false;
    }
    while (*buf_ptr != ']' && *buf_ptr != '\0') {
        buf_ptr++;
    }
    if (*buf_ptr != ']') {
        PRINT_SYNTAX_ERROR("POP", line_ind);
        return false;
    }
    buf_ptr++;
    if (LineIsEmptyComs(buf_ptr)) {
        return true;
    }
    PRINT_TOO_MANY_ARGS("POP", line_ind);
    return false;
}

bool CheckPopRegMemCom(char *buf_ptr, size_t line_ind, int *check_arg) {
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
        *check_arg += (buf_ptr[i] - 'A') + REGS_DEPTH;
        found = true;
    }
    *check_arg -= REGS_DEPTH;
    if (!found) {
        PRINT_BAD_ARGS("PUSH", line_ind);
        return false;
    }
    while (*buf_ptr != ']' && *buf_ptr != '\0') {
        buf_ptr++;
    }
    if (*buf_ptr != ']') {
        PRINT_SYNTAX_ERROR("POP", line_ind);
        return false;
    }
    buf_ptr++;
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
            buf++;
            SpacesSkip(&buf);
            if (isalpha(*buf)) {
                return PUSHRM_COM;
            }
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
        if (*buf++ == '[') {
            SpacesSkip(&buf);
            if (isalpha(*buf)) {
                return POPRM_COM;
            }
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
    if (strcmp("CALL", com) == 0) {
        return CALL_COM;
    }
    if (strcmp("RET", com) == 0) {
        return RET_COM;
    }
    if (strcmp("IN", com) == 0) {
        return IN_COM;
    }
    if (strcmp("DRAW", com) == 0) {
        return DRAW_COM;
    }
    fprintf(stderr, "Syntax Error: <%s> on the line %llu does not match the list of the commands\n", 
        com, line_ind + 1);
    return WRONG_COMMAND_COM;
}

// !SECTION