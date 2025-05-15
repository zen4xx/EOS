#include "string.h"

int strlen(char* str){
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

void backspace(char s[]){
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