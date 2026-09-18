#include <stdio.h>

#define MAXLEN 1000

size_t StrLen(const char *line);
void PutS(const char *line);
void StrCpy(char *to, const char *from);
void StrCat(char *to, const char *from);
int StrCmp(const char *s1, const char *s2);

int main() {
    char s[] = {'a', '\0'};
    size_t len = StrLen(s);
    printf("%llu\n", len);
    PutS("abc\n");
    char from[] = "Smth";
    char to[] = "qwerty";
    printf("to = %s and ptr = %p\n", to, to);
    StrCpy(to, from);
    printf("to after = %s and ptr after = %p\n", to, to);
    char Cat_To[MAXLEN] = "BIG FLOPPA";
    char Cat_From[MAXLEN] = " IS GOOD";
    printf("CAT: from = %s, to = %s\n", Cat_From, Cat_To);
    StrCat(Cat_To, Cat_From);
    printf("After CAT: from = %s, to = %s\n", Cat_From, Cat_To);
    StrCat(Cat_To, " AND PELMEN");
    printf("After After CAT: from = %s, to = %s\n", " AND PELMEN", Cat_To);
    char s1[] = "Abc";
    char s2[] = "abc";
    printf("strcmp(s1, s2) = %d\n", StrCmp(s1, s2));
    return 0;
}

size_t StrLen(const char *line) {
    size_t i = 0;
    while (line[i] != '\0') {
        i++;
    }
    return i;
}

void PutS(const char *line) {
    while (*line != '\0') {
        putchar(*line++);
    }
}

void StrCpy(char *to, const char *from) {
    while ((*to++ = *from++) != '\0')
        ;
}

void StrCat(char *to, const char *from) {
    while (*to++ != '\0')
        ;
    to--;
    while ((*to++ = *from++) != '\0')
        ;
}

int StrCmp(const char *s1, const char *s2) {
    int i = 0;
    while (s1[i] == s2[i] && s1[i] != '\0') {
        i++;
    }
    if (s1[i] < s2[i]) {
        return -1;
    }
    else if (s1[i] == s2[i]) {
        return 0;
    }
    else {
        return 1;
    }
}

