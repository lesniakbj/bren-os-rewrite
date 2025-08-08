#include <libc/stdlib.h>

void itoa(char* str_buf, int base, int value) {
    char *p = str_buf;
    char *p1, *p2;
    unsigned int ud = value;
    int divisor = 10;

    if (base == 'd' && value < 0) {
        *p++ = '-';
        str_buf++;
        ud = -value;
    } else if (base == 'x') {
        divisor = 16;
    }

    do {
        int remainder = ud % divisor;
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

 void* memcpy(void *destination, const void *source, size_t num) {
     unsigned char *d = (unsigned char *)destination;
     const unsigned char *s = (const unsigned char *)source;

     for (size_t i = 0; i < num; ++i) {
         d[i] = s[i];
     }
     return destination;
 }
