#include "common_func.h"

void cf_itoa(int value, char* str) {
    int i = 0, isNegative = 0;
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    if (value < 0) {
        isNegative = 1;
        value = -value;
    }
    while (value != 0) {
        str[i++] = (value % 10) + '0';
        value /= 10;
    }
    if (isNegative) str[i++] = '-';
    str[i] = '\0';

    // reverse
    for (int j = 0; j < i / 2; ++j) {
        char tmp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = tmp;
    }
}

#if 0
size_t cf_strlen(const char* s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}
#endif

void cf_strcat(char* dest, const char* src) {
    while (*dest) dest++;
    while ((*dest++ = *src++));
}

char* cf_strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return original_dest;
}


