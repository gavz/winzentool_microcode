#ifndef WIN_H
#define WIN_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <json-c/json_inttypes.h>

void err(int eval, const char* fmt, ...);
void errx(int eval, const char* fmt, ...);
void warn(const char* fmt, ...);
int strncasecmp(const char* s1, const char* s2, size_t n);
size_t strlcat(char* dst, const char* src, size_t dstsize);
char* strptime(const char* str, const char* format, struct tm* tm);
char* strdupa(const char* s);
void* mempcpy(void* dest, const void* src, size_t n);
char* strchrnul(const char* s, int c);
ssize_t getline(char** lineptr, size_t* n, FILE* stream);
char* strtok_r(char* str, const char* delim, char** saveptr);

#endif
