#include "Utils.hpp"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

std::string format(const std::string& fmt, ...)
{
    int size = ((int)fmt.size()) * 2 + 50;
    std::string str;
    va_list ap;
    while (1) {
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.data(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {
            str.resize(n);
            return str;
        }
        if (n > -1)
            size = n + 1;
        else
            size *= 2;
    }
    return str;
}

void hexdump(const unsigned char *data, size_t length)
{
    char asciibuf[18];
    asciibuf[16] = '\0';
    for (unsigned int index = 0; index < length; index++) {
        if((index % 16) == 0) {
            printf("%08X ", index);
        }

        if((index % 8) == 0) {
            putchar(' ');
        }
        printf("%02X ", data[index]);
        asciibuf[index % 16] = isprint(data[index])?data[index]:'.';

        if((index % 16) == 15) {
            printf("  %s\n", asciibuf);
        }
    }
    printf("\n");
}
