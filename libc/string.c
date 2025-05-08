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

char* itoa(int n){
	char* str;
	int i, sign;
	if((sign = n) < 0) n = -n;
	i = 0;
	do {
		str[i++] = n % 10 + '0';
	} while((n /= 10) > 0);

	if(sign < 0) str[i++] = '-';
	str[i] = '\0';

    reverse(str);

	return str;
}

int atoi(char* s){
	int res = 0;
	char* reversed_s = s;
	reverse(reversed_s);
	for(int i = 0; i < strlen(reversed_s); ++i){
		if(reversed_s[i] != '-'){
			res += (reversed_s[i] - '0');
		}
		else{
			res *= -1;
		}
	}
	return res;
}

int strcmp(char* s1, char* s2){
	if(strlen(s1) != strlen(s2)) return 0;
	int i = strlen(s1);
	while(i--) if (s1[i] != s2[i]) return 0;
	return 1;
}

char* strdup(const char* s) {
    int len = strlen(s) + 1;
    char* dst = (char*)allocate(len);
    if (!dst) return 0;
    mem_cpy(dst, s, len);
    return dst;
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