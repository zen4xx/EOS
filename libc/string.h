#ifndef STRING_H
#define STRING_H

int strlen(char str[]);
void reverse(char str[]);
char* itoa(int);
int strcmp(char s1[], char s2[]);
char* strdup(const char* s);
void backspace(char s[]);
void append(char s1[], char c);
void append_str(char s1[], char s2[]);
int atoi(char* s);
int isspace(char c);

#endif