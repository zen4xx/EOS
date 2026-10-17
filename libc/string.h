#ifndef STRING_H
#define STRING_H

int strlen(const char *str);
void reverse(char *str);
void itoa(int, char* str);
int strcmp(char *s1, char *s2);
void backspace(char *s);
void append(char *s1, char c);
void append_str(char *s1, const char *s2);
int atoi(char* s);
int isspace(char c);
char* strcpy(char *strDest, const char *strSrc);
char *strncpy(char *dest, const char *src, unsigned long long count);
char *strtok_r(char *str, const char *delim, char **saveptr);
char *strrchr(const char *s, int c);

#endif