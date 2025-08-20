#include <libc/stdlib.h>

void itoa(char* str_buf, kint32_t base, kint32_t value) {
    char *p = str_buf;
    char *p1, *p2;
    kuint32_t ud = value;
    kint32_t divisor = 10;

    if (base == 'd' && value < 0) {
        *p++ = '-';
        str_buf++;
        ud = -value;
    } else if (base == 'x') {
        divisor = 16;
    }

    do {
        kint32_t remainder = ud % divisor;
        *p++ = (remainder < 10) ? remainder + '0' : remainder + 'a' - 10;
    } while (ud /= divisor);

    *p = 0;

    p1 = str_buf;
    p2 = p - 1;
    while (p1 < p2) {
        char tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
        p1++;
        p2--;
    }
}