#include "win.h"
#include <stdarg.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <time.h>

void err(int eval, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);
    fprintf(stderr, ": %s\n", strerror(GetLastError()));

    va_end(args);

    exit(eval);
}

void errx(int eval, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);

    exit(eval);
}

void warn(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);
}

int strncasecmp(const char* s1, const char* s2, size_t n)
{
    if (n == 0) return 0;

    while (n-- != 0 && tolower((unsigned char)*s1) == tolower((unsigned char)*s2))
    {
        if (n == 0 || *s1 == '\0' || *s2 == '\0')
            break;
        s1++;
        s2++;
    }

    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

size_t strlcat(char* dst, const char* src, size_t dstsize)
{
    size_t dlen = strnlen(dst, dstsize);
    size_t slen = strlen(src);

    if (dlen == dstsize) return dstsize + slen;

    if (slen < dstsize - dlen)
    {
        memcpy(dst + dlen, src, slen + 1);
    }
    else
    {
        memcpy(dst + dlen, src, dstsize - dlen - 1);
        dst[dstsize - 1] = '\0';
    }

    return dlen + slen;
}

char* strptime(const char* str, const char* format, struct tm* tm)
{
    if (!str || !tm || strlen(str) < 8)
    {
        return NULL;
    }

    memset(tm, 0, sizeof(struct tm));

    const char* ptr = str;
    while (*ptr)
    {
        if (!isdigit(*ptr))
        {
            return NULL;
        }
        ptr++;
    }

    char month_str[3] = { str[0], str[1], '\0' };
    char day_str[3] = { str[2], str[3], '\0' };
    char year_str[5] = { str[4], str[5], str[6], str[7], '\0' };

    int month = atoi(month_str);
    int day = atoi(day_str);
    int year = atoi(year_str);

    if (month < 1 || month > 12 || day < 1 || day > 31 || year < 0)
    {
        return NULL;
    }

    tm->tm_year = year - 1900;
    tm->tm_mon = month - 1;
    tm->tm_mday = day;

    tm->tm_hour = 0;
    tm->tm_min = 0;
    tm->tm_sec = 0;
    tm->tm_isdst = -1;

    return (char*)str + 8;
}

char* strdupa(const char* s)
{
    return _strdup(s);
}

void* mempcpy(void* dest, const void* src, size_t n)
{
    return (char*)memcpy(dest, src, n) + n;
}

ssize_t getline(char** lineptr, size_t* n, FILE* stream)
{
    if (lineptr == NULL || n == NULL || stream == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    char* buf = *lineptr;
    size_t size = *n;
    size_t len = 0;

    int c = fgetc(stream);
    if (c == EOF)
    {
        return -1;
    }

    if (buf == NULL)
    {
        size = 128;
        buf = (char*)malloc(size);
        if (buf == NULL)
        {
            return -1;
        }
    }

    while (c != EOF)
    {
        if (len + 1 >= size)
        {
            size *= 2;
            char* new_buf = (char*)realloc(buf, size);
            if (new_buf == NULL)
            {
                free(buf);
                return -1;
            }
            buf = new_buf;
        }

        buf[len++] = (char)c;
        if (c == '\n')
        {
            break;
        }
        c = fgetc(stream);
    }

    buf[len] = '\0';
    *lineptr = buf;
    *n = size;

    return len;
}

char* strchrnul(const char* s, int c)
{
    while (*s && *s != (char)c)
    {
        s++;
    }
    return (char*)s;
}

char* strtok_r(char* str, const char* delim, char** saveptr)
{
    char* token;

    if (str == NULL)
    {
        str = *saveptr;
    }

    str += strspn(str, delim);
    if (*str == '\0')
    {
        return NULL;
    }

    token = str;
    str = strpbrk(token, delim);
    if (str == NULL)
    {
        *saveptr = strchr(token, '\0');
    }
    else
    {
        *str = '\0';
        *saveptr = str + 1;
    }

    return token;
}