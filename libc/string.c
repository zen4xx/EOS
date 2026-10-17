#include "string.h"

int strlen(const char* str){
	int r = 0;
	while(str[r] != '\0') ++r;
	return r;
}

void reverse(char* s) {
    int c, i, j;
    for (i = 0, j = strlen(s)-1; i < j; ++i, --j) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n, char* str){
	int i, sign;
	if((sign = n) < 0) n = -n;
	i = 0;
	do {
		str[i++] = n % 10 + '0';
	} while((n /= 10) > 0);

	if(sign < 0) str[i++] = '-';
	str[i] = '\0';
    reverse(str);
}

int atoi(char* s) {
    int res = 0;
    int sign = 1;

    while (*s == ' ') s++;

    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }

    return sign * res;
}

int strcmp(char* s1, char* s2){
	while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int isspace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

void backspace(char *s){
	s[strlen(s)-1] = '\0';
}

void append(char *s, char n) {
    int len = strlen(s);
    s[len] = n;
    s[len+1] = '\0';
}

void append_str(char* s1, char* s2){
	for(int i = 0; i < strlen(s2); ++i){
		append(s1, s2[i]);
	}
	append(s1, '\0');
}

char* strcpy(char *strDest, const char *strSrc)
{
    char *temp = strDest;
    while((*strDest++ = *strSrc++));
    return temp;
}

char* strncpy(char *dest, const char *src, unsigned long long count) {
    if (count != 0) {
        char *d = dest;
        const char *s = src;
        do {
            if ((*d++ = *s++) == 0) {
                while (--count != 0)
                    *d++ = 0;
                break;
            }
        } while (--count != 0);
    }
    return dest;
}

char *strtok_r(char *str, const char *delim, char **saveptr)
{
    char *token_start;
    char *token_end;
    
    if (str != 0) {
        token_start = str;
    } else {
        token_start = *saveptr;
    }
    
    if (token_start == 0 || *token_start == '\0') {
        *saveptr = 0;
        return 0;
    }
    
    while (*token_start != '\0') {
        int is_delim = 0;
        for (const char *d = delim; *d != '\0'; d++) {
            if (*token_start == *d) {
                is_delim = 1;
                break;
            }
        }
        if (!is_delim) break;
        token_start++;
    }
    
    if (*token_start == '\0') {
        *saveptr = 0;
        return 0;
    }
    
    token_end = token_start;
    while (*token_end != '\0') {
        int is_delim = 0;
        for (const char *d = delim; *d != '\0'; d++) {
            if (*token_end == *d) {
                is_delim = 1;
                break;
            }
        }
        if (is_delim) break;
        token_end++;
    }
    
    if (*token_end != '\0') {
        *token_end = '\0';
        *saveptr = token_end + 1;
    } else {
        *saveptr = 0;
    }
    
    return token_start;
}

char *strrchr(const char *s, int c) 
{
    if (s == 0) {
        return 0;
    }

    char *p_char = 0;
    while (*s != '\0') {
        if (*s == (char)c) {
            p_char = (char *)s;
        }
        s++;
    }
    return p_char;
}
